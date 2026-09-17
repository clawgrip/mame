// license:BSD-3-Clause
// copyright-holders:
/*
  Gaelco 'Futbol-3' hardware for kiddie rides, pinballs, and electromechanicals.

  The PCB is very compact and has few components. The main ones are:

  PIC16C56 as main CPU (RC oscillator, no crystal)
  OKI M6295 for sound (44-pin QFP, no oscillator of its own)
  AMD Am27C020 sound ROM
  MC74HCT273A output latch driving an ULN2803A
  SN74LS365AN buffer for the DIP switches
  2x TLP504A optocouplers
  1 bank of 6 dips

  Gaelco FUTBOL-3 PCB
  _____________________________________________________
  |JP1  JP2         __________                        |
  | __   ___        |ULN2803A_|                       |
  || |  |  |        ___________                       |
  || |  |  |        |MC74HCT273A                      |
  ||_|  |  |        _________   _________             |
  |JP3  |  |        |TLP504A_| |TLP504A_|             |
  | __  |  |        __________                        |
  || |  |  |  C11-> |PIC16C56|                        |
  ||_|  |  |                                          |
  | ___ |__|   ___ <-SN74LS365AN                      |
  | VOL        |  |   ______    ___________________   |
  |  _______   |  |  | OKI |   |ROM U1             |  |
  | |DIPSx6|   |  |  |6295_|   |___________________|  |
  |            |__|                                   |
  |___________________________________________________|

  JP1 = 10 pin [+5V, GND, DAT, CLK, ENA, PU1, PU2, PU3, PU4, GND]
  JP2 = 14 pin [12VA, 12VA, +5V, ALT, CON, BOM, MOT, N/U, BOM, POT, ALT, 12V, GND, GND]
  JP3 =  5 pin [PU5, PU6, PU7, PU8, GND]

  The PCBs were inside two "Coche de Bomberos" kiddie rides from CMC Cresmatic (https://www.recreativas.org/coche-de-bomberos-6022-cresmatic).
  Anyway, the hardware is generic enough to serve any basic kiddie ride.

  There is a newer version of the PCB with the same components (Gaelco REF.920505, from 1992). It adds a fuse, a LED for PCB control, and
  better connectors. One of them, seen with the 'autopapa' sound ROM, has:
	U1   NEC D27C2001D-15 EPROM, labeled "AUTO PAPA reclam. F2C7 Pic. IRN" (0xF2C7 is the byte sum of the dump).
	U2   44-pin QFP (M6295, marking barely legible).
	U3   PIC16C56-RC/P (factory RC oscillator version), hand labeled "FUTBOL.N", not dumped.
	U7   SN74HCT273N output latch.
	U8   SN74LS365AN, buffers the 6 dips of SW1 (pull-ups in RR3).
	OP1  16-pin quad optocoupler, OP2 single optocoupler, TR1 to TR5 output transistors.
	C11  trimmer next to the PIC; there is no crystal or resonator on the PCB.
	SW2  2-position switch and SW3 MOT/LAMP jumpers, functions unknown.
	D7   'PCB CONTROL' LED, F1 fuse.
  It has a single 15-pin connector and no connector for the external display board.

  Hardware details, deduced from the 'IRN' kiddie ride program (m.irn_pic16c56.u3) and the PCB:

  The PIC runs in RC oscillator mode, with the frequency set by trimmer C11 and R1. The decapped 'IR' PIC
  (see 'donpepito') has config word 0x?07: RC oscillator, watchdog enabled, code protected. The program assumes 4 MHz: the main loop runs
  every 10 ms and the ride times are exact seconds. The M6295 is presumably clocked from the PIC OSC2/CLKOUT
  pin (Fosc / 4 = 1 MHz), so C11 adjusts both the timings and the sound pitch.

  PIC port A (all outputs):
	RA0  M6295 /CS
	RA1  M6295 /WR and 74LS365 /G1
	RA2  74HCT273 CLK
	RA3  M6295 /RD and 74LS365 /G2

  PIC port B is a data bus shared by:
	- the M6295 (commands are latched on the /WR rising edge, the status is read with /RD low).
	- the 74HCT273 output latch.
	- the 74LS365, enabled when /WR and /RD are both low while /CS is high: 6 DIP switches on D0-D5.
	- the inputs, active low, read when nothing else drives the bus (PIC as input, RA0, RA1 and RA3 high).

  74HCT273 outputs, through the ULN2803A open collector drivers:
	Q0  JP1 DAT  \
	Q1  JP1 CLK   > serial link to the external display board
	Q2  JP1 ENA  /
	Q3  coin counter
	Q4  auxiliary output, toggled at 75%, 50% and 25% of the ride time (maybe JP2 POT)
	Q5  lamp
	Q6  lamp
	Q7  motor

  External display board: 16 bits are shifted in on each Q1 falling edge (rising edge at the connector) and
  latched when Q2 goes high again. First bit shifted: credits display enable, then the units digit
  segments (a, f, e, d, c, g, b), then the time display enable and the tens digit segments (f, g, c, d, e, b, a).
  The program alternates the credits and time left displays on every main loop tick (multiplexing).

  'IRN' kiddie ride program (the 'IR' PIC of 'donpepito' holds exactly the same 1024 program words, and the
  same ID words 0A 05 02 01):
	- Idle: with demo sounds enabled, phrase 1 plays about every 4 minutes while the lamps blink.
	- A credit starts the ride: phrase 2, then phrase 8 (the song) loops on voice 1, the motor runs,
	  the lamps alternate and the time display counts from 99 down to 0.
	- At 75, 50 and 25 the motor stops for about 0.7 seconds, Q4 toggles and phrase 9 plays on voice 4
	  (the buttons are ignored while phrase 9 plays). Phrase 9 is optional: the M6295 ignores the request
	  when the ROM leaves it empty.
	- Button 1 plays phrase 3 on voice 2, button 2 plays phrase 4 on voice 3.
	- End of ride: phrase 5, unless there are credits left; in that case phrase 7 plays and the next ride
	  starts after pressing start or after about 30 seconds.
	- Phrase 6 is not used.
	- The DIP switches are read only at power on.

  The 'IRN' sound ROMs share a common set of phrases: 2 (ride start), 3 and 4 (button sounds), 5 (ride end)
  and 7 (credits left) are byte identical in all of them. Phrase 9 (75/50/25 announcement) is also identical in
  'autopapa' and 'susanita', while 'donpepito', 'mueve' and 'obladi' don't have it. Phrase 1 (attract jingle)
  and phrase 8 (song) are specific to each ride.

  The hex number on the sound ROM labels ('mueve_reclam_ea76', 'obladi_reclam_5a5c', "AUTO PAPA reclam. F2C7")
  is the 16-bit sum of all the EPROM bytes, including any leftover data after the phrases. The labels also name
  the PIC program the ROM is meant for ('Pic. IRN').

  TODO:
  - Verify the M6295 SS pin. PIN7_HIGH is the most likely setting: at 7575 Hz the songs of 'mueve' (a cover of a
	dance hit of about 123-130 BPM), 'donpepito' (known recordings at 130-137 BPM) and 'obladi' (original at
	113-115 BPM) play at 123.6, 128.9 and 120.1 BPM, while at 6060 Hz they would drop to 98.9, 103.1 and
	96.1 BPM. The tuning of the musical pieces is inconclusive.
  - Verify the DIP switch order and the connector assignment of the inputs and outputs.
  - Dump the 'FUTBOL.N' PIC of the REF.920505 PCB, and find out what SW2, SW3 and D7 do.
  - The pinballs have not been analysed.
*/

