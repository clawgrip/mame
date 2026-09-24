// license:BSD-3-Clause
// copyright-holders:
/********************************************************************************

 Driver for MCS51-based crane coinops from Compumatic.
 The same PCB was used on machines from different manufacturers, like OM Vending
 and Covielsa.

 Different hardware revisions from Compumatic, called "GANCHONEW" PCB.
 From V2 to V8 hardware with the following layout (minor changes, like the power
 supply connector, moving from PC AT to PC ATX):

 COMPUMATIC "GANCHONEW V8" CPU
							  CN7 COUNTERS
  ______________________________________
 |     ______  ______  ______  ········ |
 |     ST8251  ST8251  ST8251           |
 |      IC16    IC15    IC14            |
 | __________    ____                   |
 | ULN2803APG    LM358N                 |
 |   IC12         IC10                  |
 | __________    __________     __ __  _|_
 | SN74HC273N    SN74HC244N    | || | |   |
 |   IC6           IC7         | || | | C |
 | __________    __________    |F2 F1| O |
 | SN74HC273N    SN74HC244N    | || | | N |
 |   IC11          IC8         |_||_| | N |
 | __________    __________           |___|
 | SN74HC373N    |_GAL16V8_|            |
 |   IC2           IC4                  |
 | ________________    ____     ____  oo|
 || W29C020C  IC3 | TL7705ACP         oo|
 ||_______________|   IC9             oo|<- ATX Power
 |                    24C16  IC5      oo|   Supply conn
 | ___________________   XT1          oo|
 || TS80C32X2-MCA    |   12MHz  TEST  oo|
 ||_______IC1________|           SW1  oo|
 | ....  ...... ..  ..... .......       |
 |______________________________________|
  CN6     CN5    CN4  CN3     CN2
  + JP1
  DISPLAY SENSOR SPK  SELECT  JOYSTICK

 The MCU on the older PCBs can differ between 80C32 compatible models (found
 with a Winbond W78C32C-40 and with a TS80C32X2-MCA).

 The "GANCHONEW-V2 COMP" board Octopussy runs on has the same part numbering,
 an AT PSU connector instead of the ATX one and the motor connector (CN8) on
 the opposite edge.  SW1 is a slide switch on both, so the firmware staying in
 the menu while it is on is the expected behaviour.  The edge connectors are
 the ones the OM Vending clone silkscreens by name: CN2 joystick (5 ways),
 CN3 coin selector (7), CN4 speaker (2), CN5 sensors (7), CN6 display (5) and
 CN7 counters (12).

 One 74HC273 drives the resistor ladder of the DAC, the other one the ULN2803
 that feeds the counters, the lamps and the prize coil, and the two 74HC244
 read the inputs through 10K pull-up arrays, so an idle input reads high.

 "GANCHONEW/CPU-V1 COMP" PCB has a different layout, with the connectors on
 the side instead of the front edge:
			  __________
  ___________|   CN1    |_____CN8__________
 |           |_________| |||||||||||||  .|
 |:                ________   _______   :| CN2
 |:   ____        TD62783AP  HD74HC244P  |
 |:   BUZ12        ________   _______   :| CN3
 |:               TD62083AP  74HCT273N  :|
 |  ____________   ________   _______    | CN4
 | | EPROM  IC3|  HD74HC373P HD74HC244P :|
 | |___________|   ________   _______    | CN5
 |                PALCE16V8H HD74HC273P :|
 |  _______________    ____   _______   :| CN6
 | | TSC80C31-12CA|  24LC16B TD62083AP   | + JP1
 | |____IC1_______|    IC5      IC12    :|
 |     CN7           XT1 12MHz    P1    :|
 |_______________________________________|

 Its part numbering matches the later boards and JP1, a three pin header here,
 again sits next to the display connector.  It carries no DAC and no op-amp at
 all, the motors and the claw magnet hanging from the Darlington arrays.

 The OM Vending clone is silkscreened "CPU GRUA V2  O. M. VENDING":

  _______________________________________________________
 |  ___     ___     ___        ______________            |
 | |IC13|  |IC15|  |IC14|     |__CN7________|  CONTADORES|
 | (Multiwatt-15 power devices) D8..D21 (flyback diodes) _|_
 | P1  R1 R2  ____                    __________        |   |
 |[#] o-o-o  |IC10|  JP1 JP2 JP3     |_SN74HC32N| IC16  | C |
 |  ___________ AR5                  ____    _______    | N |
 | |SN74HC273N| IC6      F1 (BOBINA 3A)  |  |ULN2803|   | 8 |
 |  _______________              _______________ IC12   |___|
 | |SST 39SF040   | IC3         |24C16WP| IC5    ______ MOTOR
 | |______________|              _______________ IC11  |
 |  ___________   _________     |TLC7705| IC9    ______ |
 | |SN74HC373N|  |ATF16V8B |     AR2 AR3         IC7    |
 |  IC2  AR1     |_IC4_____|                     ______ |
 |  _________________________   XT1 12MHz        IC8    |
 | |AT89S52 24PU            |                   ____    |
 | |____IC1__________________|                  SW1 TEST|
 |  [CN6]   [CN5]      [CN4] [CN3]        [CN2]         |
 |_______________________________________________________|
   DISPLAY SENSOR+V.RET. ALTAVOZ SELECTOR    JOYSTICK
	5 pins    7 pins     2 pins   7 pins

 On that board the flash /OE is the wired-OR of /PSEN and /RD (D2 and D3 next
 to the GAL), so code and samples come from the same device, and IC16 (a quad
 OR gate) drives the enable input of each motor bridge from its two direction
 bits, hence both bits low = coast and both high = brake.  JP1 and JP2 route
 the motor supply (CN7.9 / CN7.6 or CN8) and JP3 sets the coin selector input
 type ("C.A." open collector or TTL), which matches the two coin reading modes
 the firmware supports.

 --------------------------------------------------------------------------
 Hardware notes, from the disassembly of the six dumped program ROMs:

 The 80C31/80C32 runs from the external EPROM (HC373 address latch), of which
 only the first 64 KBytes are used for code.  The rest of the EPROM holds the
 sound samples, read with MOVX while one of the bank lines decoded by the GAL
 (driven by CPU port pins) is asserted.  While a bank line is active the GAL
 also disables the input buffers, so the whole 64 KBytes window reads from the
 EPROM; the power-on checksum relies on that, as it adds up the whole EPROM,
 code and samples, and expects zero ("EPro" is shown otherwise).  The OM Vending
 clone has no checksum, it just checks the sample table.

 Sound samples are 8 bit unsigned PCM terminated by a zero byte; the timer 0
 interrupt (6.67 kHz) mixes two of them and writes the result to the DAC.

 MOVX map (with all the bank lines inactive):
   R  8000h  74HC244, sensors, limit switches and coin selector
   R  8001h  74HC244, joystick, play button, test switch and alarm
   W  A000h  74HC273, 8 bit R-2R DAC (V2 and later) / motor latch (V1)
   W  A001h  74HC273, lamps, counters and token hopper

 Port usage on the "GANCHONEW V2" to "V8" boards:
   P1.0      24C16 SCL
   P1.1      24C16 SDA
   P1.2      gantry motor, towards the back
   P1.3      gantry motor, towards the front
   P1.4      trolley motor, towards the right
   P1.5      trolley motor, towards the left
   P1.6      winch motor, claw down
   P1.7      winch motor, claw up (both bits of a motor set = brake)
   P3.0      display data
   P3.1      display clock
   P3.2      coin selector line 1 (INT0, polled)
   P3.3      coin selector line 2 (INT1, polled)
   P3.4      claw magnet, PWMed with a 16 step pattern to set the claw strength
   P3.5      EPROM A16 (active low)

 The V2+ boards have a single input for both limit switches of each horizontal
 axis (the firmware remembers the direction it was moving), plus the claw up
 and claw down switches.  The power-on self test drives every motor until its
 limit switch closes and shows an error ("F Fr", "F  I", "F do", "F uP"...)
 when one doesn't.

 The "GANCHONEW" (V1) board has no DAC, sound being a square wave generated by
 toggling P3.4 on the timer 0 interrupt.  There the motors and the claw magnet
 are driven by the A000h latch (same bit order as P1.2-P1.7 above, bit 6 claw
 magnet), each limit switch has its own input, the 24C16 is also on P1.0/P1.1,
 P1.2 is the latch strobe of one of the two supported display boards, P1.3
 reads the alarm sensor, P1.4 the display board type and P1.7 seems to be the
 EPROM A16 line.  Its coin selector has a single line, on bit 5 of the 8001h
 port.  Its power-on checksum reads the input ports too, so it only passes
 with all the inputs idle (no limit switch closed).

 The OM Vending clone ("CPU GRUA V2") has a 512 KBytes flash ROM, so it uses
 three bank lines (P3.3-P3.5, all active low), and moves the claw magnet PWM
 to bit 7 of the A001h latch (which drives the fused "BOBINA 3A" output
 through IC12).

 Display: the four digits are not multiplexed, the 32 segment lines are driven
 by a serial LED driver on the "Plumadig" board.  The firmware supports two
 different display boards, selected by JP1 on bit 5 of the 8001h input port
 (on P1.4 on the V1 board), and carries a different segment table for each:
  - bit 5 low: 36 clock frames, MM5450 style driver, segments active high.  The
	32 data bits are followed by 0,0,0,1, that trailing '1' being the start bit
	of the next frame, so each frame latches the data sent on the previous one.
	This is the one emulated here, and the one JP1 selects on every board seen.
  - bit 5 high: four dummy clocks with data low followed by the 32 bits shifted
	out by the MCS51 serial port in mode 0 (plus a latch strobe on P1.2 on the
	V1 board), segments active low, shift register board.  Not emulated, as the
	MCS51 core doesn't emulate the mode 0 output timings on the port pins.

 Both tables hold one byte per digit, first byte sent = leftmost digit:
	MM5450 board:      bit 0 a, 1 f, 2 g, 3 e, 4 d, 5 dp, 6 c, 7 b (active high)
	shift register one: bit 0 g, 1 f, 2 a, 3 b, 4 e, 5 d, 6 c, 7 dp (active low)

 The 24C16 must hold the machine type code at address 1 (and a valid BCD value
 at address 2) or the firmware hangs on purpose: that code is factory
 programmed and never rewritten by the game, while all the other settings are
 rebuilt by the machine itself when their checksums fail ("cLE" is shown on the
 display while doing so).  No SEEPROM has been dumped, so the ones loaded here
 are hand built: those two bytes plus the defaults each machine writes when it
 initializes a SEEPROM holding just them.

 The crane itself is simulated just enough for the self test and the game
 cycle to work: each motor moves its axis at a constant speed and the limit
 switches close at the end of the travel.  Prizes aren't simulated, the prize
 sensor is a regular input.

 TODO:
  - Emulate the shift register display board (needs the MCS51 serial port
	mode 0 output emulated on the port pins).
  - Dump a real SEEPROM and the V1 PLD.

********************************************************************************/

