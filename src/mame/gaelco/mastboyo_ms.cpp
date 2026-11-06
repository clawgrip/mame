// license:BSD-3-Clause
// copyright-holders:

/*

  Master Boy, Modular System version (Z80, 1987).

  This is Gaelco's first game. Master Boy was originally developed for Modular System, so this was
  the very first Master Boy.

  The exact hardware setup is unknown. At least four Modular System PCBs for Master Boy:
    - MOD 1: Sound board (Z80, 2 x YM2203 according to the program).
    - MOD 3: CPU board (Z80).
    - MOD 4: Tilemap board.
    - MOD ?: ROM board for questions.

  The program is similar to the single board version (mastboyo.cpp):
    - The sound CPU runs the same sound driver and music, using only the SSG part of the first
      YM2203.
    - The player inputs and DIP switches are read through the YM2203 ports. The sound CPU passes
      them to the main CPU in 16 bytes both CPUs can access, which also carry the sound commands.
    - The palette is in RAM, filled from a table in the program ROM.
    - The MOD 4 scroll registers (0x5800-0x5803) are only written at boot, with fixed values that
      align the picture.

  The MOD 3 address decoder (P0314) only takes two bank bits, so the four question banks repeat.
  The program drops the repeated question blocks at boot. Its outputs select the program EPROM at
  0x0000-0x3fff and the question EPROMs at 0x8000-0xffff, and give one code to each 2 KB area of
  0x4000-0x7fff, where the boards tell their registers apart with A12, A11 and A10.

  TODO:
  - clocks, screen timing and sound interrupt source

*/

#include "emu.h"

#include "gaelco_ms_mod1.h"
#include "gaelco_ms_mod3.h"
#include "gaelco_ms_mod4.h"

#include "cpu/z80/z80.h"
#include "machine/nvram.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"


namespace {

class mastboyo_ms_state : public driver_device
{
public:
	mastboyo_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mod1(*this, "mod1"),
		m_mod3(*this, "mod3"),
		m_mod4(*this, "mod4"),
		m_palette(*this, "palette"),
		m_program(*this, "maincpu"),
		m_questions(*this, "questions"),
		m_ram(*this, "nvram", 0x800, ENDIANNESS_LITTLE),
		m_paletteram(*this, "palette", 0x100, ENDIANNESS_LITTLE),
		m_paletteram_ext(*this, "palette_ext", 0x100, ENDIANNESS_LITTLE)
	{ }

	void mastboyo_ms(machine_config &config) ATTR_COLD;
	void mastboyoma(machine_config &config) ATTR_COLD;

	void init_mastboyom() ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<gaelco_ms_mod1_device> m_mod1;
	required_device<gaelco_ms_mod3_decoder_device> m_mod3;
	required_device<gaelco_ms_mod4_device> m_mod4;
	required_device<palette_device> m_palette;
	required_region_ptr<u8> m_program;
	required_region_ptr<u8> m_questions;
	memory_share_creator<u8> m_ram;
	memory_share_creator<u8> m_paletteram;
	memory_share_creator<u8> m_paletteram_ext;

	void main_program_map(address_map &map) ATTR_COLD;

	void mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch);
	uint8_t irq_ack_r();
	void tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
};


void mastboyo_ms_state::tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	code = data & 0xff;
	// palette bank lines are scrambled, the program reorders its palette table to match
	color = bitswap<4>(data, 8, 9, 11, 10);
}

uint32_t mastboyo_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_mod4->draw(screen, bitmap, cliprect);

	return 0;
}