#include "emu.h"

#include "cpu/pic16c5x/pic16c5x.h"
#include "sound/okim6295.h"

#include "speaker.h"

#include "futbol3_kid.lh"

#define LOG_OKI     (1U << 1)
#define LOG_DISPLAY (1U << 2)

#define VERBOSE (0)
#include "logmacro.h"


namespace {

// RC oscillator adjusted with trimmer C11, the programs expect 4 MHz
static constexpr u32 PIC_CLOCK = 4'000'000;

class gaelcof3_state : public driver_device
{
public:
	gaelcof3_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_oki(*this, "oki"),
		m_inputs(*this, "IN0"),
		m_dsw(*this, "DSW1"),
		m_lamps(*this, "lamp%u", 0U),
		m_motor(*this, "motor"),
		m_aux(*this, "aux"),
		m_digits(*this, "digit%u", 0U)
	{ }

	void gaelcof3(machine_config &config) ATTR_COLD;
	void gaelcof3_c54(machine_config &config) ATTR_COLD;

	void init_irn() ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	required_device<pic16c5x_device> m_maincpu;
	required_device<okim6295_device> m_oki;
	required_ioport m_inputs;
	required_ioport m_dsw;
	output_finder<2> m_lamps;
	output_finder<> m_motor;
	output_finder<> m_aux;
	output_finder<4> m_digits;

