// license:BSD-3-Clause
// copyright-holders:
/*******************************************************************************

 Driver for "Loto-Play", a small PCB with a LED roulette installed on
 "First Games" arcade cabs from Covielsa that gives player the option to win a
 free play.

 It's a very simple PCB with this layout:
  ___________________________________
 |   _________               O <- GREEN LED
 |  |_8xDIPS_|            O     O   |
 |  _________________   O         O |
 | | MC68705P3S     |     O     O   |
 | |________________|        O      |
=|=                                 |
=|= <- 8-Pin Connector              |
 |__________________________________|

 Roulette of eight LEDs, seven red and one green (the upper one).
 There are at least two versions, one from 1988 and other from 1990.
 Some units use a M68705P5 or a Z80 instead of a MC68705P3S.

 More info and dip switches:
  - https://www.recreativas.org/loto-play-88-11392-gaelco-sa
  - https://www.recreativas.org/loto-play-90-14731-covielsa

 First Games arcade cab (see the bezel left upper corner with the roulette):
  - https://www.recreativas.org/first-games-954-covielsa

 Version with a PIC16C54 as main CPU:
  _____________________________________________
 |  |_8xDIPS_|   __________            O <- GREEN LED
 |              |SN74LS166N         O     O   |
 |                   Xtal         O         O |
 |                  4 MHz           O     O   |
 |         ___   ___                   O      |
=|=       |  |  |  |<-PIC16C54-XT/P           |
=|= <- 8-Pin Connector     ___                |
 |        |  |<-CNY/74-4  |  |<-TL7702ACP     |
 |        |__|  |__|      |__|                |
 |____________________________________________|


Notes on the roulette program (lotoplay, lotoplaya and lotoplayb):

- The LEDs are not wired in numerical order.  The rotation table in the ROM
  (DF BF 7F FE FD FB F7 EF) walks the ring as PB5, PB6, PB7, PB0, PB1, PB2,
  PB3, PB4, so the green LED at the top of the bezel is PB5.  The attract mode
  animations only come out symmetrical about it that way.

- The coin inputs are debounced in software by sampling them once per main
  loop, so a pulse has to stay low for two loops after having been high for
  six.

- PC0 emits one pulse per credit, so the board sits between the coin mechanism
  and the game PCB and repeats credits through it.  Winning the roulette just
  adds more credits to the pending counter.

- A spin is 48 steps through a deceleration ramp.  48 is a multiple of 8, so it
  always ends on the LED it started on, and the starting LED is what decides
  the prize.  Starting on the green one is only allowed once a 16 bit
  accumulator has carried into its high byte, and the per play increment is 5,
  10, 18 or 25 out of 256 depending on SW7 and SW8.


Notes on the 7-segment display program (lotoplayc):

- lotoplayc runs an unrelated program and is wired differently.  There is no
  roulette on it: PORTB bits 0 to 6 carry a common anode seven segment font
  (the entries for 0 to 8 are the canonical codes, the one for 9 is 0xe0 where
  0x10 would be expected) and what gets displayed is a credit counter clamped to
  ten.  Four DIP switches are read one at a time by driving a mux address on
  PA0-PA2 and sampling PA3, and the whole of PORTC is inputs.  Its two coin rate
  tables give one coin per four, three, two or one pulses on the first input, and
  three, two, five or four credits per pulse on the second.

- Every pin is accounted for and none of them carries sound.  PA0-PA1 and PA4-PA5
  hold a four bit value strobed out on PA2, PA6 and PA7 emit single pulses ten to
  twenty timer ticks wide, PB7 is turned around to be sampled, and the timer
  interrupt only keeps time, dividing by a hundred and then by sixty.  There is
  nothing anywhere that toggles a pin at an audio rate, unlike the roulette
  program, which swings PC1 in its interrupt handler.

*******************************************************************************/

#include "emu.h"

#include "cpu/m6805/m68705.h"
#include "cpu/pic16c5x/pic16c5x.h"

#include "sound/spkrdev.h"

#include "speaker.h"

#include "lotoplay_7s.lh"
#include "lotoplay_ro.lh"


namespace {

class lotoplay_ro_state : public driver_device
{
public:
	lotoplay_ro_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_speaker(*this, "speaker")
		, m_leds(*this, "led%u", 0U)
	{
	}