void mastboyo_ms_state::mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch)
{
	if (BIT(code, 7))
	{
		// EPROM sockets: 6 program, 3 and 5 questions; bits 2-3 drive EPROM A14 and A15
		u8 *base = nullptr;
		switch (BIT(code, 4, 3))
		{
		case 3: base = &m_questions[0x00000]; break;
		case 5: base = &m_questions[0x10000]; break;
		case 6: base = &m_program[0]; break;
		}
		if (base && !write)
			view.install_rom(start, end, &base[(BIT(code, 2, 2) << 14) | (start & 0x3fff)]);
		return;
	}

	for (offs_t block = start; block < end; block += 0x400)
	{
		offs_t const last = block + 0x3ff;
		bool const a10 = BIT(block, 10);
		switch (code)
		{
		case 0x6d: // RAM, and the MOD 4 board at A12 for writes
			if (!BIT(block, 12))
			{
				if (write)
					view.install_writeonly(block, last, &m_ram[block & 0x400]);
				else
					view.install_rom(block, last, &m_ram[block & 0x400]);
			}
			else if (write)
			{
				view.install_write_handler(block, last, a10 ?
						write8sm_delegate(*m_mod4, FUNC(gaelco_ms_mod4_device::attr_w)) :
						write8sm_delegate(*m_mod4, FUNC(gaelco_ms_mod4_device::code_w)));
			}
			break;

		case 0x71: // writes: bank latch, MOD 1 board at A10; reads: MOD 4 board
			if (write)
			{
				if (a10)
					view.install_write_handler(block, block + 0x00f, 0, 0x3f0, 0, write8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_device::host_w)));
				else
					view.install_write_handler(block, last, write8smo_delegate(*m_mod3, FUNC(gaelco_ms_mod3_decoder_device::latch_w)));
			}
			else if (!BIT(block, 11))
			{
				view.install_read_handler(block, last, a10 ?
						read8sm_delegate(*m_mod4, FUNC(gaelco_ms_mod4_device::attr_r)) :
						read8sm_delegate(*m_mod4, FUNC(gaelco_ms_mod4_device::code_r)));
			}
			break;

		case 0x61: // reads: MOD 1 board at A10, palette at A11
			if (BIT(block, 11))
				view.install_rom(block, block + 0x0ff, 0x300, a10 ? &m_paletteram_ext[0] : &m_paletteram[0]);
			else if (a10)
				view.install_read_handler(block, block + 0x00f, 0, 0x3f0, 0, read8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_device::host_r)));
			break;

		case 0x65: // MOD 4 board scroll registers, written once with fixed values
			break;

		case 0x42: // palette writes
			view.install_write_handler(block, block + 0x0ff, 0, 0x300, 0, a10 ?
					write8sm_delegate(*m_palette, FUNC(palette_device::write8_ext)) :
					write8sm_delegate(*m_palette, FUNC(palette_device::write8)));
			break;

		case 0x7d: // interrupt acknowledge
			if (!write)
				view.install_read_handler(block, last, read8smo_delegate(*this, FUNC(mastboyo_ms_state::irq_ack_r)));
			break;
		}
	}
}

uint8_t mastboyo_ms_state::irq_ack_r()
{
	if (!machine().side_effects_disabled())
		m_maincpu->set_input_line(0, CLEAR_LINE);

	return 0xff;
}


void mastboyo_ms_state::main_program_map(address_map &map)
{
	map(0x0000, 0xffff).m(m_mod3, FUNC(gaelco_ms_mod3_decoder_device::map));
}


static INPUT_PORTS_START( mastboyo_ms )
	// note, Player 2 buttons must be used when entering name, and can be used for answering questions even in single player mode
	PORT_START("IN0") // first YM2203 port A
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Red / Delete Initial")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Red / <<")
	PORT_BIT( 0x3c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START("IN1") // first YM2203 port B
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Green / Enter Initial")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Green / >>")

	PORT_START("DSW") // second YM2203 port B, the sound CPU reverses the bit order
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW ) // From manual... 'Test Inicial'
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )  // From manual... 'Con Reclamo'
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, "Disabled" )
INPUT_PORTS_END


void mastboyo_ms_state::mastboyo_ms(machine_config &config)
{
	// basic machine hardware
	Z80(config, m_maincpu, 20_MHz_XTAL / 6); // Unknown clock and divisor
	m_maincpu->set_addrmap(AS_PROGRAM, &mastboyo_ms_state::main_program_map);
	m_maincpu->set_vblank_int("screen", FUNC(mastboyo_ms_state::irq0_line_assert));

	using decoder = gaelco_ms_mod3_decoder_device;
	GAELCO_MS_MOD3_DECODER(config, m_mod3);
	m_mod3->set_lines({ decoder::A(11), decoder::A(12), decoder::A(13), decoder::A(14), decoder::NOT_A(15), decoder::HIGH, decoder::WRITE, decoder::LATCH(0), decoder::LATCH(1) });
	m_mod3->set_latch_mask(0x03);
	m_mod3->set_install_callback(FUNC(mastboyo_ms_state::mod3_install));

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);

	// video hardware
	screen_device &screen(SCREEN(config, "screen"));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(0));
	screen.set_size(256, 256);
	screen.set_visarea(0, 256-1, 2*8, 256-2*8-1);
	screen.set_screen_update(FUNC(mastboyo_ms_state::screen_update));
	screen.set_palette(m_palette);

	PALETTE(config, m_palette).set_format(palette_device::xBGR_333_nibble, 0x100);

	GAELCO_MS_MOD4(config, m_mod4);
	m_mod4->set_palette(m_palette);
	m_mod4->set_tile_size(8);
	m_mod4->set_tile_callback(FUNC(mastboyo_ms_state::tile_cb));

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1(config, m_mod1, 4'000'000); // unknown, no crystal on the board
	// Unknown clock and divisor, 2.5 MHz gives the pitch of the single board version with the prescaler the program selects
	m_mod1->set_ym_clock(20_MHz_XTAL / 8);
	// unknown source, 4 times the 256.244 Hz measured on the single board version gives its music tempo
	m_mod1->set_int_period(attotime::from_hz(4 * 256.244));
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<3>().set_ioport("DSW");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 1.0);
}