	u8 m_porta = 0x0f;
	u8 m_portb = 0xff;
	u8 m_portb_driven = 0x00;
	u8 m_latch = 0x00;
	u16 m_display_shift = 0;

	void common(machine_config &config) ATTR_COLD;

	void porta_w(offs_t offset, u8 data, u8 mem_mask);
	u8 portb_r();
	void portb_w(offs_t offset, u8 data, u8 mem_mask);

	u8 bus_r();
	void latch_w(u8 data);
	void update_outputs();
	void update_display();
};


void gaelcof3_state::machine_start()
{
	save_item(NAME(m_porta));
	save_item(NAME(m_portb));
	save_item(NAME(m_portb_driven));
	save_item(NAME(m_latch));
	save_item(NAME(m_display_shift));
}

void gaelcof3_state::machine_reset()
{
	// the PIC pins are high impedance after reset (assumed to be pulled up)
	m_porta = 0x0f;
	m_portb_driven = 0x00;

	// assume the 74HCT273 is cleared by the reset circuit
	m_latch = 0x00;
	update_outputs();
}

void gaelcof3_state::init_irn()
{
	// RC oscillator, watchdog enabled (0x0fff in the 'IRN' dump, 0x?07 in the decapped 'IR' PIC, which is code protected)
	m_maincpu->set_config(0x0fff);
}


u8 gaelcof3_state::bus_r()
{
	// 74LS365: both enables (active low) must be asserted
	if (!BIT(m_porta, 1) && !BIT(m_porta, 3))
		return 0xc0 | (m_dsw->read() & 0x3f);

	// M6295 status read
	if (!BIT(m_porta, 0) && !BIT(m_porta, 3))
		return m_oki->read();

	// nothing drives the bus: pull-ups and inputs
	return m_inputs->read();
}

void gaelcof3_state::porta_w(offs_t offset, u8 data, u8 mem_mask)
{
	// pins configured as inputs float high (assumed)
	data = (data & mem_mask) | (~mem_mask & 0x0f);

	u8 const old = m_porta;
	m_porta = data;

	// the M6295 latches a command on the /WR rising edge, with /CS asserted
	if (!BIT(old, 1) && BIT(data, 1) && !BIT(old, 0))
	{
		if (m_portb_driven == 0xff)
		{
			LOGMASKED(LOG_OKI, "M6295 write %02x\n", m_portb);
			m_oki->write(m_portb);
		}
		else
		{
			// happens once at power on, when the PIC port B is still an input
			LOGMASKED(LOG_OKI, "M6295 write ignored, bus not driven by the PIC\n");
		}
	}

	// 74HCT273 clock
	if (!BIT(old, 2) && BIT(data, 2))
		latch_w((m_portb & m_portb_driven) | (bus_r() & ~m_portb_driven));
}

u8 gaelcof3_state::portb_r()
{
	return bus_r();
}

void gaelcof3_state::portb_w(offs_t offset, u8 data, u8 mem_mask)
{
	m_portb = data;
	m_portb_driven = mem_mask;
}


void gaelcof3_state::latch_w(u8 data)
{
	u8 const old = m_latch;
	m_latch = data;
	update_outputs();

	// external display board, shift register clocked on the Q1 falling edge
	// (rising edge at the connector, after the ULN2803A inverter)
	if (BIT(old, 1) && !BIT(data, 1))
		m_display_shift = (m_display_shift << 1) | BIT(data, 0);

	// the shifted data is latched when Q2 goes high again
	if (!BIT(old, 2) && BIT(data, 2))
		update_display();
}

void gaelcof3_state::update_outputs()
{
	machine().bookkeeping().coin_counter_w(0, BIT(m_latch, 3));
	m_aux = BIT(m_latch, 4);
	m_lamps[0] = BIT(m_latch, 5);
	m_lamps[1] = BIT(m_latch, 6);
	m_motor = BIT(m_latch, 7);
}

void gaelcof3_state::update_display()
{
	// 16-bit frame, the first bit shifted in ends at bit 15 (1 = active):
	// bit 15: credits display enable, bits 14-8: units digit segments (a, f, e, d, c, g, b)
	// bit 7:  time display enable,    bits 6-0:  tens digit segments  (f, g, c, d, e, b, a)
	u8 const units = bitswap<7>(m_display_shift >> 8, 1, 5, 4, 3, 2, 0, 6);
	u8 const tens = bitswap<7>(m_display_shift, 5, 6, 2, 3, 4, 1, 0);

	LOGMASKED(LOG_DISPLAY, "display frame %04x\n", m_display_shift);

	if (BIT(m_display_shift, 15))
	{
		m_digits[0] = tens;
		m_digits[1] = units;
	}

	if (BIT(m_display_shift, 7))
	{
		m_digits[2] = tens;
		m_digits[3] = units;
	}
}


static INPUT_PORTS_START( gaelcof3 ) // generic, for programs not analysed yet
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 )

	PORT_START("DSW1") // only 6 switches, order not verified
	PORT_DIPUNKNOWN_DIPLOC( 0x01, 0x01, "SW1:1" )
	PORT_DIPUNKNOWN_DIPLOC( 0x02, 0x02, "SW1:2" )
	PORT_DIPUNKNOWN_DIPLOC( 0x04, 0x04, "SW1:3" )
	PORT_DIPUNKNOWN_DIPLOC( 0x08, 0x08, "SW1:4" )
	PORT_DIPUNKNOWN_DIPLOC( 0x10, 0x10, "SW1:5" )
	PORT_DIPUNKNOWN_DIPLOC( 0x20, 0x20, "SW1:6" )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED ) // not connected, pulled up