#include "emu.h"

#include "cpu/mcs51/i80c51.h"
#include "cpu/mcs51/i80c52.h"
#include "machine/i2cmem.h"
#include "sound/dac.h"
#include "sound/spkrdev.h"

#include "speaker.h"

#include <algorithm>

#include "compucranes.lh"


namespace
{

class compucranes_state : public driver_device
{
public:
	compucranes_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_i2cmem(*this, "i2cmem")
		, m_dac(*this, "dac")
		, m_speaker(*this, "speaker")
		, m_rom(*this, "maincpu")
		, m_inputs(*this, "IN%u", 0U)
		, m_conf(*this, "CONF")
		, m_digits(*this, "digit%u", 0U)
		, m_outputs(*this, "out%u", 0U)
		, m_motors(*this, "motor%u", 0U)
		, m_claw(*this, "claw")
		, m_crane_fb(*this, "crane_fb")
		, m_crane_lr(*this, "crane_lr")
		, m_crane_z(*this, "crane_z")
	{
	}

	ioport_value limits_r();
	ioport_value limits_v1_r();

	void ganchonew(machine_config &config) ATTR_COLD;
	void ganchonew_v1(machine_config &config) ATTR_COLD;
	void toyshop(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	required_device<mcs51_cpu_device> m_maincpu;
	required_device<i2cmem_device> m_i2cmem;
	optional_device<dac_8bit_r2r_device> m_dac;
	optional_device<speaker_sound_device> m_speaker;
	required_region_ptr<u8> m_rom;
	required_ioport_array<2> m_inputs;
	optional_ioport m_conf;
	output_finder<4> m_digits;
	output_finder<8> m_outputs;
	output_finder<6> m_motors;
	output_finder<> m_claw;
	output_finder<> m_crane_fb;
	output_finder<> m_crane_lr;
	output_finder<> m_crane_z;

	void common(machine_config &config) ATTR_COLD;

	void program_map(address_map &map) ATTR_COLD;
	void ext_map(address_map &map) ATTR_COLD;
	void ext_v1_map(address_map &map) ATTR_COLD;

	u8 ext_r(offs_t offset);
	u8 ext_v1_r(offs_t offset);
	void ext_w(offs_t offset, u8 data);

	u8 p1_r();
	u8 p1_v1_r();
	void p1_w(u8 data);
	void p1_v1_w(u8 data);
	void p3_w(u8 data);
	void p3_v1_w(u8 data);
	void p3_toyshop_w(u8 data);

	void motors_w(u8 data);
	void outputs_w(u8 data);
	void display_w(u8 data);
	void set_motors(u8 data);
	void mech_update();

	u32 m_bank = 0;
	u64 m_shifter = 0;
	bool m_disp_clk = false;
	u8 m_p3 = 0xff;

	// crane mechanics: 0 = front/back (0.0 = front), 1 = left/right (0.0 = left),
	// 2 = claw (0.0 = up); the crane starts wherever it was left, not at home
	double m_pos[3] = { 0.5, 0.5, 0.1 };
	u8 m_motor_state = 0;
	attotime m_mech_time;
};


void compucranes_state::machine_start()
{
	m_mech_time = machine().time();

	save_item(NAME(m_pos));
	save_item(NAME(m_motor_state));
	save_item(NAME(m_mech_time));
	save_item(NAME(m_bank));
	save_item(NAME(m_shifter));
	save_item(NAME(m_disp_clk));
	save_item(NAME(m_p3));
}

void compucranes_state::machine_reset()
{
	m_bank = 0;
	m_shifter = 0;
	m_disp_clk = false;
	m_p3 = 0xff;

	mech_update();
}


/********************************************************************************
	Memory maps
********************************************************************************/

void compucranes_state::program_map(address_map &map)
{
	map(0x0000, 0xffff).rom().region("maincpu", 0);
}

void compucranes_state::ext_map(address_map &map)
{
	map(0x0000, 0xffff).rw(FUNC(compucranes_state::ext_r), FUNC(compucranes_state::ext_w));
}

void compucranes_state::ext_v1_map(address_map &map)
{
	map(0x0000, 0xffff).rw(FUNC(compucranes_state::ext_v1_r), FUNC(compucranes_state::ext_w));
}

u8 compucranes_state::ext_r(offs_t offset)
{
	// the input buffers are only enabled while all the bank lines are inactive
	if (m_bank == 0)
	{
		switch (offset)
		{
		case 0x8000: return m_inputs[0]->read();
		case 0x8001: return m_inputs[1]->read();
		}
	}

	return m_rom[((m_bank << 16) | offset) & (m_rom.bytes() - 1)];
}

u8 compucranes_state::ext_v1_r(offs_t offset)
{
	switch (offset)
	{
	case 0x8000: return m_inputs[0]->read();
	case 0x8001: return m_inputs[1]->read();
	}

	return m_rom[((m_bank << 16) | offset) & (m_rom.bytes() - 1)];
}

void compucranes_state::ext_w(offs_t offset, u8 data)
{
	switch (offset)
	{
	case 0xa000: // 74HC273
		if (m_dac.found())
			m_dac->write(data); // R-2R ladder and LM358 buffer
		else
			motors_w(data);     // V1 board
		break;

	case 0xa001: // 74HC273
		outputs_w(data);
		break;
	}
}


/********************************************************************************
	Crane mechanics simulation
********************************************************************************/

void compucranes_state::mech_update()
{
	// full travel takes 3 seconds on the horizontal axes, 2 seconds for the claw
	static constexpr double SPEED[3] = { 1.0 / 3.0, 1.0 / 3.0, 1.0 / 2.0 };

	attotime const now = machine().time();
	double const elapsed = (now - m_mech_time).as_double();
	m_mech_time = now;

	for (int axis = 0; axis < 3; axis++)
	{
		// motor bits: back/front, right/left, down/up (both set = brake)
		int const dir = BIT(m_motor_state, axis * 2) - BIT(m_motor_state, axis * 2 + 1);
		m_pos[axis] = std::clamp(m_pos[axis] + dir * SPEED[axis] * elapsed, 0.0, 1.0);
	}

	m_crane_fb = int(m_pos[0] * 100.0 + 0.5);
	m_crane_lr = int(m_pos[1] * 100.0 + 0.5);
	m_crane_z = int(m_pos[2] * 100.0 + 0.5);
}

void compucranes_state::set_motors(u8 data)
{
	// bit 0 back, 1 front, 2 right, 3 left, 4 claw down, 5 claw up
	mech_update();
	m_motor_state = data & 0x3f;

	for (int i = 0; i < 6; i++)
		m_motors[i] = BIT(data, i);
}

ioport_value compucranes_state::limits_r()
{
	// V2+ boards: claw up, claw down, both left/right ends, both front/back ends
	mech_update();
	return
			((m_pos[2] <= 0.0) ? 0 : 0x01) |
			((m_pos[2] >= 1.0) ? 0 : 0x02) |
			((m_pos[1] <= 0.0 || m_pos[1] >= 1.0) ? 0 : 0x04) |
			((m_pos[0] <= 0.0 || m_pos[0] >= 1.0) ? 0 : 0x08);
}

ioport_value compucranes_state::limits_v1_r()
{
	// V1 board: back, front, right, left, claw down, claw up
	mech_update();
	return
			((m_pos[0] >= 1.0) ? 0 : 0x01) |
			((m_pos[0] <= 0.0) ? 0 : 0x02) |
			((m_pos[1] >= 1.0) ? 0 : 0x04) |
			((m_pos[1] <= 0.0) ? 0 : 0x08) |
			((m_pos[2] >= 1.0) ? 0 : 0x10) |
			((m_pos[2] <= 0.0) ? 0 : 0x20);
}


/********************************************************************************
	I/O
********************************************************************************/

void compucranes_state::motors_w(u8 data)
{
	// V1 board: motors on bits 0-5 (same order as P1.2-P1.7 on the later
	// boards), bit 6 = claw magnet, bit 7 = unknown
	set_motors(data);

	m_claw = BIT(data, 6);
}

void compucranes_state::outputs_w(u8 data)
{
	// lamps, electromechanical counters and token hopper
	for (int i = 0; i < 8; i++)
		m_outputs[i] = BIT(data, i);

	machine().bookkeeping().coin_counter_w(0, BIT(data, 0));
}

void compucranes_state::display_w(u8 data)
{
	// P3.0 = data, P3.1 = clock
	bool const clk = BIT(data, 1);

	if (clk && !m_disp_clk)
	{
		m_shifter = (m_shifter << 1) | BIT(data, 0);

		// MM5450 type driver: the start bit reaching the end of the 36 bit
		// shift register latches the 35 data bits following it
		if (BIT(m_shifter, 35))
		{
			for (int digit = 0; digit < 4; digit++)
			{
				// bits as sent: a f g e d dp c b (MSB of the byte = first bit sent)
				m_digits[digit] = bitswap<8>(u8(m_shifter >> (27 - 8 * digit)), 2, 5, 6, 4, 3, 1, 0, 7);
			}
			m_shifter = 0;
		}
	}

	m_disp_clk = clk;
}

u8 compucranes_state::p1_r()
{
	return 0xfd | (m_i2cmem->read_sda() << 1);
}

u8 compucranes_state::p1_v1_r()
{
	// P1.3 reads the alarm sensor and P1.4 the display board type
	return 0xe5 | (m_i2cmem->read_sda() << 1) | (m_conf->read() & 0x18);
}

void compucranes_state::p1_w(u8 data)
{
	m_i2cmem->write_scl(BIT(data, 0));
	m_i2cmem->write_sda(BIT(data, 1));

	// gantry, trolley and winch motors
	set_motors(data >> 2);
}

void compucranes_state::p1_v1_w(u8 data)
{
	m_i2cmem->write_scl(BIT(data, 0));
	m_i2cmem->write_sda(BIT(data, 1));

	m_bank = BIT(data, 7); // EPROM A16
}

void compucranes_state::p3_w(u8 data)
{
	display_w(data);

	m_bank = BIT(~data, 5); // EPROM A16
	m_claw = BIT(data, 4);  // claw magnet PWM

	m_p3 = data;
}

void compucranes_state::p3_v1_w(u8 data)
{
	display_w(data);

	if (BIT(data ^ m_p3, 4))
		m_speaker->level_w(BIT(data, 4)); // square wave sound

	m_p3 = data;
}

void compucranes_state::p3_toyshop_w(u8 data)
{
	display_w(data);

	m_bank = bitswap<3>(u8(~data), 5, 4, 3); // EPROM A16-A18

	m_p3 = data;
}


/********************************************************************************
	Inputs
********************************************************************************/

static INPUT_PORTS_START(ganchonew)
	PORT_START("IN0") // 74HC244 read at 8000h
	PORT_BIT(0x0f, IP_ACTIVE_HIGH, IPT_CUSTOM) PORT_CUSTOM_MEMBER(FUNC(compucranes_state::limits_r)) // limit switches
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_COIN3)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_START1) // only used when not set to start automatically
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Prize Sensor")  PORT_CODE(KEYCODE_P)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Hopper Sensor") PORT_CODE(KEYCODE_H)

	PORT_START("IN1") // 74HC244 read at 8001h
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    // towards the back
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  // towards the front
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
	PORT_CONFNAME(0x20, 0x00, "JP1 - Display Board")
	PORT_CONFSETTING(   0x00, "Serial LED driver (MM5450 type)")
	PORT_CONFSETTING(   0x20, "Shift registers (not emulated)")
	PORT_SERVICE(0x40, IP_ACTIVE_LOW) // "TEST SW" on the PCB
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Alarm Sensor")  PORT_CODE(KEYCODE_A)

	PORT_START("COINS") // coin selector lines, polled on P3.2 and P3.3
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT(0xf3, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

static INPUT_PORTS_START(ganchonew_v1)
	PORT_START("IN0") // 74HC244 read at 8000h
	PORT_BIT(0x3f, IP_ACTIVE_HIGH, IPT_CUSTOM) PORT_CUSTOM_MEMBER(FUNC(compucranes_state::limits_v1_r)) // limit switches
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Prize Sensor")  PORT_CODE(KEYCODE_P)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Hopper Sensor") PORT_CODE(KEYCODE_H)

	PORT_START("IN1") // 74HC244 read at 8001h
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    // towards the back
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  // towards the front
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_SERVICE(0x40, IP_ACTIVE_LOW)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_START1) // only used when not set to start automatically

	PORT_START("COINS") // not used by this board
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("CONF") // read on P1
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Alarm Sensor")  PORT_CODE(KEYCODE_A)
	PORT_CONFNAME(0x10, 0x00, "Display Board")
	PORT_CONFSETTING(   0x00, "Serial LED driver (MM5450 type)")
	PORT_CONFSETTING(   0x10, "Shift registers (not emulated)")
	PORT_BIT(0xe7, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


/********************************************************************************
	Machine configs
********************************************************************************/

void compucranes_state::common(machine_config &config)
{
	m_maincpu->set_addrmap(AS_PROGRAM, &compucranes_state::program_map);
	m_maincpu->set_addrmap(AS_DATA, &compucranes_state::ext_map);
	m_maincpu->port_in_cb<1>().set(FUNC(compucranes_state::p1_r));
	m_maincpu->port_out_cb<1>().set(FUNC(compucranes_state::p1_w));
	m_maincpu->port_in_cb<3>().set_ioport("COINS");

	I2C_24C16(config, m_i2cmem);

	SPEAKER(config, "mono").front_center();
}

void compucranes_state::ganchonew(machine_config &config)
{
	I80C32(config, m_maincpu, 12_MHz_XTAL);

	common(config);

	m_maincpu->port_out_cb<3>().set(FUNC(compucranes_state::p3_w));

	DAC_8BIT_R2R(config, m_dac, 0).add_route(ALL_OUTPUTS, "mono", 0.5); // 74HC273 + resistor ladder + LM358
}

void compucranes_state::ganchonew_v1(machine_config &config)
{
	I80C31(config, m_maincpu, 12_MHz_XTAL);

	common(config);

	m_maincpu->set_addrmap(AS_DATA, &compucranes_state::ext_v1_map);
	m_maincpu->port_in_cb<1>().set(FUNC(compucranes_state::p1_v1_r));
	m_maincpu->port_out_cb<1>().set(FUNC(compucranes_state::p1_v1_w));
	m_maincpu->port_out_cb<3>().set(FUNC(compucranes_state::p3_v1_w));

	SPEAKER_SOUND(config, m_speaker).add_route(ALL_OUTPUTS, "mono", 0.50);
}

void compucranes_state::toyshop(machine_config &config)
{
	AT89S52(config, m_maincpu, 12_MHz_XTAL);

	common(config);

	m_maincpu->port_out_cb<3>().set(FUNC(compucranes_state::p3_toyshop_w));

	DAC_8BIT_R2R(config, m_dac, 0).add_route(ALL_OUTPUTS, "mono", 0.5);
}


/********************************************************************************
	ROM definitions
********************************************************************************/

// "GANCHONEW/CPU-V1 COMP" PCB. Temic TSC80C31-12CA CPU.
ROM_START(crsauruss)
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_LOAD("30.01.ic3",      0x00000, 0x20000, CRC(c735e024) SHA1(63dd3a71472bde7f9dead49a8dc889365fd024ef)) // 1xxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION(0x00117, "pld", 0)
	ROM_LOAD("palce16v8h.ic4", 0x00000, 0x00117, NO_DUMP) // AMD PALCE16V8H-25, its location couldn't be read on the pictures

	ROM_REGION(0x00800, "i2cmem", 0)
	ROM_LOAD("24lc16b.ic5",    0x00000, 0x00800, BAD_DUMP CRC(7213cbb9) SHA1(7417c83c5a5254f86f3d56529341ae8a254e8e53)) // hand built, see the notes at the top
ROM_END

// "GANCHONEW-V8" PCB with ATX PSU connector. TS80C32X2-MCA CPU.
ROM_START(mastcrane)
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_LOAD("v8_w29c020c.ic3", 0x00000, 0x40000, CRC(733dfcbc) SHA1(d18d7945e9b8f189f2169d3d90c3cfea97d3b39c)) // 1ST AND 2ND HALF IDENTICAL

	ROM_REGION(0x00117, "pld", 0)
	ROM_LOAD("gal16v8.ic4",     0x00000, 0x00117, CRC(4d665a06) SHA1(504f0107482f636cd216579e982c6162c0b120a7)) // Verified to be the same on all known PCB revisions

	ROM_REGION(0x00800, "i2cmem", 0)
	ROM_LOAD("24c16_v8.ic5",    0x00000, 0x00800, BAD_DUMP CRC(9b919023) SHA1(aafbabfc70f33e0a453c6bd9bec2c7127733fb15)) // hand built, see the notes at the top
ROM_END

// "GANCHONEW V7" PCB with AT PSU connector
ROM_START(mastcranea)
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_LOAD("v7.ic3",       0x00000, 0x40000, CRC(299c9ad1) SHA1(b0ba2ab588151dba89307e118ba061cad2b8116b)) // 1ST AND 2ND HALF IDENTICAL (W29C020C)

	ROM_REGION(0x00117, "pld", 0)
	ROM_LOAD("atf16v8.ic4",  0x00000, 0x00117, CRC(4d665a06) SHA1(504f0107482f636cd216579e982c6162c0b120a7)) // Verified to be the same on all known PCB revisions

	ROM_REGION(0x00800, "i2cmem", 0)
	ROM_LOAD("24c16_v7.ic5", 0x00000, 0x00800, BAD_DUMP CRC(eebe1da3) SHA1(472650d0884aff0b3d406c17bbca32af41468070)) // hand built, see the notes at the top
ROM_END

// "GANCHONEW V2" PCB with AT PSU connector. W78C32C-40 CPU.
ROM_START(mastcraneb)
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_LOAD("505.ic3",     0x00000, 0x20000, CRC(3dbb83f1) SHA1(3536762937332add0ca942283cc22ff301884a4a))

	ROM_REGION(0x00117, "pld", 0)
	ROM_LOAD("atf168b.ic4", 0x00000, 0x00117, CRC(4d665a06) SHA1(504f0107482f636cd216579e982c6162c0b120a7)) // Verified to be the same on all known PCB revisions

	ROM_REGION(0x00800, "i2cmem", 0)
	ROM_LOAD("24c16_v2.ic5", 0x00000, 0x00800, BAD_DUMP CRC(2d4ce67d) SHA1(77f2cd20f057dbfe5cd99e0eb7f14274781bd8ad)) // hand built, see the notes at the top
ROM_END

// "GANCHONEW-V2 COMP" PCB with AT PSU connector, machine number sticker "NºMAQ. 00-356  13/06/00"
ROM_START(octopussy)
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_LOAD("w29c011.ic3", 0x00000, 0x20000, CRC(47da93e8) SHA1(aa821dd22c1912ec2942ca6afd989d61df4387d7))

	ROM_REGION(0x00117, "pld", 0)
	ROM_LOAD("atf16v8.ic4", 0x00000, 0x00117, CRC(4d665a06) SHA1(504f0107482f636cd216579e982c6162c0b120a7))

	ROM_REGION(0x00800, "i2cmem", 0)
	ROM_LOAD("24c16.ic5",   0x00000, 0x00800, BAD_DUMP CRC(1c93e051) SHA1(e1cd62da24b049377d3103f1aae088d4581789c2)) // hand built, see the notes at the top
ROM_END

/* Direct clone of the GANCHONEW PCB by OM Vending, silkscreened "CPU GRUA V2  O. M. VENDING".
   The whole program, vectors included, is in the external flash, so the AT89S52 internal ROM is
   presumably disabled (EA tied low), but the pin hasn't been traced on the PCB. */
ROM_START(toyshop)
	ROM_REGION(0x10000, "internal", 0)
	ROM_LOAD("89s52.ic1",   0x00000, 0x10000, NO_DUMP) // 8 KBytes internal ROM

	ROM_REGION(0x80000, "maincpu", 0)
	ROM_LOAD("39sf040.ic3", 0x00000, 0x80000, CRC(0d9d157d) SHA1(e70f095d3524e3a4c8d5d07857bb2692b6260cc1))

	ROM_REGION(0x00117, "pld", 0)
	ROM_LOAD("atf16v8.ic4", 0x00000, 0x00117, NO_DUMP)

	ROM_REGION(0x00800, "i2cmem", 0)
	ROM_LOAD("24c16.ic5",   0x00000, 0x00800, BAD_DUMP CRC(ab4445d8) SHA1(5ee38c6ac64442b25e6707f492ac92d753ee111c)) // hand built, see the notes at the top
ROM_END

} // anonymous namespace

