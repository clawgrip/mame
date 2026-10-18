// license:BSD-3-Clause
// copyright-holders: Tomás García-Merás Capote (ClawGrip)
/*******************************************************************************

 Gaelco "2x1 (puerta monedero)" credit distributor

 Takes the coins from one coin acceptor and hands the credits out to one or
 two Speed Up cabinets.  With two, each cabinet has a lit button the player
 presses to take them.

  ________________________
 |         ____  ____    |
 |         ····  ····    |
 |  ______   _________ ·||
 | LM340T5  |PIC16C54| ·||
 |                 ___ ·||
 |     _________  |  | ·||
 |    |ULN2003A1| |  |   |
 | ___            |  <-74HCT245N
 ||  |  _________ |__|   |
 ||  | |_DIPSx8_|      ·||
 ||  <-CD74HCT174E     ·||
 ||__|  _________      ·||
 |     |74HCT245N      ·||
 |_______________________|

 The schematic (2X1.SCH, September 1996) adds an RC oscillator (4K7, 22 pF)
 and a common cathode seven segment display.  Manuals with the DIP switch
 setup and the schematic can be downloaded from
 https://www.recreativas.org/manuales/videojuegos

 Notes from the programs:

 - The main loop is paced by TMR0 with a 1:256 prescaler, waiting for it to
   reach 0x4e, about 22 ms with the RC oscillator.  An input counts when it
   goes low from one loop to the next.

 - In parallel mode Inp4 strobes a three bit code on Inp1 to Inp3, read as it
   comes in, so all lines high is the code worth the most.

 - Coins go into a pool shown on the display, E standing for ten or more, and
   both lamps blink while it holds any.  Credits leave the pool as pulses on
   the CRED line of a cabinet, six loops long.

 - M.2x1 is the program the English manual describes, down to the extra
   credit for the second lap.  It waits about half a second after reset and
   then looks at Credit Button 2: held low by the jumper on JP4 pins 5 and 6,
   it hands every credit to JP2 as soon as it is in the pool, as the manual's
   single cabinet setup expects; otherwise each button press hands one credit
   to its cabinet.  CONT2 counts the coin pulses and CONT1 is not driven,
   which the manual calls reserved.  The extra credit counts the pulses of the
   coins inserted in one go.

 - 2x1 Prg works the same with older tables: Inp1 and the eighth parallel
   code are worth 25 pulses with SW2 off, the last entry of the bonus table is
   10, and there is no second lap credit.

 - M.2x1 Twin has the tables of 2x1 Prg, never looks for the jumper, keeps the
   parallel strobe flag in STATUS bit 7, and puts the coin pulses each credit
   cost on the CONT line of the cabinet that took it.

 - The Covielsa program is a different design.  Its serial inputs weigh 1, 2,
   4, 8 and 20 pulses, which fits 25, 50, 100, 200 and 500 peseta coins, the
   sixth input is ignored, SW2 to SW5 do something else than in the manual and
   SW6 to SW8 are not read.  Each button press hands one credit, or all of them
   with SW2 on, to its cabinet, the coin pulses each credit cost go out on that
   cabinet's CONT line, and the parallel strobe flag lives in STATUS bit 7
   (unused on a PIC16C54).  The extra pulses of its bonus settings count from
   the coins inserted since a credit was last taken.

 TODO:
  - Create a layout for the credit display.

*******************************************************************************/

#include "emu.h"
#include "gaelco_cred2x1.h"


DEFINE_DEVICE_TYPE(GAELCO_CRED2X1,      gaelco_cred2x1_device,      "gcred2x1",      "Gaelco 2x1 credit distributor")
DEFINE_DEVICE_TYPE(GAELCO_CRED2X1_COVI, gaelco_cred2x1_covi_device, "gcred2x1_covi", "Gaelco 2x1 credit distributor (Covielsa)")


namespace {

// programmer images: program words, ID words at 0x400 and the low byte of the
// config word at 0x1ffe; the Twin one stops after the ID words
ROM_START(gcred2x1)
	ROM_REGION(0x1fff, "pic", ROMREGION_ERASEFF)
	ROM_DEFAULT_BIOS("m2x1")

	ROM_SYSTEM_BIOS(0, "m2x1", "M.2x1")
	ROMX_LOAD("m.2x1_93c7_pic16c54a.u1", 0x0000, 0x1fff, CRC(794f0f2b) SHA1(2cdcb645f6a61770755cdcb554e33af8bdafc4f9), ROM_BIOS(0))

	ROM_SYSTEM_BIOS(1, "prg", "2x1 Prg")
	ROMX_LOAD("2x1_prg_102ee_pic16c54a.u1", 0x0000, 0x1fff, CRC(70ae94a7) SHA1(9e0d8d7fa400668355b3954d89ff020f1b307f63), ROM_BIOS(1))