INPUT_PORTS_END

static INPUT_PORTS_START( irn ) // 'IRN' kiddie ride program
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 ) // only read at the end of a ride, when there are credits left
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("DSW1") // only 6 switches, order not verified, read only at power on
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Game_Time ) ) PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x00, "140 seconds" )
	PORT_DIPSETTING(    0x01, "126 seconds" )
	PORT_DIPSETTING(    0x02, "112 seconds" )
	PORT_DIPSETTING(    0x03, "98 seconds" )
	PORT_DIPSETTING(    0x04, "84 seconds" )
	PORT_DIPSETTING(    0x05, "70 seconds" )
	PORT_DIPSETTING(    0x06, "56 seconds" )
	PORT_DIPSETTING(    0x07, "42 seconds" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW1:5,6")
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED ) // not connected, pulled up
INPUT_PORTS_END


void gaelcof3_state::common(machine_config &config)
{
	m_maincpu->write_a().set(FUNC(gaelcof3_state::porta_w));
	m_maincpu->read_b().set(FUNC(gaelcof3_state::portb_r));
	m_maincpu->write_b().set(FUNC(gaelcof3_state::portb_w));

	SPEAKER(config, "mono").front_center();

	// clocked from the PIC CLKOUT pin (Fosc / 4), SS pin not verified
	OKIM6295(config, m_oki, PIC_CLOCK / 4, okim6295_device::PIN7_HIGH);
	m_oki->add_route(ALL_OUTPUTS, "mono", 1.0);
}

