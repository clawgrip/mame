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
  The program drops the repeated question blocks at boot.

  TODO:
  - clocks, screen timing and sound interrupt source

*/

#include "emu.h"

#include "gaelco_ms_mod1.h"

#include "cpu/z80/z80.h"
#include "machine/nvram.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"

#include <vector>


namespace {

class mastboyo_ms_state : public driver_device
{
public:
	mastboyo_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_tileram(*this, "tileram.%u", 0U),
		m_maincpu(*this, "maincpu"),
		m_mod1(*this, "mod1"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette"),
		m_rombank(*this, "rombank")
	{ }

	void mastboyo_ms(machine_config &config) ATTR_COLD;
	void mastboyoma(machine_config &config) ATTR_COLD;

	void init_mastboyom() ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void video_start() override ATTR_COLD;

private:
	required_shared_ptr_array<uint8_t, 2> m_tileram;
	required_device<cpu_device> m_maincpu;
	required_device<gaelco_ms_mod1_device> m_mod1;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;
	required_memory_bank m_rombank;

	tilemap_t *m_tilemap = nullptr;

	void main_program_map(address_map &map) ATTR_COLD;

	template<uint8_t Which> void tileram_w(offs_t offset, uint8_t data);
	void rombank_w(uint8_t data);
	uint8_t irq_ack_r();
	TILE_GET_INFO_MEMBER(get_tile_info);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
};


TILE_GET_INFO_MEMBER(mastboyo_ms_state::get_tile_info)
{
	int const code = m_tileram[0][tile_index];
	// palette bank lines are scrambled, the program reorders its palette table to match
	int const color = bitswap<4>(m_tileram[1][tile_index], 0, 1, 3, 2);

	tileinfo.set(0, code, color, 0);
}

void mastboyo_ms_state::video_start()
{
	m_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(mastboyo_ms_state::get_tile_info)), TILEMAP_SCAN_ROWS, 8, 8, 32, 32);
}

uint32_t mastboyo_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_tilemap->draw(screen, bitmap, cliprect, 0, 0);

	return 0;
}

template<uint8_t Which>
void mastboyo_ms_state::tileram_w(offs_t offset, uint8_t data)
{
	m_tileram[Which][offset] = data;
	m_tilemap->mark_tile_dirty(offset);
}


void mastboyo_ms_state::machine_start()
{
	m_rombank->configure_entries(0, 4, memregion("questions")->base(), 0x8000);
	m_rombank->set_entry(0);
}

void mastboyo_ms_state::rombank_w(uint8_t data)
{
	m_rombank->set_entry(data & 0x03);
}

uint8_t mastboyo_ms_state::irq_ack_r()
{
	if (!machine().side_effects_disabled())
		m_maincpu->set_input_line(0, CLEAR_LINE);

	return 0xff;
}