// Years and versions are the ones the programs show on the display (or store in the SEEPROM) at power on
//     YEAR  NAME        PARENT     MACHINE       INPUT         CLASS              INIT        ROT   COMPANY               FULLNAME                       FLAGS                                       LAYOUT
GAMEL( 2002, crsauruss,  0,         ganchonew_v1, ganchonew_v1, compucranes_state, empty_init, ROT0, "Recreativos Presas", "Cranesaurus Single (v30.01)", MACHINE_MECHANICAL | MACHINE_SUPPORTS_SAVE, layout_compucranes ) // 28/01/2002
GAMEL( 2012, mastcrane,  0,         ganchonew,    ganchonew,    compucranes_state, empty_init, ROT0, "Compumatic",         "Master Crane (v44.12)",       MACHINE_MECHANICAL | MACHINE_SUPPORTS_SAVE, layout_compucranes ) // 30/04/2012
GAMEL( 2016, mastcranea, mastcrane, ganchonew,    ganchonew,    compucranes_state, empty_init, ROT0, "Compumatic",         "Master Crane (v46.11)",       MACHINE_MECHANICAL | MACHINE_SUPPORTS_SAVE, layout_compucranes ) // 05/12/2016
GAMEL( 2001, mastcraneb, mastcrane, ganchonew,    ganchonew,    compucranes_state, empty_init, ROT0, "Compumatic",         "Master Crane (v05.05)",       MACHINE_MECHANICAL | MACHINE_SUPPORTS_SAVE, layout_compucranes ) // 16/10/2001
GAMEL( 2000, octopussy,  0,         ganchonew,    ganchonew,    compucranes_state, empty_init, ROT0, "Covielsa",           "Octopussy (v21.01)",          MACHINE_MECHANICAL | MACHINE_SUPPORTS_SAVE, layout_compucranes ) // 29/05/2000
GAMEL( 2016, toyshop,    0,         toyshop,      ganchonew,    compucranes_state, empty_init, ROT0, "OM Vending",         "Toy Shop (v17.01)",           MACHINE_MECHANICAL | MACHINE_SUPPORTS_SAVE, layout_compucranes ) // 09/12/2016