void mastboyo_ms_state::mastboyoma(machine_config &config)
{
	mastboyo_ms(config);

	// the sound program waits for 2 interrupts per update instead of 4, half the rate keeps the same tempo
	m_mod1->set_int_period(attotime::from_hz(2 * 256.244));
}


ROM_START( mastboyom )
	ROM_REGION( 0x10000, "maincpu", 0 ) // on MOD-3 board
	ROM_LOAD( "ma_303_27512.bin", 0x00000, 0x10000, CRC(0d48929b) SHA1(eaa8e3752d5d5a051813dd408988d53430d2df1a) ) //  1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x20000, "questions", 0 ) // on MOD-3 board
	ROM_LOAD( "ma_302_27512.bin", 0x00000, 0x10000, CRC(871d4a70) SHA1(20126ca9887d51f488847540d23264bc1c9a8233) ) // decrypted: mastboyo mastboy_27256.ic10 + mastboy_27256.ic9
	ROM_LOAD( "ma_301_27512.bin", 0x10000, 0x10000, CRC(3b67c516) SHA1(bf36b143e0df10fc871a7b079c4aa8a2cbdccca0) ) // decrypted: mastboyob mb_1.ic11 + mastboyo mastboy_27256.ic11

	ROM_REGION( 0x8000, "mod1", 0 ) // on MOD-1 board
	ROM_LOAD( "ma_0101_n_27c256.bin", 0x0000, 0x8000, CRC(7c1ae820) SHA1(2922b9094289daa2830acb6aab3b72941eab02b8) ) // 1xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "82s123.ic20", 0x00, 0x20, NO_DUMP )
	// not dumped, hand-made with the P0101 contents, which fit the sound program
	ROM_FILL( 0x00, 0x08, 0xfc )
	ROM_FILL( 0x08, 0x08, 0xfd )
	ROM_FILL( 0x10, 0x04, 0xff )
	ROM_FILL( 0x14, 0x04, 0x7f )
	ROM_FILL( 0x18, 0x04, 0xfb )
	ROM_FILL( 0x1c, 0x01, 0xef )
	ROM_FILL( 0x1d, 0x01, 0xf7 )
	ROM_FILL( 0x1e, 0x01, 0xbf )
	ROM_FILL( 0x1f, 0x01, 0xdf )

	ROM_REGION( 0x10000, "mod4", ROMREGION_INVERT ) // on MOD-4 board
	ROM_LOAD32_BYTE( "ma_0401_27128.bin", 0x00003, 0x04000, CRC(ad9f97d2) SHA1(1b8cde21c5fd5f1970ef6cc20616bff5c36a6b68) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_LOAD32_BYTE( "ma_0402_27128.bin", 0x00002, 0x04000, CRC(76d14275) SHA1(6bd920bd58b9b30cd81e2658084639a16314322d) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_LOAD32_BYTE( "ma_0403_27128.bin", 0x00001, 0x04000, CRC(58abbe25) SHA1(0747985ceaf74c9f1fd34b6927d2d5dc9e8d8ad9) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_LOAD32_BYTE( "ma_0404_27128.bin", 0x00000, 0x04000, CRC(7e9286c2) SHA1(495b065ee5cf0f5a88dee7eb999a54dae53921bd) ) // 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x200, "mod3", 0 ) // on MOD-3 board, address decoder
	ROM_LOAD( "p0314_82s147a.bin", 0x000, 0x200, CRC(aea2bbab) SHA1(506cca2acd8e65e5dbddef3a7e93950d54cf3d5a) )
ROM_END

ROM_START( mastboyoma )
	ROM_REGION( 0x10000, "maincpu", 0 ) // on MOD-3 board
	ROM_LOAD( "ma_303_27512.bin", 0x00000, 0x10000, CRC(0d48929b) SHA1(eaa8e3752d5d5a051813dd408988d53430d2df1a) ) //  1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x20000, "questions", 0 ) // on MOD-3 board
	ROM_LOAD( "ma_302_27512.bin", 0x00000, 0x10000, CRC(871d4a70) SHA1(20126ca9887d51f488847540d23264bc1c9a8233) ) // decrypted: mastboyo mastboy_27256.ic10 + mastboy_27256.ic9
	ROM_LOAD( "ma_301_27512.bin", 0x10000, 0x10000, CRC(3b67c516) SHA1(bf36b143e0df10fc871a7b079c4aa8a2cbdccca0) ) // decrypted: mastboyob mb_1.ic11 + mastboyo mastboy_27256.ic11

	ROM_REGION( 0x8000, "mod1", 0 ) // on MOD-1 board
	ROM_LOAD( "ma_0101_v_27c256.bin", 0x0000, 0x8000, CRC(e754176b) SHA1(325415ddc6bbe77493a7ee0130c1fe103f771bb0) ) // 1xxxxxxxxxxxxxx = 0xFF, only differs from ma_0101_n at 0x89 and 0x91

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "82s123.ic20", 0x00, 0x20, NO_DUMP )
	// not dumped, hand-made with the P0101 contents, which fit the sound program
	ROM_FILL( 0x00, 0x08, 0xfc )
	ROM_FILL( 0x08, 0x08, 0xfd )
	ROM_FILL( 0x10, 0x04, 0xff )
	ROM_FILL( 0x14, 0x04, 0x7f )
	ROM_FILL( 0x18, 0x04, 0xfb )
	ROM_FILL( 0x1c, 0x01, 0xef )
	ROM_FILL( 0x1d, 0x01, 0xf7 )
	ROM_FILL( 0x1e, 0x01, 0xbf )
	ROM_FILL( 0x1f, 0x01, 0xdf )

	ROM_REGION( 0x10000, "mod4", ROMREGION_INVERT ) // on MOD-4 board
	ROM_LOAD32_BYTE( "ma_0401_27128.bin", 0x00003, 0x04000, CRC(ad9f97d2) SHA1(1b8cde21c5fd5f1970ef6cc20616bff5c36a6b68) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_LOAD32_BYTE( "ma_0402_27128.bin", 0x00002, 0x04000, CRC(76d14275) SHA1(6bd920bd58b9b30cd81e2658084639a16314322d) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_LOAD32_BYTE( "ma_0403_27128.bin", 0x00001, 0x04000, CRC(58abbe25) SHA1(0747985ceaf74c9f1fd34b6927d2d5dc9e8d8ad9) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_LOAD32_BYTE( "ma_0404_27128.bin", 0x00000, 0x04000, CRC(7e9286c2) SHA1(495b065ee5cf0f5a88dee7eb999a54dae53921bd) ) // 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x200, "mod3", 0 ) // on MOD-3 board, address decoder
	ROM_LOAD( "p0314_82s147a.bin", 0x000, 0x200, CRC(aea2bbab) SHA1(506cca2acd8e65e5dbddef3a7e93950d54cf3d5a) )
ROM_END


void mastboyo_ms_state::init_mastboyom()
{
	gaelco_ms_mod3_descramble(&m_program[0], m_program.bytes());
	gaelco_ms_mod3_descramble(&m_questions[0], m_questions.bytes());
}

} // anonymous namespace


GAME( 1987, mastboyom,  mastboyo, mastboyo_ms, mastboyo_ms, mastboyo_ms_state, init_mastboyom, ROT0, "Gaelco", "Master Boy (1987, Modular System, set 1)", MACHINE_SUPPORTS_SAVE )
GAME( 1987, mastboyoma, mastboyo, mastboyoma,  mastboyo_ms, mastboyo_ms_state, init_mastboyom, ROT0, "Gaelco", "Master Boy (1987, Modular System, set 2)", MACHINE_SUPPORTS_SAVE )
