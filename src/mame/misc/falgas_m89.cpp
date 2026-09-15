// license:BSD-3-Clause
// copyright-holders:
/***************************************************************************

   M89 hardware for kiddie rides from Falgas.

   Base Falgas M89 PCB:

   _|_|_|_|___|_|_|_|___|_|_|_|_|_|_|____
  |          _______   _______ _______  |
  | _______  TIC206M   TIC206M TIC206M  |
  |                                     |
  |                   __________________|
  |                  | AY8910A         ||
  |                  |_________________||
  |                                     |
  | 7805CV            :::::::::         |
  |                   __________________|
  |                  | 82C55           ||
  |                  |_________________||
  | TDA7241B             _______________|
  |                     | GM76C28A     ||
  |                     |______________||
  |                    _________________|
  |                   | EPROM          ||
  |                   |________________||
  |    __________           __________  |
  |   |_PAL16V8_|          |SN74LS373N  |
  |  __________       __________________|
  | |_PAL16V8_|      |OKI M80C85A-2    ||
  |                  |_________________||
  |  6.000 MHz Xtal  _________ ________ |
  |                MC14020BCP MC14020BCP|
  | ____________RISER_PCB_______________|
  |_____________________________________|

  The riser PCB contains:
   -4 LEDs (motor on, coin input, timer-sound, light).
   -Bank of 4 dipswitches for timer configuration.
   -Bank of 4 dipswitches for coinage configuration.
   -Volume knob.


   Optional video PCB (25291):
   ______________________________________
  |   Power conn -> ::::::  :::::::::: <- Conn to M89 (timer, sound)
  |                   __________________|
  |                  | NEC D8155HC     ||
  |                  |_________________||
  |                                     |
  |                     ________________|
  |           __       | GM76C28A-10   ||
  |          | |       |_______________||
SN74LS14N -> | |       _________________|
  |          |_|      | EPROM          ||
  |                   |________________||
  |      __________        __________   |
  |     |_PAL16V8_|       |SN74LS373N|  |
  |                   __________________|
  |             Xtal | OKI M80C85A-2   ||
  |        6.000 MHz |_________________||
  |                   __________________|
  |             Xtal | TMS9129NL       ||
  |    10.738635 MHz |_________________||
  |                           __________|
  |                          |UD61464DC||
  |                           __________|
  |                          |UD61464DC||
  |  ___   ___   ___     _________  :   |
  |LM318P LM318P LM318P |CD4016BCN  : <- Conn to monitor
  |                                 :   |
  |_____________________________________|


  TODO (for games with video):
  * main - video CPUs communications
  * inputs

***************************************************************************/

#include "emu.h"

#include "cpu/i8085/i8085.h"
#include "machine/i8155.h"
#include "machine/i8255.h"
#include "machine/timer.h"
#include "machine/watchdog.h"
#include "sound/ay8910.h"
#include "video/tms9928a.h"

#include "screen.h"
#include "speaker.h"

#include "falgas_m87.lh"

namespace
{

class falgasm89_state : public driver_device
{
public:
	falgasm89_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_inputs(*this, "IN%u", 0U)
		, m_psg_pa(0xff)
	{
	}

	void falgasm89_simple(machine_config &config);
	void falgasm89(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;

	required_device<i8085a_cpu_device> m_maincpu;

private:
	void psg_pa_w(u8 data);
	u8 psg_pb_r();

	void mem_map(address_map &map) ATTR_COLD;
	void io_map(address_map &map) ATTR_COLD;

	required_ioport_array<4> m_inputs;

	u8 m_psg_pa;
};

class falgasm89_video_state : public falgasm89_state
{
public:
	falgasm89_video_state(const machine_config &mconfig, device_type type, const char *tag)
		: falgasm89_state(mconfig, type, tag)
		, m_videocpu(*this, "videocpu")
	{
	}

	void falgasm89_video(machine_config &config);

private:
	required_device<i8085a_cpu_device> m_videocpu;