void mastboyo_ms_state::main_program_map(address_map &map)
{
	map(0x0000, 0x3fff).rom();
	map(0x4000, 0x47ff).ram().share("nvram");
	map(0x5000, 0x53ff).ram().w(FUNC(mastboyo_ms_state::tileram_w<0>)).share(m_tileram[0]);
	map(0x5400, 0x57ff).ram().w(FUNC(mastboyo_ms_state::tileram_w<1>)).share(m_tileram[1]);
	map(0x5800, 0x5803).nopw(); // MOD 4 scroll registers
	map(0x6000, 0x6000).w(FUNC(mastboyo_ms_state::rombank_w));
	map(0x6400, 0x640f).rw(m_mod1, FUNC(gaelco_ms_mod1_device::host_r), FUNC(gaelco_ms_mod1_device::host_w));
	map(0x6800, 0x68ff).ram().w(m_palette, FUNC(palette_device::write8)).share("palette");
	map(0x6c00, 0x6cff).ram().w(m_palette, FUNC(palette_device::write8_ext)).share("palette_ext");
	map(0x7000, 0x7000).r(FUNC(mastboyo_ms_state::irq_ack_r));
	map(0x8000, 0xffff).bankr(m_rombank);
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


static GFXDECODE_START( gfx_mastboyo_ms )
	GFXDECODE_ENTRY( "tiles", 0, gfx_8x8x4_planar, 0, 16 )
GFXDECODE_END


void mastboyo_ms_state::mastboyo_ms(machine_config &config)
{
	// basic machine hardware
	Z80(config, m_maincpu, 20_MHz_XTAL / 6); // Unknown clock and divisor
	m_maincpu->set_addrmap(AS_PROGRAM, &mastboyo_ms_state::main_program_map);
	m_maincpu->set_vblank_int("screen", FUNC(mastboyo_ms_state::irq0_line_assert));

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);

	// video hardware
	screen_device &screen(SCREEN(config, "screen"));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(0));
	screen.set_size(256, 256);
	screen.set_visarea(0, 256-1, 2*8, 256-2*8-1);
	screen.set_screen_update(FUNC(mastboyo_ms_state::screen_update));
	screen.set_palette(m_palette);

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_mastboyo_ms);

	PALETTE(config, m_palette).set_format(palette_device::xBGR_333_nibble, 0x100);

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1(config, m_mod1, 20_MHz_XTAL / 5); // Unknown clock and divisor
	m_mod1->set_decoder(gaelco_ms_mod1_device::decoder::P0101); // PROM not dumped, the program fits this layout
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

	ROM_REGION( 0x8000, "tiles", ROMREGION_INVERT ) // on MOD-4 board
	ROM_LOAD( "ma_0401_27128.bin", 0x00000, 0x02000, CRC(ad9f97d2) SHA1(1b8cde21c5fd5f1970ef6cc20616bff5c36a6b68) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_CONTINUE(                  0x00000, 0x02000)
	ROM_LOAD( "ma_0402_27128.bin", 0x02000, 0x02000, CRC(76d14275) SHA1(6bd920bd58b9b30cd81e2658084639a16314322d) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_CONTINUE(                  0x02000, 0x02000)
	ROM_LOAD( "ma_0403_27128.bin", 0x04000, 0x02000, CRC(58abbe25) SHA1(0747985ceaf74c9f1fd34b6927d2d5dc9e8d8ad9) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_CONTINUE(                  0x04000, 0x02000)
	ROM_LOAD( "ma_0404_27128.bin", 0x06000, 0x02000, CRC(7e9286c2) SHA1(495b065ee5cf0f5a88dee7eb999a54dae53921bd) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_CONTINUE(                  0x06000, 0x02000)

	ROM_REGION( 0x200, "proms", 0 ) // address decoder
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

	ROM_REGION( 0x8000, "tiles", ROMREGION_INVERT ) // on MOD-4 board
	ROM_LOAD( "ma_0401_27128.bin", 0x00000, 0x02000, CRC(ad9f97d2) SHA1(1b8cde21c5fd5f1970ef6cc20616bff5c36a6b68) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_CONTINUE(                  0x00000, 0x02000)
	ROM_LOAD( "ma_0402_27128.bin", 0x02000, 0x02000, CRC(76d14275) SHA1(6bd920bd58b9b30cd81e2658084639a16314322d) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_CONTINUE(                  0x02000, 0x02000)
	ROM_LOAD( "ma_0403_27128.bin", 0x04000, 0x02000, CRC(58abbe25) SHA1(0747985ceaf74c9f1fd34b6927d2d5dc9e8d8ad9) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_CONTINUE(                  0x04000, 0x02000)
	ROM_LOAD( "ma_0404_27128.bin", 0x06000, 0x02000, CRC(7e9286c2) SHA1(495b065ee5cf0f5a88dee7eb999a54dae53921bd) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_CONTINUE(                  0x06000, 0x02000)

	ROM_REGION( 0x200, "proms", 0 ) // address decoder
	ROM_LOAD( "p0314_82s147a.bin", 0x000, 0x200, CRC(aea2bbab) SHA1(506cca2acd8e65e5dbddef3a7e93950d54cf3d5a) )
ROM_END


void mastboyo_ms_state::init_mastboyom()
{
	for (char const *const tag : { "maincpu", "questions" })
	{
		uint8_t *const src = memregion(tag)->base();
		int const len = memregion(tag)->bytes();

		// bitswap data
		for (int i = 0; i < len; i++)
			src[i] = bitswap<8>(src[i], 5, 3, 7, 2, 0, 1, 4, 6);

		// descramble address
		std::vector<uint8_t> const buffer(src, src + len);

		for (int i = 0; i < len; i++)
			src[i] = buffer[bitswap<24>(i, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 10, 8, 6, 4, 0, 2, 7, 3, 1, 9, 11, 5)];
	}
}

} // anonymous namespace


GAME( 1987, mastboyom,  mastboyo, mastboyo_ms, mastboyo_ms, mastboyo_ms_state, init_mastboyom, ROT0, "Gaelco", "Master Boy (1987, Modular System, set 1)", MACHINE_SUPPORTS_SAVE )
GAME( 1987, mastboyoma, mastboyo, mastboyoma,  mastboyo_ms, mastboyo_ms_state, init_mastboyom, ROT0, "Gaelco", "Master Boy (1987, Modular System, set 2)", MACHINE_SUPPORTS_SAVE )