	void lotoplay_ro(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	void portb_w(u8 data)
	{
		for (unsigned i = 0; i < 8; i++)
			m_leds[i] = BIT(~data, i);
	}

	void portc_w(u8 data)
	{
		// there is nothing on the bezel to show the credits going out
		if (m_credit_line && !BIT(data, 0))
			popmessage("Credits out: %u", ++m_credit_count);
		m_credit_line = BIT(data, 0);

		m_speaker->level_w(BIT(data, 1));
	}

	required_device<m68705p3_device> m_maincpu;
	required_device<speaker_sound_device> m_speaker;
	output_finder<8> m_leds;

	bool m_credit_line = false;
	u32 m_credit_count = 0;
};

void lotoplay_ro_state::machine_start()
{
	save_item(NAME(m_credit_line));
	save_item(NAME(m_credit_count));
}

void lotoplay_ro_state::lotoplay_ro(machine_config &config)
{
	M68705P3(config, m_maincpu, 3'579'545); // MC68705P3S, unknown clock
	m_maincpu->porta_r().set_ioport("DSW");
	m_maincpu->portb_w().set(FUNC(lotoplay_ro_state::portb_w));
	m_maincpu->portc_r().set_ioport("IN");
	m_maincpu->portc_w().set(FUNC(lotoplay_ro_state::portc_w));

	SPEAKER(config, "mono").front_center();
	SPEAKER_SOUND(config, m_speaker).add_route(ALL_OUTPUTS, "mono", 0.35);
}


class lotoplay_7s_state : public driver_device
{
public:
	lotoplay_7s_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_dsw(*this, "DSW")
		, m_digit(*this, "digit0")
	{
	}

	void lotoplay_7s(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	u8 porta_r() { return BIT(m_dsw->read(), m_mux) << 3; }
	void porta_w(u8 data) { m_mux = data & 0x07; }
	void portb_w(u8 data) { m_digit = ~data & 0x7f; }

	required_device<m68705p3_device> m_maincpu;
	required_ioport m_dsw;
	output_finder<> m_digit;

	u8 m_mux = 0;
};

void lotoplay_7s_state::machine_start()
{
	save_item(NAME(m_mux));
}

void lotoplay_7s_state::lotoplay_7s(machine_config &config)
{
	M68705P3(config, m_maincpu, 3'579'545); // unknown clock
	m_maincpu->porta_r().set(FUNC(lotoplay_7s_state::porta_r));
	m_maincpu->porta_w().set(FUNC(lotoplay_7s_state::porta_w));
	m_maincpu->portb_w().set(FUNC(lotoplay_7s_state::portb_w));
	m_maincpu->portc_r().set_ioport("IN");
}


class lotoplay_ro_pic_state : public driver_device
{
public:
	lotoplay_ro_pic_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
	{
	}

	void lotoplay_ro_pic(machine_config &config) ATTR_COLD;

private:
	required_device<pic16c54_device> m_maincpu;
};

void lotoplay_ro_pic_state::lotoplay_ro_pic(machine_config &config)
{
	PIC16C54(config, m_maincpu, 4_MHz_XTAL);
}


/*
    The switches short their pin to ground and PORTA has internal pull-ups, so a
    switch that is on reads back as 0.
    SW5 and SW6 modify whatever the table produces rather than standing on their own.
*/
INPUT_PORTS_START(lotoplay_ro)
	PORT_START("DSW")
	PORT_DIPNAME( 0x03, 0x03, "Lottery Percentage" )     PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(    0x00, "2%" )
	PORT_DIPSETTING(    0x01, "4%" )
	PORT_DIPSETTING(    0x02, "7%" )
	PORT_DIPSETTING(    0x03, "10%" )
	PORT_DIPNAME( 0x04, 0x00, "Double Credit Values" )   PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Extra Credit On Coin A" ) PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coinage ) )       PORT_DIPLOCATION("SW1:1,2,3,4")
	PORT_DIPSETTING(    0x00, "Coin A 1C/1C, Coin B 1C/1C" )
	PORT_DIPSETTING(    0x10, "Coin A 1C/2C, Coin B 1C/1C" )
	PORT_DIPSETTING(    0x20, "Coin A 1C/3C, Coin B 1C/1C" )
	PORT_DIPSETTING(    0x30, "Coin A 1C/4C, Coin B 1C/1C" )
	PORT_DIPSETTING(    0x40, "Coin A 1C/5C, Coin B 1C/1C" )
	PORT_DIPSETTING(    0x50, "Coin A 1C/6C, Coin B 1C/1C" )
	PORT_DIPSETTING(    0x60, "Coin A 1C/7C, Coin B 1C/1C" )
	PORT_DIPSETTING(    0x70, "Coin A 1C/8C, Coin B 1C/1C" )
	PORT_DIPSETTING(    0x80, "Coin A 1C/2C, Coin B 2C/1C" )
	PORT_DIPSETTING(    0x90, "Coin A 1C/1C, Coin B 2C/1C" )
	PORT_DIPSETTING(    0xa0, "Coin A 1C/1C, Coin B 3C/1C" )
	PORT_DIPSETTING(    0xb0, "Coin A 1C/1C, Coin B 4C/1C" )
	PORT_DIPSETTING(    0xc0, "Coin A 1C/1C, Coin B 5C/1C" )
	PORT_DIPSETTING(    0xd0, "Coin A 1C/1C, Coin B 6C/1C" )
	PORT_DIPSETTING(    0xe0, "Coin A 1C/1C, Coin B 7C/1C" )
	PORT_DIPSETTING(    0xf0, "Coin A 1C/1C, Coin B 8C/1C" )