	void main_io_map(address_map &map) ATTR_COLD;
	void video_mem_map(address_map &map) ATTR_COLD;
	void video_io_map(address_map &map) ATTR_COLD;
};


/***************************************************************************

   Falgas Micro-87 hardware.

   Older than M89 and simpler.  Everything lives on a single board silkscreened
   9187B: there is no riser PCB, just a folded metal front panel screwed to the
   board edge that carries the LEDs, the knobs and the selector.  Separate
   daughter boards exist for the lamps and the buttons, reached through the two
   IDC connectors.  Photographed example (fantcar87, EPROM labelled
   "DATA 15.2.88 / SOFT: MIKEL ES / HARD: M.87.E"):

	 NEC D8085AHC        CPU
	 4 MHz crystal
	 SN74LS373 PC        address latch for the multiplexed bus
	 T74LS138B1          I/O decoder
	 74LS32 PC           /
	 KS74HCTLS00N        \ read and write strobes
	 HCF4093BE (SGS)     RC oscillators for the two interrupts
	 NM27C256Q           program EPROM
	 KM6816AL-15         2 KiB SRAM
	 AY-3-8910 (GI)      sound and the only general purpose I/O
	 3021-type optotriac, triac and heatsink for the mains loads
	 10K potentiometer mounted on the board, shaft through the panel

   Note that no output latch is fitted on the main board: the only 373 is the
   address latch next to the CPU.  The latches live on the daughter boards
   reached through the IDC connectors, clocked by the decoded write strobes:

	 driver board    four 20 pin Fairchild octal latches (the part number ends
					 in 77, so almost certainly 74LS377) feeding four ULN2003A
					 Darlington arrays and a row of power devices, plus a
					 relay.  Four latches of eight bits is 32 outputs, which
					 lines up with the four strobes left over once E8h is
					 accounted for.
	 display board   silkscreened 20187, three 7 segment digits and two
					 HCF4511BE BCD latch/decoder/drivers on a 20 pin IDC
	 lamp boards     phenolic boards carrying the filament bulbs themselves,
					 around forty per board, no active parts

   The front panel carries a resettable breaker with a red button, a 220 V
   neon, a FAIL LED, two panel fuses (SOUND-TIMER and LIGHT), a bank of four
   dipswitches labelled SELECTOR on a small daughter board behind a cutout, a
   SOUND volume knob, a TIMER knob graduated in seconds (60, 90, 120 and 150
   are printed, with ticks in between), and a row of five red LEDs labelled,
   in this order:

	 MOTOR ON    COIN INPUT    SOUND    TIMER    LIGHT

   Memory map (from the code):

	 0000-7FFF   R   EPROM (27C256)
	 8000-87FF   R/W KM6816AL-15 (2 KiB SRAM).  The stack is set to 8800 and
					 grows down, and the boot code clears 8010-87FF, so the
					 2 KiB are fully used.  Mirroring is a guess: the decode
					 most likely only looks at A15.

   I/O map.  The 74LS138 decodes A7-A5 of the I/O address (enabled for
   A7=A6=1), so each device answers to a whole block of eight ports; the ROMs
   always use the port with A2 set, which is why the skeleton only listed C4h
   and CCh:

	 C0-C7  R/W  AY-3-8910 data      (ROMs use C4h)
	 C8-CF    W  AY-3-8910 address   (ROMs use CCh)
	 D0-D7    W  watchdog strobe; data is don't care and the boot code hits it
				 three times in a row.  Almost certainly the retriggerable
				 monostable built around the HCF4093BE.
	 D8-DF    W  strobe, used once at boot with 40h on the bus
	 E0-E7    W  strobe
	 E8-EF    W  strobe for the latch that feeds the display board.  Bits 3-6
				 carry a BCD digit and bits 2 and 7 are pulsed low and back
				 again, which is exactly how you drive the LE inputs of two
				 HCF4511s: the latch is transparent while LE is low and holds
				 the digit when it goes high again.  See digit_w below.
	 F0-F7    W  strobe
	 F8-FF  R    player controls, 4 bits, active low
			W    strobe

   Interrupts.  Two periodic sources, both RC astables built around the
   HCF4093BE, which has resistors and a capacitor on both sides of it:

	 TRAP     refreshes the AY port A outputs every interrupt and divides by 50
			  (32h) to produce the flag that paces the whole main loop, so the
			  sound engine and the input scan run at TRAP/50.  Calibrated
			  against a recording of a real machine by playing the same tune
			  back and matching it note for note: the melody comes out right,
			  so the only free parameter is the tempo, and the ratio of the
			  note onsets pins the frame at 10.5 ms.  That puts TRAP at about
			  4.8 kHz and the input scan at 95 Hz, which also makes the coin
			  debouncer sensible, since it wants two consecutive samples, so
			  about 21 ms of closed contact.  NOT mains derived: at 50 Hz the
			  frame rate would be 1 Hz and nothing would work at all.
	 RST 7.5  time base for the length of the ride.  The handler increments a
			  prescaler, compares it against a limit taken from the coinage
			  table and, when they match, decrements the ride counter (which
			  starts at the value stored at ROM offset 0003h).  This is what
			  the TIMER potentiometer on the panel adjusts, by changing the
			  frequency of the astable, so it is modelled here as a
			  PORT_ADJUSTER rather than as something the program can read.

   TODO:
   * work out which AY port A bit lights which of the five panel LEDs, and what
	 drives the FAIL LED
   * identify what the D8h-F8h strobes reach on the daughter boards and hook up
	 the player controls read at F8h
   * SOD is driven low at boot and high for exactly as long as the ride lasts:
	 motor enable, or the signal behind the TIMER LED
   * measure the real RC values of the watchdog and of the two astables

***************************************************************************/

class falgasm87_state : public driver_device
{
public:
	falgasm87_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_psg(*this, "psg")
		, m_traptimer(*this, "traptimer")
		, m_ridetimer(*this, "ridetimer")
		, m_controls(*this, "IN1")
		, m_dial(*this, "DIAL")
		, m_leds(*this, "led%u", 0U)
		, m_digits(*this, "digit%u", 0U)
		, m_lamps(*this, "lamp%u", 0U)
	{
	}