void gaelcof3_state::gaelcof3(machine_config &config)
{
	PIC16C56(config, m_maincpu, PIC_CLOCK);
	common(config);
}

void gaelcof3_state::gaelcof3_c54(machine_config &config)
{
	PIC16C54(config, m_maincpu, PIC_CLOCK);
	common(config);
}


// Pinballs

ROM_START( futbol )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD( "p4n_pic16c56.bin", 0x0000, 0x2000, CRC(a4d69b51) SHA1(aa0f20b45aa92912ab235c9dc30b3532ff7103eb) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "pinball_futbol_p4_97e7_p4n_26-6-98_27c020.bin", 0x00000, 0x40000, CRC(448d244b) SHA1(51c3d6309b487d17085aac161016190249e2900b) )
ROM_END

ROM_START( futbola )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD( "p4n_pic16c56.bin", 0x0000, 0x2000, CRC(a4d69b51) SHA1(aa0f20b45aa92912ab235c9dc30b3532ff7103eb) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "pinball_futbol_p3_20f6_p4n_21-10-97_27c020.bin", 0x00000, 0x40000, CRC(05a3595d) SHA1(226fd63ea23d06022bbad9eb5a60fe04707a8fca) )
ROM_END

ROM_START( futbolt )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD( "test_pic16c54a.bin", 0x0000, 0x2000, CRC(ad819aaa) SHA1(f10500e9147c703e24a26b4d48305c16996b7c0a) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "test_futbol_27c010a.bin", 0x00000, 0x20000, CRC(57cf1ca4) SHA1(8d7f027bf7809194035c5b4671919d3b3dce2f1b) )
ROM_END

// Kiddie rides

/* Based on the song "El auto feo", composed by Enrique Fischer 'Pipo Pescador'.
   Sound ROM label: "AUTO PAPA reclam. F2C7 Pic. IRN" (REF.920505 PCB, with a PIC labeled 'FUTBOL.N'). */
ROM_START( autopapa )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD( "m.irn_pic16c56.u3", 0x0000, 0x2000, CRC(089699f5) SHA1(2cc470a97936887804363c8783bad4db4cad4f64) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "autopapa.u1", 0x00000, 0x40000, CRC(a3e5607e) SHA1(24a9c79edec7b2f7f64b622240f2ad8f3ffa29ca) ) // NEC D27C2001D, sum 0xf2c7 matches the label
ROM_END

/* Based on the song "Hola Don Pepito", composed by Ramón del Rivero.
   The PIC is labeled 'IR'. Its program is identical to the 'IRN' one; the dump also has the ID words and the low
   byte of the config word (0x07: RC oscillator, watchdog enabled, code protected).
   The sound ROM has no phrase 9 (the 75/50/25 announcements are silent); the song fills it up to 0x3ff08, and the
   remaining 247 bytes match the 'mueve' EPROM image, so both were made with the same tools. */
ROM_START( donpepito )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD( "ir_pic16c56.u3", 0x0000, 0x1fff, CRC(a2c24ec3) SHA1(e87520c6de714b1638c9b156411522e0209fb06e) ) // Decapped, config word high byte missing

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "don_pepito.u1", 0x00000, 0x40000, CRC(574fcd14) SHA1(a23f1eb6d2cef5aa07df3a553fe1d33803648f43) ) // byte sum 0x793f
ROM_END

/* Based on the Spanish cover version of the song "I Like To Move It" by Reel 2 Real, named "Te Gusta el Mueve Mueve".
   The sound ROM has no phrase 9, so the 75/50/25 announcements are silent. The data area ends at 0x381a7, the rest
   of the EPROM has leftover data (not played, but included in the label checksum). */
ROM_START( mueve )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD( "m.irn_pic16c56.u3", 0x0000, 0x2000, CRC(089699f5) SHA1(2cc470a97936887804363c8783bad4db4cad4f64) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "mueve_reclam_ea76_pic_irn_27c020.u1", 0x00000, 0x40000, CRC(f3cc6936) SHA1(35334aeb85f3524f2afdf20f49005d7573ec5494) ) // sum 0xea76 matches the label
ROM_END