	PORT_START("IN")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_NAME("Coin B")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_NAME("Coin A")
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START(lotoplay_7s)
	PORT_START("DSW")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:3,4")
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("IN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Play A")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Play B")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_NAME("Coin A")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_NAME("Coin B")
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


// Sets with MC68705.

ROM_START(lotoplay)
	ROM_REGION(0x0800, "maincpu", 0)
	ROM_LOAD("lp_mostra_sp_ultima_68705p3s.bin", 0x0000, 0x0800, CRC(112645cd) SHA1(f2ad6b2fbec36d0bfe034d7bfb036ef6bf4ee395))
ROM_END

ROM_START(lotoplaya)
	ROM_REGION(0x0800, "maincpu", 0)
	ROM_LOAD("lp_mostra_s_125d9_68705p3s.bin", 0x0000, 0x0800, CRC(9b77603c) SHA1(6799b930f9805332bf20c6146b044222fe49d243))
ROM_END

ROM_START(lotoplayb)
	ROM_REGION(0x0800, "maincpu", 0)
	ROM_LOAD("lp_vii_sch_mostra_11302_68705p3s.bin", 0x0000, 0x0800, CRC(61b426d3) SHA1(b66dc6c382a04d8cdbaee342f179ce80abfd3c71))
ROM_END

// Different PCB than the previous sets, with MC68705 and a seven segment display instead of a roulette.
ROM_START(lotoplayc)
	ROM_REGION(0x0800, "maincpu", 0)
	ROM_LOAD("multn.bin", 0x0000, 0x0800, CRC(20a0e0d0) SHA1(832ed64dfa5f5f150f0e9918b40e9fb4e8e4260d))
ROM_END

// Sets with PIC16C54.

ROM_START(lotoplayp)
	ROM_REGION(0x1fff, "maincpu", 0)
	ROM_LOAD("loto_play_ff46_pic16c54.bin", 0x0000, 0x1fff, CRC(8840349d) SHA1(e9dcc572c7b577618ddda06be1538be69eb15584))
ROM_END

} // anonymous namespace


//     YEAR   NAME       PARENT    MACHINE          INPUT        CLASS                  INIT        ROT   COMPANY              FULLNAME                      FLAGS                                      LAYOUT
GAMEL( 1988?, lotoplay,  0,        lotoplay_ro,     lotoplay_ro, lotoplay_ro_state,     empty_init, ROT0, "Gaelco / Covielsa", "Loto-Play (MC68705, set 1)", MACHINE_SUPPORTS_SAVE,                     layout_lotoplay_ro )
GAMEL( 1988?, lotoplaya, lotoplay, lotoplay_ro,     lotoplay_ro, lotoplay_ro_state,     empty_init, ROT0, "Gaelco / Covielsa", "Loto-Play (MC68705, set 2)", MACHINE_SUPPORTS_SAVE,                     layout_lotoplay_ro )
GAMEL( 1988?, lotoplayb, lotoplay, lotoplay_ro,     lotoplay_ro, lotoplay_ro_state,     empty_init, ROT0, "Gaelco / Covielsa", "Loto-Play (MC68705, set 3)", MACHINE_SUPPORTS_SAVE,                     layout_lotoplay_ro )
GAMEL( 1988?, lotoplayc, lotoplay, lotoplay_7s,     lotoplay_7s, lotoplay_7s_state,     empty_init, ROT0, "Gaelco / Covielsa", "Loto-Play (MC68705, set 4)", MACHINE_NO_SOUND_HW | MACHINE_NOT_WORKING, layout_lotoplay_7s )
GAMEL( 1990?, lotoplayp, lotoplay, lotoplay_ro_pic, lotoplay_ro, lotoplay_ro_pic_state, empty_init, ROT0, "Gaelco / Covielsa", "Loto-Play (PIC16C54)",       MACHINE_NO_SOUND | MACHINE_NOT_WORKING,    layout_lotoplay_ro )