	ROM_SYSTEM_BIOS(2, "twin", "M.2x1 Twin")
	ROMX_LOAD("m.2x1_twin_pic16c54a.u1", 0x0000, 0x0407, CRC(b6d8ac40) SHA1(435ebe5219ae65c1e6bf8ac25215872c5b9ef3dc), ROM_BIOS(2))
ROM_END

ROM_START(gcred2x1_covi)
	ROM_REGION(0x1fff, "pic", 0)
	ROM_LOAD("m.2x1_covi_152b6_pic16c54.u1", 0x0000, 0x1fff, CRC(66dd3bd0) SHA1(fe9f010428c35042d6084288968103b8d8439dee))
ROM_END


INPUT_PORTS_START(gcred2x1)
	PORT_START("IN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN6 ) // Inp1, 20 pulses
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN5 ) // Inp2, 10 pulses
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN4 ) // Inp3, 5 or 8 pulses
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 ) // Inp4, 4 pulses
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 ) // Inp5, 2 pulses
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 ) // Inp6, 1 pulse
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Credit Button 1") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Credit Button 2") PORT_CODE(KEYCODE_W)

	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Coin Acceptor Output" ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, "Serial" )
	PORT_DIPSETTING(    0x00, "Parallel" )
	PORT_DIPNAME( 0x02, 0x02, "Coin Values" )          PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, "Europe except Spain" )
	PORT_DIPSETTING(    0x00, "Spain and USA" )
	PORT_DIPNAME( 0x04, 0x04, "Parallel Coin Table" )  PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, "Reduced" )
	PORT_DIPSETTING(    0x00, "Extended" )
	PORT_DIPNAME( 0x18, 0x18, "Extra Credit" )         PORT_DIPLOCATION("SW1:4,5")
	PORT_DIPSETTING(    0x18, DEF_STR( None ) )
	PORT_DIPSETTING(    0x10, "At 5 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0xe0)
	PORT_DIPSETTING(    0x08, "At 4 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0xe0)
	PORT_DIPSETTING(    0x00, "At 2 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0xe0)
	PORT_DIPSETTING(    0x10, "At 5 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0xc0)
	PORT_DIPSETTING(    0x08, "At 4 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0xc0)
	PORT_DIPSETTING(    0x00, "At 2 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0xc0)
	PORT_DIPSETTING(    0x00, "At 6 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0xa0)
	PORT_DIPSETTING(    0x10, "At 5 pulses, 2 on the second lap" ) PORT_CONDITION("DSW", 0xe0, EQUALS, 0xa0)
	PORT_DIPSETTING(    0x08, "At 3 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0xa0)
	PORT_DIPSETTING(    0x10, "At 20 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x80)
	PORT_DIPSETTING(    0x08, "At 16 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x80)
	PORT_DIPSETTING(    0x00, "At 8 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0x80)
	PORT_DIPSETTING(    0x10, "At 25 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x60)
	PORT_DIPSETTING(    0x08, "At 20 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x60)
	PORT_DIPSETTING(    0x00, "At 10 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x60)
	PORT_DIPSETTING(    0x10, "At 20 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x40)
	PORT_DIPSETTING(    0x08, "At 16 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x40)
	PORT_DIPSETTING(    0x00, "At 8 pulses" )          PORT_CONDITION("DSW", 0xe0, EQUALS, 0x40)
	PORT_DIPSETTING(    0x10, "At 25 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x20)
	PORT_DIPSETTING(    0x08, "At 20 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x20)
	PORT_DIPSETTING(    0x00, "At 10 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x20)
	PORT_DIPSETTING(    0x00, "At 24 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x00)
	PORT_DIPSETTING(    0x10, "At 20 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x00)
	PORT_DIPSETTING(    0x08, "At 16 pulses" )         PORT_CONDITION("DSW", 0xe0, EQUALS, 0x00)
	PORT_DIPNAME( 0xe0, 0xe0, "Pulses Per Credit" )    PORT_DIPLOCATION("SW1:6,7,8")
	PORT_DIPSETTING(    0x00, "12" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x40, "8" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xe0, "1" )

	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Cabinets" )
	PORT_CONFSETTING(    0x00, "Twin" )
	PORT_CONFSETTING(    0x01, "Single (JP4 pins 5-6 linked)" )
INPUT_PORTS_END

INPUT_PORTS_START(gcred2x1_covi)
	PORT_START("IN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) // 1 pulse
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) // 2 pulses
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) // 4 pulses
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 ) // 8 pulses
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN5 ) // 20 pulses
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Credit Button 1") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Credit Button 2") PORT_CODE(KEYCODE_W)

	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Coin Acceptor Output" )     PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, "Serial" )
	PORT_DIPSETTING(    0x00, "Parallel" )
	PORT_DIPNAME( 0x02, 0x02, "Credits Per Button Press" ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, "One" )
	PORT_DIPSETTING(    0x00, "All" )
	PORT_DIPNAME( 0x1c, 0x1c, "Pulses Per Credit" )        PORT_DIPLOCATION("SW1:3,4,5")
	PORT_DIPSETTING(    0x04, "8" )
	PORT_DIPSETTING(    0x00, "8, 2 extra every 20" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x08, "4, 4 extra every 20" )
	PORT_DIPSETTING(    0x14, "2" )
	PORT_DIPSETTING(    0x10, "2, 1 extra every 4" )
	PORT_DIPSETTING(    0x1c, "1" )
	PORT_DIPSETTING(    0x18, "1, 1 extra every 4" )
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x20, "SW1:6" )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW1:7" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:8" )

	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x00, "Cabinets" )
	PORT_CONFSETTING(    0x00, "Twin" )
	PORT_CONFSETTING(    0x01, "Single (JP4 pins 5-6 linked)" )
INPUT_PORTS_END

} // anonymous namespace


gaelco_cred2x1_base::gaelco_cred2x1_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, type, tag, owner, clock)
	, m_pic(*this, "pic")
	, m_display(*this, "display")
	, m_in(*this, "IN")
	, m_dsw(*this, "DSW")
	, m_config(*this, "CONFIG")
	, m_meter_cb(*this)
	, m_lamp_cb(*this)
	, m_digit_cb(*this)
	, m_porta(0x0f)
	, m_portb(0)
	, m_latch(0)
{
}

gaelco_cred2x1_device::gaelco_cred2x1_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: gaelco_cred2x1_base(mconfig, GAELCO_CRED2X1, tag, owner, clock)
{
}

gaelco_cred2x1_covi_device::gaelco_cred2x1_covi_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: gaelco_cred2x1_base(mconfig, GAELCO_CRED2X1_COVI, tag, owner, clock)
{
}

const tiny_rom_entry *gaelco_cred2x1_device::device_rom_region() const
{
	return ROM_NAME(gcred2x1);
}

ioport_constructor gaelco_cred2x1_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gcred2x1);
}

const tiny_rom_entry *gaelco_cred2x1_covi_device::device_rom_region() const
{
	return ROM_NAME(gcred2x1_covi);
}

ioport_constructor gaelco_cred2x1_covi_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gcred2x1_covi);
}

void gaelco_cred2x1_base::device_add_mconfig(machine_config &config)
{
	PIC16C54(config, m_pic, 3'700'000); // RC oscillator, approximate
	m_pic->set_config(0xfff); // RC oscillator, watchdog on
	m_pic->write_a().set(FUNC(gaelco_cred2x1_base::porta_w));
	m_pic->read_b().set(FUNC(gaelco_cred2x1_base::portb_r));
	m_pic->write_b().set(FUNC(gaelco_cred2x1_base::portb_w));

	PWM_DISPLAY(config, m_display).set_size(1, 8);
	m_display->set_segmask(1, 0xff);
	m_display->output_digit().set(FUNC(gaelco_cred2x1_base::digit_w));
}

void gaelco_cred2x1_base::device_start()
{
	save_item(NAME(m_porta));
	save_item(NAME(m_portb));
	save_item(NAME(m_latch));
}

void gaelco_cred2x1_base::device_reset()
{
	m_porta = 0x0f;
	latch_w(0);
	update_display();
}


void gaelco_cred2x1_base::porta_w(offs_t offset, u8 data, u8 mem_mask)
{
	// RR3 pulls up the pins left as inputs
	data |= ~mem_mask & 0x0f;
	u8 const rising = data & ~m_porta;
	m_porta = data;

	if (BIT(rising, 3))
		latch_w(m_portb & 0x1f);
	update_display();
}

u8 gaelco_cred2x1_base::portb_r()
{
	u8 data = 0xff;
	if (!BIT(m_porta, 0))
	{
		data &= m_in->read();
		if (BIT(m_config->read(), 0))
			data &= 0x7f;
	}
	if (!BIT(m_porta, 1))
		data &= m_dsw->read();
	return data;
}

void gaelco_cred2x1_base::portb_w(u8 data)
{
	m_portb = data;
	update_display();
}

void gaelco_cred2x1_base::latch_w(u8 data)
{
	u8 const old = m_latch;
	m_latch = data;

	for (unsigned i = 0; i < 2; i++)
	{
		if (BIT(old ^ data, 2 * i))
			m_meter_cb[i](BIT(data, 2 * i));
	}
	if (BIT(old ^ data, 4))
		m_lamp_cb(BIT(data, 4));
}

void gaelco_cred2x1_base::update_display()
{
	m_display->matrix(1, BIT(m_porta, 2) ? m_portb : 0);
}