/* Based on the song by the Beatles.
   The sound ROM has no phrase 9, so the 75/50/25 announcements are silent. */
ROM_START( obladi )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD( "m.irn_pic16c56.u3", 0x0000, 0x2000, CRC(089699f5) SHA1(2cc470a97936887804363c8783bad4db4cad4f64) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "obladi_reclam_5a5c_pic_irn_27c020.u1", 0x00000, 0x40000, CRC(a156f749) SHA1(f2bcbe5857e8ea6d96c2abe3051a5d02308dc963) ) // sum 0x5a5c matches the label
ROM_END

/* Based on the song composed by Rafael Pérez Botija.
   The PIC on this PCB is labeled 'IR' and was not dumped; the 'IR' PIC of 'donpepito' has the same program as the
   'IRN' ones. The sound ROM defines phrases 1 to 9 with phrase 6 empty, exactly the phrases used by the program,
   and its common phrases are byte identical to the 'autopapa' ones.
   The data area ends at 0x287f0, the rest of the EPROM has leftover data (not played). */
ROM_START( susanita )
	ROM_REGION( 0x2000, "maincpu", 0 )
	ROM_LOAD( "m.irn_pic16c56.u3", 0x0000, 0x2000, CRC(089699f5) SHA1(2cc470a97936887804363c8783bad4db4cad4f64) )

	ROM_REGION( 0x40000, "oki", 0 )
	ROM_LOAD( "susanita.u1", 0x00000, 0x40000, CRC(766868cb) SHA1(eb42dc46b865bc448052d9d67c840e51c49ce49a) ) // Am27C020
ROM_END

} // anonymous namespace

GAME( 1998, futbol,       0, gaelcof3,     gaelcof3, gaelcof3_state, empty_init, ROT0, "Gaelco / Cresmatic", "Futbol (set 1)",    MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK | MACHINE_SUPPORTS_SAVE )
GAME( 1997, futbola, futbol, gaelcof3,     gaelcof3, gaelcof3_state, empty_init, ROT0, "Gaelco / Cresmatic", "Futbol (set 2)",    MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK | MACHINE_SUPPORTS_SAVE )
GAME( 1997, futbolt, futbol, gaelcof3_c54, gaelcof3, gaelcof3_state, empty_init, ROT0, "Gaelco / Cresmatic", "Futbol (test ROM)", MACHINE_NOT_WORKING | MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK | MACHINE_SUPPORTS_SAVE )

GAMEL( 199?, autopapa,  0, gaelcof3, irn, gaelcof3_state, init_irn, ROT0, "Gaelco / Cresmatic", u8"El auto de papá", MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK | MACHINE_SUPPORTS_SAVE, layout_futbol3_kid )
GAMEL( 199?, donpepito, 0, gaelcof3, irn, gaelcof3_state, init_irn, ROT0, "Gaelco / Cresmatic", "Don Pepito",        MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK | MACHINE_SUPPORTS_SAVE, layout_futbol3_kid )
GAMEL( 199?, mueve,     0, gaelcof3, irn, gaelcof3_state, init_irn, ROT0, "Gaelco / Cresmatic", "Mueve",             MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK | MACHINE_SUPPORTS_SAVE, layout_futbol3_kid )
GAMEL( 199?, obladi,    0, gaelcof3, irn, gaelcof3_state, init_irn, ROT0, "Gaelco / Cresmatic", "Ob-La-Di",          MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK | MACHINE_SUPPORTS_SAVE, layout_futbol3_kid )
GAMEL( 199?, susanita,  0, gaelcof3, irn, gaelcof3_state, init_irn, ROT0, "Gaelco / Cresmatic", "Susanita",          MACHINE_MECHANICAL | MACHINE_REQUIRES_ARTWORK | MACHINE_SUPPORTS_SAVE, layout_futbol3_kid )