	void falgasm87(machine_config &config);

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	required_device<i8085a_cpu_device> m_maincpu;
	required_device<ay8910_device> m_psg;
	required_device<timer_device> m_traptimer;
	required_device<timer_device> m_ridetimer;
	required_ioport m_controls;
	required_ioport m_dial;
	output_finder<6> m_leds;
	output_finder<2> m_digits;
	output_finder<4> m_lamps;

	u8 m_outlatch[5];
	u8 m_sod;
	u8 m_trap;

	void mem_map(address_map &map) ATTR_COLD;
	void io_map(address_map &map) ATTR_COLD;

	void leds_w(u8 data);
	u8 controls_r();
	template <unsigned N> void outlatch_w(u8 data);
	void digit_w(u8 data);
	void sod_w(int state);

	attotime dial_period() const;

	TIMER_DEVICE_CALLBACK_MEMBER(trap_tick);
	TIMER_DEVICE_CALLBACK_MEMBER(ride_tick);
};


void falgasm89_state::machine_start()
{
	save_item(NAME(m_psg_pa));
}

void falgasm89_state::psg_pa_w(u8 data)
{
	m_psg_pa = data;
}

u8 falgasm89_state::psg_pb_r()
{
	u8 result = 0xff;
	for (int n = 0; n < 4; n++)
		if (!BIT(m_psg_pa, n))
			result &= m_inputs[n]->read();

	return result;
}

void falgasm89_state::mem_map(address_map &map)
{
	map(0x0000, 0xbfff).rom().region("maincpu", 0);
	map(0xfc00, 0xffff).ram();
}

void falgasm89_state::io_map(address_map &map)
{
	map(0x00, 0x00).rw("psg", FUNC(ay8910_device::data_r), FUNC(ay8910_device::data_w));
	map(0x04, 0x04).w("psg", FUNC(ay8910_device::address_w));
}

void falgasm89_video_state::main_io_map(address_map &map)
{
	map(0x00, 0x00).rw("psg", FUNC(ay8910_device::data_r), FUNC(ay8910_device::data_w));
	map(0x04, 0x04).w("psg", FUNC(ay8910_device::address_w));
	map(0x98, 0x98).lw8(NAME([this] (u8 data) { logerror("to video: %02x\n", data); }));
	map(0x99, 0x99).lr8(NAME([this] () -> u8 { logerror("from video\n"); return 0xff; }));
}

void falgasm89_video_state::video_mem_map(address_map &map)
{
	map(0x0000, 0x8fff).rom().region("videocpu", 0);
	map(0xf800, 0xffff).ram();
	//map(0xf800, 0xf8ff).rw("i8155", FUNC(i8155_device::memory_r), FUNC(i8155_device::memory_w)); // TODO: where's this?
}

void falgasm89_video_state::video_io_map(address_map &map)
{
	map(0x00, 0x07).rw("i8155", FUNC(i8155_device::io_r), FUNC(i8155_device::io_w));
	map(0x08, 0x08).rw("vdp", FUNC(tms9129_device::vram_read), FUNC(tms9129_device::vram_write));
	map(0x09, 0x09).rw("vdp", FUNC(tms9129_device::register_read), FUNC(tms9129_device::register_write));
}


/***************************************************************************
   Falgas Micro-87 implementation
***************************************************************************/

void falgasm87_state::machine_start()
{
	// output_finder resolves itself now; there is no resolve() to call
	std::fill(std::begin(m_outlatch), std::end(m_outlatch), 0);
	m_sod = 0;
	m_trap = 0;

	save_item(NAME(m_outlatch));
	save_item(NAME(m_sod));
	save_item(NAME(m_trap));
}

void falgasm87_state::machine_reset()
{
	m_trap = 0;
	m_ridetimer->adjust(dial_period());
}

void falgasm87_state::mem_map(address_map &map)
{
	map(0x0000, 0x7fff).rom().region("maincpu", 0);
	map(0x8000, 0x87ff).mirror(0x7800).ram(); // KM6816AL-15
}

void falgasm87_state::io_map(address_map &map)
{
	// only A7-A5 reach the TC74HC138P, hence the eight-port blocks
	map(0xc0, 0xc7).rw(m_psg, FUNC(ay8910_device::data_r), FUNC(ay8910_device::data_w));
	map(0xc8, 0xcf).w(m_psg, FUNC(ay8910_device::address_w));
	map(0xd0, 0xd7).w("watchdog", FUNC(watchdog_timer_device::reset_w));
	map(0xd8, 0xdf).w(FUNC(falgasm87_state::outlatch_w<0>));
	map(0xe0, 0xe7).w(FUNC(falgasm87_state::outlatch_w<1>));
	map(0xe8, 0xef).w(FUNC(falgasm87_state::outlatch_w<2>));
	map(0xf0, 0xf7).w(FUNC(falgasm87_state::outlatch_w<3>));
	map(0xf8, 0xff).rw(FUNC(falgasm87_state::controls_r), FUNC(falgasm87_state::outlatch_w<4>));
}

/*
   AY-3-8910 port A (register 14, programmed as output by R7 = 7Fh) is the only
   place on the main board where eight bits of state can be held.  Five bits
   are used by either ROM revision:

	 bit 1     set while the machine holds credit and cleared otherwise
	 bits 4-7  a nibble stream from ROM, selected together with each sound
			   effect and consumed by the TRAP handler at one nibble per
			   interrupt, so around 4800 nibbles per second

   Whatever bits 4-7 drive, it is not the five panel LEDs: at that rate nothing
   is visible, and wiring them to the layout just produces a flickering mess.
   They are the car lights, and the rate points at brightness control rather
   than plain on/off.  The mains loads go through a 3021 type optotriac, which
   is the non zero crossing kind used for phase angle control, and at 4.8 kHz a
   mains half cycle is about 48 interrupts, so a 0-15 nibble fits as a firing
   delay.  They are exposed as lamp0-lamp3 and deliberately kept out of the
   layout until someone can check this with a scope.

   That leaves the five monitor LEDs on the panel, of which only two can be
   justified from the code:

	 MOTOR ON      SOD, which goes high exactly when the ride starts and low
				   exactly when the ride counter runs out.  The TIMER LED would
				   follow the same signal, so which of the two it really is has
				   not been settled.
	 COIN INPUT    AY port A bit 1

   TODO: SOUND, TIMER and LIGHT are left undriven rather than guessed at.
*/
void falgasm87_state::leds_w(u8 data)
{
	m_leds[1] = BIT(data, 1); // coin input

	for (int i = 0; i < 4; i++)
		m_lamps[i] = BIT(data, 4 + i);
}

u8 falgasm87_state::controls_r()
{
	return m_controls->read();
}

/*
   The byte written to E8h is held by a latch on the display board side and
   drives two HCF4511BE decoders: bits 3-6 are the BCD digit, bit 2 is the LE
   of one of them and bit 7 the LE of the other.  A 4511 is transparent while
   LE is low and freezes the digit when LE goes high, so the digit is captured
   on the rising edge of either strobe.

   TODO: which strobe is which digit has not been checked, and the board has a
   third digit that these two decoders cannot account for.
*/
void falgasm87_state::digit_w(u8 data)
{
	// CD4511 pattern, including its tail-less 6 and 9; 10-15 blank the digit
	static constexpr u8 PATTERNS[16] =
	{
		0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07,
		0x7f, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	const u8 digit = (data >> 3) & 0x0f;
	if (BIT(data, 2) && !BIT(m_outlatch[2], 2))
		m_digits[0] = PATTERNS[digit];
	if (BIT(data, 7) && !BIT(m_outlatch[2], 7))
		m_digits[1] = PATTERNS[digit];
}

template <unsigned N>
void falgasm87_state::outlatch_w(u8 data)
{
	if constexpr (N == 2)
		digit_w(data);

	// TODO: which of the 32 latched bits is the motor, the lamps, the coin
	// counter and so on is still unidentified
	if (m_outlatch[N] != data)
		logerror("%s: out latch %u = %02x\n", machine().describe_context(), N, data);
	m_outlatch[N] = data;
}

void falgasm87_state::sod_w(int state)
{
	if (m_sod != state)
		logerror("%s: SOD = %d\n", machine().describe_context(), state);
	m_sod = state;
	m_leds[0] = state; // motor on
}

/*
   The knob on the riser PCB is a potentiometer that sets the frequency of the
   astable feeding RST 7.5, and the program uses that interrupt as the time
   base for the length of the ride; nothing reads the knob through a port.
   The knob is graduated in seconds, from 60 to 150, and the ride lasts
   [801B] * [801C] ticks: 8 * 255 = 2040 with the prescaler the COIN1 path
   forces.  That calibrates the whole range, 34.0 Hz at the 60 second end down
   to 13.6 Hz at the 150 second end, which is what the mapping below does.  A
   152.9 second recording of what looks like one complete ride sits right at
   the slow end of the scale.  Note that a credit taken through COIN2 uses the
   shorter prescaler from the coinage table, so the ride then comes out shorter
   than the dial reads.
*/
attotime falgasm87_state::dial_period() const
{
	// 0 on the adjuster is the 60 second mark of the dial, 100 is the 150
	const double seconds = 60.0 + (m_dial->read() & 0xff) * 0.9;
	return attotime::from_hz(2040.0 / seconds);
}

/*
   TRAP on the 8085 is level as well as edge triggered: MAME arms it on the
   rising edge and disarms it again if the line goes low before it has been
   serviced, which is why a zero width pulse is rejected outright.  The astable
   feeds it a square wave, so that is what is generated here: the timer runs at
   twice the interrupt rate and toggles the line, leaving it asserted for half
   a period.
*/
TIMER_DEVICE_CALLBACK_MEMBER(falgasm87_state::trap_tick)
{
	m_trap = !m_trap;
	m_maincpu->set_input_line(I8085_TRAP_LINE, m_trap ? ASSERT_LINE : CLEAR_LINE);
}

TIMER_DEVICE_CALLBACK_MEMBER(falgasm87_state::ride_tick)
{
	m_maincpu->pulse_input_line(I8085_RST75_LINE, attotime::zero);

	// re-arm with the current knob position so it can be turned while running
	m_ridetimer->adjust(dial_period());
}


INPUT_PORTS_START(falgasm89)
	PORT_START("IN0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("IN1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("IN2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("IN3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

/*
   Everything the program can read comes in through AY-3-8910 port B
   (register 15, input).  Bits 0-1 are the first two switches of the SELECTOR
   bank and pick one of four three-byte parameter sets stored right at the
   beginning of the EPROM (offsets 10h, 13h, 16h and 19h):

	 byte 0   credits given by one COIN2 pulse
	 byte 1   COIN1 pulses needed for one credit
	 byte 2   RST 7.5 ticks per unit of the ride counter, i.e. ride length

   The ride lengths are the same in both revisions (FFh, D4h, BBh and A1h,
   quoted below as a percentage of the longest one) but the prices are not:
   the 1987 program is roughly twice as generous as the 1988 one.

   The other two switches of the bank are not read by either revision, and the
   remaining bits of port B read back high.
*/
INPUT_PORTS_START(fantcar87)
	PORT_START("IN0") // AY-3-8910 port B
	PORT_DIPNAME(0x03, 0x03, "Coinage / Ride Length") PORT_DIPLOCATION("SELECTOR:1,2")
	PORT_DIPSETTING(   0x00, "1 pulse/credit, 3 credits per COIN2, 100% time")
	PORT_DIPSETTING(   0x01, "2 pulses/credit, 3 credits per COIN2, 83% time")
	PORT_DIPSETTING(   0x02, "1 pulse/credit, 2 credits per COIN2, 73% time")
	PORT_DIPSETTING(   0x03, "2 pulses/credit, 2 credits per COIN2, 63% time")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_START1) // edge triggered: starts the ride
	PORT_DIPNAME(0x20, 0x20, DEF_STR(Unused)) PORT_DIPLOCATION("SELECTOR:3")
	PORT_DIPSETTING(   0x20, DEF_STR(Off))
	PORT_DIPSETTING(   0x00, DEF_STR(On))
	PORT_DIPNAME(0x40, 0x40, DEF_STR(Unused)) PORT_DIPLOCATION("SELECTOR:4")
	PORT_DIPSETTING(   0x40, DEF_STR(Off))
	PORT_DIPSETTING(   0x00, DEF_STR(On))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("IN1") // read at F8h, only while a ride is running
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("DIAL") // TIMER knob on the panel, sets the RST 7.5 frequency
	PORT_ADJUSTER(50, "Timer Knob (60-150 s ride)")
INPUT_PORTS_END

INPUT_PORTS_START(fantcar87a)
	PORT_INCLUDE(fantcar87)

	PORT_MODIFY("IN0")
	PORT_DIPNAME(0x03, 0x03, "Coinage / Ride Length") PORT_DIPLOCATION("SELECTOR:1,2")
	PORT_DIPSETTING(   0x00, "1 pulse/credit, 5 credits per COIN2, 100% time")
	PORT_DIPSETTING(   0x01, "1 pulse/credit, 6 credits per COIN2, 83% time")
	PORT_DIPSETTING(   0x02, "2 pulses/credit, 3 credits per COIN2, 73% time")
	PORT_DIPSETTING(   0x03, "2 pulses/credit, 4 credits per COIN2, 63% time")
INPUT_PORTS_END


// The "simple" PCB has the i8255 socket empty
void falgasm89_state::falgasm89_simple(machine_config &config)
{
	I8085A(config, m_maincpu, 6_MHz_XTAL); // OKI M80C85A-2
	m_maincpu->set_addrmap(AS_PROGRAM, &falgasm89_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &falgasm89_state::io_map);

	SPEAKER(config, "mono").front_center();

	ay8910_device &psg(AY8910(config, "psg", 6_MHz_XTAL / 4)); // divider unknown
	psg.add_route(ALL_OUTPUTS, "mono", 0.50);
	psg.port_a_write_callback().set(FUNC(falgasm89_state::psg_pa_w));
	psg.port_b_read_callback().set(FUNC(falgasm89_state::psg_pb_r));
}

void falgasm89_state::falgasm89(machine_config &config)
{
	falgasm89_simple(config);

	I8255(config, "i8255"); // NEC D71055C
}

// Falgas Micro-87 hardware
void falgasm87_state::falgasm87(machine_config &config)
{
	I8085A(config, m_maincpu, 4_MHz_XTAL); // NEC D8085AHC
	m_maincpu->set_addrmap(AS_PROGRAM, &falgasm87_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &falgasm87_state::io_map);
	m_maincpu->out_sod_func().set(FUNC(falgasm87_state::sod_w));
	// TRAP, see the notes above; the timer toggles, so it runs at twice the rate
	TIMER(config, m_traptimer).configure_periodic(FUNC(falgasm87_state::trap_tick), attotime::from_hz(4800 * 2));

	// RST 7.5, adjustable through the TIMER knob on the panel.  This one is
	// edge triggered and latched inside the CPU until SIM clears it, so a bare
	// pulse is enough and the duty cycle does not matter.
	TIMER(config, m_ridetimer).configure_generic(FUNC(falgasm87_state::ride_tick));

	// RC monostable on the CD4093BE; the period is a guess, the main loop
	// strobes it several times per iteration
	WATCHDOG_TIMER(config, "watchdog").set_time(attotime::from_msec(500));

	SPEAKER(config, "mono").front_center();

	// Clocked by the 8085 CLK OUT pin (XTAL/2).  Confirmed against recordings
	// of a real machine: fitting the 69 entry period table against 3830 tones
	// measured from three recordings, over a blind 0.85-2.45 MHz search, lands
	// on 2.0004 MHz with a mean error of 6 cents.
	AY8910(config, m_psg, 4_MHz_XTAL / 2);
	m_psg->add_route(ALL_OUTPUTS, "mono", 0.50);
	m_psg->port_a_write_callback().set(FUNC(falgasm87_state::leds_w));
	m_psg->port_b_read_callback().set_ioport("IN0");
}

void falgasm89_video_state::falgasm89_video(machine_config &config)
{
	falgasm89(config);

	m_maincpu->set_addrmap(AS_IO, &falgasm89_video_state::main_io_map);

	I8085A(config, m_videocpu, 6_MHz_XTAL); // OKI M80C85A-2
	m_videocpu->set_addrmap(AS_PROGRAM, &falgasm89_video_state::video_mem_map);
	m_videocpu->set_addrmap(AS_IO, &falgasm89_video_state::video_io_map);

	tms9129_device &vdp(TMS9129(config, "vdp", 10.738635_MHz_XTAL));
	vdp.set_screen("screen");
	vdp.set_vram_size(0x10000); // 2 x UD61464DC
	SCREEN(config, "screen");

	i8155_device &i8155(I8155(config, "i8155", 6_MHz_XTAL)); // NEC D8155HC
	i8155.in_pa_callback().set([this] () { logerror("from main (i8155 PA in)\n"); return 0x00; }); // TODO: from main? returning rand() shows inputs come from here, probably sent from the main CPU
	i8155.out_pb_callback().set([this] (u8 data) { logerror("to main (i8155 PB out): %02x\n", data); }); // TODO: to main? bit 7 toggles continuously
	// other ports seem unused
	i8155.out_to_callback().set_inputline(m_videocpu, I8085_TRAP_LINE);
	i8155.out_to_callback().append_inputline("maincpu", I8085_TRAP_LINE); // TODO: wrong
}

// Falgas M89-N main PCB.
ROM_START(cbully)
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("bully-gs_m89-iv_16-1-91.u2", 0x0000, 0x8000, CRC(4cc85230) SHA1(c3851e6610bcb3427f81ecfcd4575603a9edca6e)) // 27C256

	ROM_REGION(0x22e, "plds", 0)
	ROM_LOAD("palce16v8_m894-bt.u11", 0x000, 0x117, NO_DUMP) // Protected
	ROM_LOAD("palce16v8_m894-a.u10",  0x117, 0x117, NO_DUMP) // Protected
ROM_END

// Falgas M89-E5 main PCB
ROM_START(fantcar)
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("cochefantastico89.p25", 0x0000, 0x8000, CRC(884a9768) SHA1(6f36a63312ae1f6899d26ca6953f942ddd860742))

	ROM_REGION(0x22e, "plds", 0)
	ROM_LOAD("pal.u11", 0x000, 0x117, NO_DUMP)
	ROM_LOAD("pal.u10", 0x117, 0x117, NO_DUMP)
ROM_END

/* First version of "Fantastic Car" runs on Falgas Micro-87 hardware. It was developed by Gaelco, but distributed and sold by Falgas.
   The Micro-87 hardware is older than M89, but shares the main components and architecture (without 8255).

  The riser PCB contains:
   -5 LEDs (motor on, coin input, sound, timer, light).
   -Knob for timer configuration.
   -Bank of 4 dipswitches for coinage configuration.
   -Volume knob.
*/
ROM_START(fantcar87)
	ROM_REGION(0x8000, "maincpu", 0)
	ROM_LOAD("mikel_es_m87e_15_2_88.u2",  0x0000, 0x8000, CRC(e1db4836) SHA1(7020bde14ee9afae41691beb0708ed50ca3506d3)) // NM27C256Q
ROM_END

ROM_START(fantcar87a)
	ROM_REGION(0x8000, "maincpu", 0)
	ROM_LOAD("mikel_micro_87_25-3-87.u2", 0x0000, 0x8000, CRC(83a16ff4) SHA1(52a1fcd89882fd00c1f46328d75c2623f6f2f83e))
ROM_END


// Falgas M89-E5 main PCB with 25291 video PCB. Bootleg of Konami's Hyper Rally for MSX
ROM_START(rmontecarlo)
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("uj_504_m-89e5_17-7-91.u2", 0x00000, 0x10000, CRC(ff1be338) SHA1(9a3f4760bd7e4d9328d44e546bb588561fc53016)) // 27C512

	ROM_REGION(0x10000, "videocpu", 0)
	ROM_LOAD("uj_v10_22-5-91.bin",       0x00000, 0x10000, CRC(8ac21706) SHA1(bd399136d4793c1eaa49c2d5a35022864e771833)) // 27C512

	ROM_REGION(0x345, "plds", 0)
	ROM_LOAD("palce16v8_m894-bt.u11", 0x000, 0x117, NO_DUMP) // Protected, on M89 PCB
	ROM_LOAD("palce16v8_m894-a.u10",  0x117, 0x117, NO_DUMP) // Protected, on M89 PCB
	ROM_LOAD("palce16v8_video91.bin", 0x22e, 0x117, NO_DUMP) // Protected, on video PCB
ROM_END

} // anonymous namespace

//    YEAR  NAME         PARENT   MACHINE           INPUT      CLASS                  INIT        ROT   COMPANY   FULLNAME                                    FLAGS
GAME( 1991, cbully,      0,       falgasm89_simple, falgasm89, falgasm89_state,       empty_init, ROT0, "Falgas", "Coche Bully",                              MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
GAME( 19??, fantcar,     0,       falgasm89,        falgasm89, falgasm89_state,       empty_init, ROT0, "Falgas", "Fantastic Car (M89 hardware)",             MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
GAMEL(1988, fantcar87,   fantcar, falgasm87,        fantcar87, falgasm87_state,       empty_init, ROT0, "Falgas", "Fantastic Car (Micro-87 hardware, newer)", MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_SUPPORTS_SAVE, layout_falgas_m87 )
GAMEL(1987, fantcar87a,  fantcar, falgasm87,        fantcar87a,falgasm87_state,       empty_init, ROT0, "Falgas", "Fantastic Car (Micro-87 hardware, older)", MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_SUPPORTS_SAVE, layout_falgas_m87 )
GAME( 1991, rmontecarlo, 0,       falgasm89_video,  falgasm89, falgasm89_video_state, empty_init, ROT0, "Falgas", "Rally Montecarlo",                         MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK )
