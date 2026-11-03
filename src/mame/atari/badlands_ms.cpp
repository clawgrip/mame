// license:BSD-3-Clause
// copyright-holders:

/*
    Bad Lands (Modular System)

    As with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver.

    For this game the Modular System cage contains 6 main boards and 1 sub board.

    MOD-6/1 - MC68000P10, 4 ROMs, RAMs, 20 MHz XTAL.
    MOD 1/4 - Sound board (Z8400BB1, 2 x YM2203C). 2 8-dips banks + small sub board with OKI M5205.
    MOD 51/1 - Sprite board, has logic + 4 empty ROM sockets. Sprite ROMs are actually on the below board.
    MODULAR SYSTEM 2 MOD 5/1 - red sprite ROM board, 12 sprite ROMs populated (maximum 24 ROMs)
    MOD 4/3 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0477 SOLD) with no chips, just routing along one edge.
    MOD 21/1 - Logic + 2 PROMs.

    The program is the one of badlandsb with the hardware accesses moved. The playfield map is written
    straight to the MOD 4 board, in the order of the original game. Every frame the program converts
    the motion objects into 16x8 and 16x16 sprites for the sprite board and the palette, reversing the
    sprite pens because the sprite board inverts the pixels. The steering wheels are replaced by
    joysticks, read through the MOD 1 board, which also keeps the playfield bank for the program.

    TODO:
    - sprite priority
    - the game comes up in free play with blank EEPROM settings; the DIP switches, which the sound
      program passes where the original game reads the coin and self test inputs, aren't mapped
    - screen timing: the MOD 21/1 PROMs give 450 x 266 with a 336 x 240 visible area, as on Last Duel's
      28 MHz cage
*/


#include "emu.h"

#include "../gaelco/gaelco_ms_mod1.h"
#include "../gaelco/gaelco_ms_mod4.h"

#include "cpu/m68000/m68000.h"
#include "machine/eeprompar.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"


namespace {

class badlands_ms_state : public driver_device
{
public:
	badlands_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mod1(*this, "mod1"),
		m_mod4(*this, "mod4"),
		m_screen(*this, "screen"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette"),
		m_spriteram(*this, "spriteram"),
		m_paletteram(*this, "palette", 0x400, ENDIANNESS_BIG),
		m_paletteram_ext(*this, "palette_ext", 0x400, ENDIANNESS_BIG)
	{ }

	void badlandsm(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<gaelco_ms_mod1_adpcm_device> m_mod1;
	required_device<gaelco_ms_mod4_device> m_mod4;
	required_device<screen_device> m_screen;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;

	required_shared_ptr<u16> m_spriteram;
	memory_share_creator<u8> m_paletteram;
	memory_share_creator<u8> m_paletteram_ext;

	u16 m_scroll[2]{};

	void tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void scroll_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u8 sprite_byte(offs_t offset) const { return m_spriteram[offset >> 1] >> (BIT(offset, 0) ? 0 : 8); }
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void vblank_irq(int state);

	void main_map(address_map &map) ATTR_COLD;
};


void badlands_ms_state::machine_start()
{
	save_item(NAME(m_scroll));
}


void badlands_ms_state::tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// x scroll bit 11 selects the bank of the tiles with bit 12 set, the EPROMs repeat the other ones
	code = (data & 0x0fff) | (BIT(data, 12) << 13) | (BIT(m_scroll[0], 11) << 12);
	color = data >> 13;
}

void badlands_ms_state::scroll_w(offs_t offset, u16 data, u16 mem_mask)
{
	u16 const old = m_scroll[offset];
	COMBINE_DATA(&m_scroll[offset]);
	if (!offset && BIT(old ^ m_scroll[offset], 11))
		m_mod4->mark_all_dirty();
}

void badlands_ms_state::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// 4 bytes per sprite (code, y, x, attributes) and the colour 0x200 bytes later
	for (int offs = 0x200 - 4; offs >= 0; offs -= 4)
	{
		u8 const attr = sprite_byte(offs + 3);
		u8 const color = sprite_byte(offs + 0x200);
		// attribute bit 4 selects the 16x16 sprites, bit 3 the second 16x8 bank; positions lined up with the original game
		u32 const code = (BIT(attr, 3, 2) << 11) | ((attr & 0x07) << 8) | sprite_byte(offs);
		int const sx = 96 + (sprite_byte(offs + 2) | (BIT(~color, 7) << 8));
		int const sy = 241 - sprite_byte(offs + 1);

		m_gfxdecode->gfx(0)->transpen(bitmap, cliprect, code, color & 0x0f, 0, 0, sx, sy, 15);
	}
}

uint32_t badlands_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the program writes 0x1d3 and 0xfff7, the offsets line the playfield up with the original game
	m_mod4->set_scrollx((m_scroll[0] & 0x1ff) - 51);
	m_mod4->set_scrolly(m_scroll[1] - 7);
	m_mod4->draw(screen, bitmap, cliprect);
	draw_sprites(bitmap, cliprect);

	return 0;
}

void badlands_ms_state::vblank_irq(int state)
{
	if (state)
		m_maincpu->set_input_line(4, ASSERT_LINE);
}


void badlands_ms_state::main_map(address_map &map)
{
	// the program still reads the sound status port of the original game at 0xfc0000, nothing is there
	map.unmap_value_high();

	map(0x000000, 0x03ffff).rom();
	map(0x400000, 0x4003ff).ram().share(m_spriteram);
	map(0x440000, 0x440003).w(FUNC(badlands_ms_state::scroll_w));
	map(0xfd0000, 0xfd1fff).rw("eeprom", FUNC(eeprom_parallel_28xx_device::read), FUNC(eeprom_parallel_28xx_device::write)).umask16(0x00ff);
	map(0xfe0000, 0xfe0001).nopw(); // watchdog in the original game
	map(0xfe2000, 0xfe2001).lw16(NAME([this] (u16 data) { m_maincpu->set_input_line(4, CLEAR_LINE); }));
	map(0xfe4000, 0xfe400f).rw(m_mod1, FUNC(gaelco_ms_mod1_device::host16_r), FUNC(gaelco_ms_mod1_device::host16_w));
	map(0xfee000, 0xfee001).w("eeprom", FUNC(eeprom_parallel_28xx_device::unlock_write16));
	map(0xffc000, 0xffc3ff).lr8(NAME([this] (offs_t offset) { return m_paletteram[offset]; })).w(m_palette, FUNC(palette_device::write8));
	map(0xffc400, 0xffc7ff).lr8(NAME([this] (offs_t offset) { return m_paletteram_ext[offset]; })).w(m_palette, FUNC(palette_device::write8_ext));
	map(0xffe000, 0xffefff).rw(m_mod4, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0xfff000, 0xffffff).ram();
}


static INPUT_PORTS_START( badlandsm )
	PORT_INCLUDE( gaelco_ms_mod1 )

	PORT_START("SW1") // MOD 1 YM2203 #2 port B
	PORT_DIPUNKNOWN_DIPLOC(0x01, 0x01, "SW1:1")
	PORT_DIPUNKNOWN_DIPLOC(0x02, 0x02, "SW1:2")
	PORT_DIPUNKNOWN_DIPLOC(0x04, 0x04, "SW1:3")
	PORT_DIPUNKNOWN_DIPLOC(0x08, 0x08, "SW1:4")
	PORT_DIPUNKNOWN_DIPLOC(0x10, 0x10, "SW1:5")
	PORT_DIPUNKNOWN_DIPLOC(0x20, 0x20, "SW1:6")
	PORT_DIPUNKNOWN_DIPLOC(0x40, 0x40, "SW1:7")
	PORT_DIPUNKNOWN_DIPLOC(0x80, 0x80, "SW1:8")

	PORT_START("SW2") // MOD 1 YM2203 #2 port A
	PORT_DIPUNKNOWN_DIPLOC(0x01, 0x01, "SW2:1")
	PORT_DIPUNKNOWN_DIPLOC(0x02, 0x02, "SW2:2")
	PORT_DIPUNKNOWN_DIPLOC(0x04, 0x04, "SW2:3")
	PORT_DIPUNKNOWN_DIPLOC(0x08, 0x08, "SW2:4")
	PORT_DIPUNKNOWN_DIPLOC(0x10, 0x10, "SW2:5")
	PORT_DIPUNKNOWN_DIPLOC(0x20, 0x20, "SW2:6")
	PORT_DIPUNKNOWN_DIPLOC(0x40, 0x40, "SW2:7")
	PORT_DIPUNKNOWN_DIPLOC(0x80, 0x80, "SW2:8")
INPUT_PORTS_END


static const gfx_layout sprite_layout =
{
	16, 16,
	RGN_FRAC(1, 1),
	4,
	{ 0, 8, 16, 24 },
	{ STEP8(0, 1), STEP8(16 * 32, 1) },
	{ STEP16(0, 32) },
	16 * 16 * 4
};

static GFXDECODE_START( gfx_badlands_ms )
	GFXDECODE_ENTRY( "sprites", 0, sprite_layout, 0x100, 16 )
GFXDECODE_END


void badlands_ms_state::badlandsm(machine_config &config)
{
	// basic machine hardware
	M68000(config, m_maincpu, 20_MHz_XTAL / 2); // divisor unknown
	m_maincpu->set_addrmap(AS_PROGRAM, &badlands_ms_state::main_map);

	EEPROM_2816(config, "eeprom").lock_after_write(true);

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_raw(28_MHz_XTAL / 4, 450, 96, 432, 266, 16, 256); // from the MOD 21/1 PROMs, 210 horizontal and 205 vertical
	m_screen->set_screen_update(FUNC(badlands_ms_state::screen_update));
	m_screen->screen_vblank().set(FUNC(badlands_ms_state::vblank_irq));
	m_screen->set_palette(m_palette);

	PALETTE(config, m_palette).set_format(palette_device::xBGR_444, 0x400);

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_badlands_ms);

	GAELCO_MS_MOD4(config, m_mod4);
	m_mod4->set_palette(m_palette);
	m_mod4->set_tile_size(8);
	m_mod4->set_layout(TILEMAP_SCAN_ROWS, 64, 32);
	m_mod4->set_tile_callback(FUNC(badlands_ms_state::tile_cb));

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1_ADPCM(config, m_mod1, 20_MHz_XTAL / 5); // divisor unknown
	m_mod1->set_ym_clock(1'750'000); // unknown, gives the pitch and tempo of the original game
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<2>().set_ioport("SW2");
	m_mod1->in_callback<3>().set_ioport("SW1");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 0.30);
}

ROM_START( badlandsm )
	ROM_REGION( 0x100000, "maincpu", 0 ) // on MOD 6/1 board, very similar to badlandsb
	ROM_LOAD16_BYTE( "mod-6-1_cpu_ba_603a.ic17", 0x00000, 0x10000, CRC(ee03753e) SHA1(42b3f214aa889cb62096016cc214ea7f5dde5e93) )
	ROM_LOAD16_BYTE( "mod-6-1_cpu_ba_606a.ic8",  0x00001, 0x10000, CRC(f22364a2) SHA1(d077bdcad3c0d301007e3fa539039b4a40bbc399) )
	ROM_LOAD16_BYTE( "mod-6-1_cpu_ba_602.ic20",  0x20000, 0x10000, CRC(0e2e807f) SHA1(5b61de066dca12c44335aa68a13c821845657866) )
	ROM_LOAD16_BYTE( "mod-6-1_cpu_ba_605.ic11",  0x20001, 0x10000, CRC(99a20c2c) SHA1(9b0a5a5dafb8816e72330d302c60339b600b49a8) )

	ROM_REGION( 0x10000, "mod1", 0 ) // on MOD 1/4 board
	ROM_LOAD( "mod-1-4_sound_ba_401.ic12",  0x00000, 0x10000, CRC(6cddace7) SHA1(f53c3d6fdec22bc2d39814d482db76aa0fc18af7) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "mod-1-4_sound_105_82s123.ic20", 0x00, 0x20, CRC(14d72781) SHA1(372dc021d8aaf4aa6fd46e69a3d8f1c68113426f) )

	ROM_REGION( 0x80000, "mod4", ROMREGION_INVERT ) // on MOD 4/3 board
	ROM_LOAD32_BYTE( "mod-4-3_ba_401.ic17",  0x00003, 0x20000, CRC(a943e878) SHA1(4330c5599c23818044ccba82331abe31b1d8216a) )
	ROM_LOAD32_BYTE( "mod-4-3_ba_402.ic16",  0x00002, 0x20000, CRC(cbf9f01d) SHA1(390aaed71b247099320118d8b8d59ec10f65a11c) )
	ROM_LOAD32_BYTE( "mod-4-3_ba_403.ic15",  0x00001, 0x20000, CRC(b27dc32a) SHA1(8e56a0a17d505e3a65a5d2c2c6314f21a4630485) )
	ROM_LOAD32_BYTE( "mod-4-3_ba_404.ic14",  0x00000, 0x20000, CRC(263f474f) SHA1(86b6762bcc28117e65cb201c9bc5a12c3f350e46) )

	ROM_REGION( 0xc0000, "sprites", ROMREGION_INVERT ) // on MODULAR SYSTEM 2 MOD 5/1 board: 16x8 motion objects 0-2047, 2048-3071, then pairs as 16x16
	ROM_LOAD32_BYTE( "mod-5-1_ba_503.ic3",   0x00003, 0x10000, CRC(4f3c9a22) SHA1(6ee806aaf663540902a372c04592d2e3030934aa) ) // xxxxxxxxxxxx1xxx = 0x00
	ROM_LOAD32_BYTE( "mod-5-1_ba_512.ic12",  0x00002, 0x10000, CRC(352f2167) SHA1(dd4aea8450e42b6f70801bbb1091df63bffbe463) ) // xxxxxxxxxxxx1xxx = 0x00
	ROM_LOAD32_BYTE( "mod-5-1_ba_518.ic18",  0x00001, 0x10000, CRC(834aa5ef) SHA1(0761110de2a27453fc654970f8a7540fcd2c2615) ) // xxxxxxxxxxxx1xxx = 0x00
	ROM_LOAD32_BYTE( "mod-5-1_ba_524.ic24",  0x00000, 0x10000, CRC(5d7f2231) SHA1(685ab5214c800ef49a6da1cdea5a672a03c281b3) ) // xxxxxxxxxxxx1xxx = 0x00
	ROM_LOAD32_BYTE( "mod-5-1_ba_504.ic4",   0x40003, 0x10000, CRC(690fc050) SHA1(5d77b674ff3fa5701d8fe0f4d4aa4d6821d4da7c) )
	ROM_LOAD32_BYTE( "mod-5-1_ba_513.ic13",  0x40002, 0x10000, CRC(d239ac97) SHA1(4809760dc4c3c3b830402450bd24ccc161bf01f1) )
	ROM_LOAD32_BYTE( "mod-5-1_ba_519.ic19",  0x40001, 0x10000, CRC(bf2cc0f9) SHA1(9175eeac707c04795e1198e8d3a4aac8e64f74d1) ) // xxxxxxxxxxxx1xxx = 0x00
	ROM_LOAD32_BYTE( "mod-5-1_ba_525.ic25",  0x40000, 0x10000, CRC(26d53fc5) SHA1(f0eebbe31fb887ce7c91f5268af2af373ecc88c2) ) // xxxxxxxxxxxx1xxx = 0x00
	ROM_LOAD32_BYTE( "mod-5-1_ba_505.ic5",   0x80003, 0x10000, CRC(a5f91533) SHA1(add36c54a6caec431434b138180d7ca38411d84e) )
	ROM_LOAD32_BYTE( "mod-5-1_ba_514.ic14",  0x80002, 0x10000, CRC(5d3e52e8) SHA1(f8bbc9067058af6ee5ca2ac79c5deba6fd6a366b) )
	ROM_LOAD32_BYTE( "mod-5-1_ba_520.ic20",  0x80001, 0x10000, CRC(f4e132ae) SHA1(c34b4a3b204035e2f4d5209aee48673f758f3ceb) )
	ROM_LOAD32_BYTE( "mod-5-1_ba_526.ic26",  0x80000, 0x10000, CRC(243a6bc8) SHA1(7f3e6031824c28f37afafc80fc9757bb5f80e660) )

	ROM_REGION( 0x300, "proms", 0 )    // PROMs (function unknown)
	ROM_LOAD( "mod-51-1_502_82s129a.ic10",     0x000, 0x100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )
	ROM_LOAD( "mod_21_1_205_82s129.ic12",      0x100, 0x100, CRC(204a7aee) SHA1(322164134aa65c37a9389024f921364a81d13e88) )
	ROM_LOAD( "mod_21_1_210_82s129.ic4",       0x200, 0x100, CRC(d3e676aa) SHA1(241a9f9df88e8f796ca8c773ae7930f6e7ce4db8) )

	ROM_REGION( 0x1000, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "mod-6-1_cpu_644_gal16v8.ic13", 0x000, 0x117, CRC(180a4125) SHA1(bc056d08d3f330cdba4edfdd1544be828e63c2d1) )
	ROM_LOAD( "mod-6-1_cpu_643_gal16v8.ic7",  0x000, 0x117, CRC(d05b2c26) SHA1(51b718554bfba96a3c9e7b3a000fc80679044cb3) )
	ROM_LOAD( "mod-4-3_403_gal16v8.ic29",     0x000, 0x117, CRC(c136de93) SHA1(116f6d3b456d20621ab07a005c1421f57569915c) )
	ROM_LOAD( "mod-51-1_50503_pal16r6.ic46",  0x000, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
	ROM_LOAD( "mod-5-1_5143_gal16v8.ic9",     0x000, 0x117, CRC(2d9cf890) SHA1(b8e3bdfe8543a8729e0397166780036b074f4bc5) )
	ROM_LOAD( "mod-5-1_5242_gal16v8.ic8",     0x000, 0x117, NO_DUMP )
ROM_END

} // anonymous namespace


GAME( 199?, badlandsm, badlands, badlandsm, badlandsm, badlands_ms_state, empty_init, ROT0, "bootleg (Gaelco / Ervisa)", "Bad Lands (Modular System)", MACHINE_NOT_WORKING )
