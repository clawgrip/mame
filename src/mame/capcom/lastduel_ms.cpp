// license:BSD-3-Clause
// copyright-holders:

/*
    Last Duel (Modular System)

    As with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver.

    For this game the Modular System cage contains 7 main boards.
    More information, dip switches and PCB pictures: https://www.recreativas.org/modular-system-last-duel-4358-gaelco-sa

    MOD-6/1   - MC68000P10, 6 ROMs, RAMs, 20 MHz XTAL.
    MOD 1     - Sound board (Z8400BB1, 2 x YM2203C, 2 x Y3014), one EPROM, one PROM, two 8 dip switches banks.
    MOD 4/2   - Tilemap board, has logic + 2 tilemap ROMs, long thin sub-board (C0459/I) with no chips, just routing along one edge.
    MOD 4/3-A - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0460) with no chips, just routing along one edge.
    MOD 4/3-B - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0461) with no chips, just routing along one edge.
    MOD 51/3  - Sprite board, has logic, a PROM, and 4 ROMs.
    MOD 21/1 - Logic, 28.000 MHz XTAL.

    The program is the one of lastduelo with the hardware accesses moved. Every frame it copies the tiles
    around the screen from the tile maps of the original game to the two 16x16 MOD 4 boards, whose tile
    maps are 32 x 16 tiles, and the sprites to the sprite board. It also converts the palette, applying
    the brightness itself, and drops the foreground priority bit of the original game.

    The MOD 21/1 PROMs give a narrower visible area than the original game, so the program moves the
    credits and lives texts 4 columns away from the edge.

    TODO:
    - MOD 1 interrupt source
*/


#include "emu.h"

#include "../gaelco/gaelco_ms_mod1.h"
#include "../gaelco/gaelco_ms_mod21.h"
#include "../gaelco/gaelco_ms_mod4.h"
#include "../gaelco/gaelco_ms_mod51.h"

#include "cpu/m68000/m68000.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"


namespace {

class lastduel_ms_state : public driver_device
{
public:
	lastduel_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mod1(*this, "mod1"),
		m_mod4(*this, "mod4_%c", 'a'),
		m_mod4_tx(*this, "mod4_tx"),
		m_mod51(*this, "mod51"),
		m_screen(*this, "screen"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette"),
		m_spriteram(*this, "spriteram"),
		m_paletteram(*this, "palette", 0x400, ENDIANNESS_BIG),
		m_paletteram_ext(*this, "palette_ext", 0x400, ENDIANNESS_BIG)
	{ }

	void lastduelm(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<gaelco_ms_mod1_device> m_mod1;
	required_device_array<gaelco_ms_mod4_device, 2> m_mod4; // a fg, b bg
	required_device<gaelco_ms_mod4_device> m_mod4_tx;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<screen_device> m_screen;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;

	required_shared_ptr<u16> m_spriteram;
	memory_share_creator<u8> m_paletteram;
	memory_share_creator<u8> m_paletteram_ext;

	u8 m_scroll[3][4]{}; // fg, bg and text boards: x low, x high, y low, y high

	void fg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void bg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	u8 sprite_byte(offs_t offset) const { return m_spriteram[offset >> 1] >> (BIT(offset, 0) ? 0 : 8); }
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void vblank_irq(int state);

	void main_map(address_map &map) ATTR_COLD;
};


void lastduel_ms_state::machine_start()
{
	save_item(NAME(m_scroll));
}


void lastduel_ms_state::fg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// the game only uses colours 8-15 on this layer, the program drops colour bit 3
	color = BIT(data, 12, 3) | 8;
	flags = BIT(data, 15) ? TILE_FLIPY : 0;
}

void lastduel_ms_state::bg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	code = data & 0x07ff;
	color = BIT(data, 11, 4);
	flags = BIT(data, 15) ? TILE_FLIPY : 0;
}

void lastduel_ms_state::tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// the original game format
	code = data & 0x07ff;
	flags = BIT(data, 11) ? TILE_FLIPY : 0;
}

void lastduel_ms_state::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// 4 bytes per sprite (code, y, x, attributes) and the colour 0x200 bytes later
	for (int offs = 0x200 - 4; offs >= 0; offs -= 4)
	{
		u8 const attr = sprite_byte(offs + 3);
		u32 const code = sprite_byte(offs) | ((attr & 0x0f) << 8);
		u32 const color = sprite_byte(offs + 0x200) & 0x0f;
		int const sx = (sprite_byte(offs + 2) | (BIT(~attr, 7) << 8)) - 4;
		int const sy = 0xff - 13 - sprite_byte(offs + 1);

		m_gfxdecode->gfx(0)->transmask(bitmap, cliprect, code, color, BIT(attr, 5), BIT(attr, 6), sx, sy, m_mod51->transmask());
	}
}

uint32_t lastduel_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the program writes the x scroll of the original game plus 36, or 40 for the text board, and the
	// screen bitmap is 4 pixels to the left of the original game's
	for (int i = 0; i < 3; i++)
	{
		gaelco_ms_mod4_device &board = (i < 2) ? *m_mod4[i] : *m_mod4_tx;
		board.set_scrollx((m_scroll[i][0] | (m_scroll[i][1] << 8)) - ((i < 2) ? 36 : 40) + 4);
		board.set_scrolly(m_scroll[i][2] | (m_scroll[i][3] << 8));
	}

	m_mod4[1]->draw(screen, bitmap, cliprect);
	m_mod4[0]->draw(screen, bitmap, cliprect);
	draw_sprites(bitmap, cliprect);
	m_mod4_tx->draw(screen, bitmap, cliprect);

	return 0;
}

void lastduel_ms_state::vblank_irq(int state)
{
	if (state)
		m_maincpu->set_input_line(4, HOLD_LINE);
}


void lastduel_ms_state::main_map(address_map &map)
{
	map(0x000000, 0x05ffff).rom();
	map(0x800000, 0x8003ff).ram().share(m_spriteram);
	map(0x801000, 0x8013ff).rw(m_mod4[0], FUNC(gaelco_ms_mod4_device::vram8_r), FUNC(gaelco_ms_mod4_device::vram8_w));
	map(0x801800, 0x801803).lw8(NAME([this] (offs_t offset, u8 data) { m_scroll[0][offset] = data; }));
	map(0x802000, 0x8023ff).rw(m_mod4[1], FUNC(gaelco_ms_mod4_device::vram8_r), FUNC(gaelco_ms_mod4_device::vram8_w));
	map(0x802800, 0x802803).lw8(NAME([this] (offs_t offset, u8 data) { m_scroll[1][offset] = data; }));
	map(0x803000, 0x80300f).rw(m_mod1, FUNC(gaelco_ms_mod1_device::host16_r), FUNC(gaelco_ms_mod1_device::host16_w));
	map(0xfc0000, 0xfc0fff).ram(); // sprite RAM of the original game, copied to the sprite board
	map(0xfc8000, 0xfc800f).nopw(); // video control of the original game
	map(0xfcc000, 0xfccfff).rw(m_mod4_tx, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0xfcd000, 0xfcd003).lw8(NAME([this] (offs_t offset, u8 data) { m_scroll[2][offset] = data; }));
	map(0xfd0000, 0xfd7fff).ram(); // tile maps of the original game, copied to the MOD 4 boards
	map(0xfd8000, 0xfd83ff).lr8(NAME([this] (offs_t offset) { return m_paletteram[offset]; })).w(m_palette, FUNC(palette_device::write8));
	map(0xfd8400, 0xfd87ff).lr8(NAME([this] (offs_t offset) { return m_paletteram_ext[offset]; })).w(m_palette, FUNC(palette_device::write8_ext));
	map(0xfe0000, 0xffffff).ram();
}


static INPUT_PORTS_START( lastduelm )
	PORT_INCLUDE( gaelco_ms_mod1 )

	// the sound program passes these as the switches of the original game, coin 1 goes where the original has coin 2
	PORT_START("SW1") // MOD 1 YM2203 #2 port B
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:4,5,6")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Difficult ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Difficult ) )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0x04, 0x04, "Stage select" ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x04, "Start from Auto Stage" )
	PORT_DIPSETTING(    0x00, "Start from Plane Stage" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW2:4,5")
	PORT_DIPSETTING(    0x10, "20000 60000 80000" )
	PORT_DIPSETTING(    0x18, "30000 80000 80000" )
	PORT_DIPSETTING(    0x08, "40000 80000 80000" )
	PORT_DIPSETTING(    0x00, "40000 80000 100000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Allow_Continue ) ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW2:8" ) // flip screen in the original game, the program doesn't write the flip register
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

static GFXDECODE_START( gfx_lastduel_ms )
	GFXDECODE_ENTRY( "sprites", 0, sprite_layout, 0x100, 16 )
GFXDECODE_END


void lastduel_ms_state::lastduelm(machine_config &config)
{
	// basic machine hardware
	M68000(config, m_maincpu, 20_MHz_XTAL / 2); // divisor unknown
	m_maincpu->set_addrmap(AS_PROGRAM, &lastduel_ms_state::main_map);

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_screen_update(FUNC(lastduel_ms_state::screen_update));
	m_screen->screen_vblank().set(FUNC(lastduel_ms_state::vblank_irq));
	m_screen->set_palette(m_palette);

	PALETTE(config, m_palette).set_format(palette_device::xBGR_444, 0x400);

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_lastduel_ms);

	// the first visible pixel is column 96 of the original game, the vertical position is a guess
	gaelco_ms_mod21_device &mod21(GAELCO_MS_MOD21(config, "mod21", 28_MHz_XTAL));
	mod21.set_screen(m_screen);
	mod21.set_origin(92, 16);

	GAELCO_MS_MOD51(config, m_mod51);

	GAELCO_MS_MOD4(config, m_mod4[0]);
	m_mod4[0]->set_palette(m_palette);
	m_mod4[0]->set_color_base(0x200);
	m_mod4[0]->set_half_lines(9);
	m_mod4[0]->set_layout(TILEMAP_SCAN_ROWS, 32, 16);
	m_mod4[0]->set_transparent_pen(15);
	m_mod4[0]->set_tile_callback(FUNC(lastduel_ms_state::fg_tile_cb));

	GAELCO_MS_MOD4(config, m_mod4[1]);
	m_mod4[1]->set_palette(m_palette);
	m_mod4[1]->set_color_base(0x300);
	m_mod4[1]->set_half_lines(9);
	m_mod4[1]->set_layout(TILEMAP_SCAN_ROWS, 32, 16);
	m_mod4[1]->set_tile_callback(FUNC(lastduel_ms_state::bg_tile_cb));

	GAELCO_MS_MOD4(config, m_mod4_tx);
	m_mod4_tx->set_palette(m_palette);
	m_mod4_tx->set_tile_size(8);
	m_mod4_tx->set_planes(2);
	m_mod4_tx->set_layout(TILEMAP_SCAN_ROWS, 64, 32);
	m_mod4_tx->set_transparent_pen(3);
	m_mod4_tx->set_tile_callback(FUNC(lastduel_ms_state::tx_tile_cb));

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1(config, m_mod1, 4'000'000); // unknown, no crystal on the board
	m_mod1->set_ym_clock(1'789'773); // unknown, gives the pitch and tempo of the original game
	m_mod1->set_int_period(attotime::from_hz(4 * 60)); // unknown source, the program reads the inputs every 4th interrupt
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<2>().set_ioport("SW2");
	m_mod1->in_callback<3>().set_ioport("SW1");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 0.30);
}

ROM_START( lastduelm )
	ROM_REGION( 0x100000, "maincpu", 0 ) // on MOD 6/1 board
	ROM_LOAD16_BYTE( "mod_6-1_ld_603.ic17",   0x00000, 0x10000, CRC(c3403a02) SHA1(fbcaec5db6c7a06df2244f951b1943d9e40f539e) )
	ROM_LOAD16_BYTE( "mod_6-1_ld_606.ic8",    0x00001, 0x10000, CRC(290a3dc5) SHA1(9a4ca3b525d08505d96ad867dbd75b350a98fdfa) )
	ROM_LOAD16_BYTE( "mod_6-1_ld_602.ic20",   0x20000, 0x10000, CRC(86bb19f9) SHA1(4c549a7cfaad60833a3e3bdd8aee5205cd81d1ff) ) // 1xxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD16_BYTE( "mod_6-1_ld_605.ic11",   0x20001, 0x10000, CRC(039740fa) SHA1(4ac50ffd4d6c080302204f83ace05c8b19a78452) ) // 1xxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD16_BYTE( "mod_6-1_ld_601.ic26",   0x40000, 0x10000, CRC(429fb964) SHA1(78769b05e62c190d846dd08214427d1abbbe2bba) )
	ROM_LOAD16_BYTE( "mod_6-1_ld_604.ic25",   0x40001, 0x10000, CRC(6697615f) SHA1(c977d78f1d89034c0cb7f2f7e922a4d8f334fc70) )

	ROM_REGION( 0x10000, "mod1", 0 ) // on MOD 1 board
	ROM_LOAD( "mod_1_ld_101.ic12",            0x00000, 0x10000, CRC(6bc487c9) SHA1(23e757356894ec2ba5308c9fc9e27bb1e2fd8cad) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "mod_1_116_82s123a.ic20", 0x00, 0x20, CRC(ebcedd2e) SHA1(7810d6f8a1e7474b2eb755cceb6a51ade4567020) )

	ROM_REGION( 0x10000, "mod4_tx", ROMREGION_INVERT ) // on MOD 4/2 board
	ROM_LOAD16_BYTE( "mod_4-2_ld_401.ic17",   0x00000, 0x08000, CRC(f8c8e903) SHA1(f5b390eaf48245e0a1b0c2c54df9777da97ac310) ) //  1ST AND 2ND HALF IDENTICAL
	ROM_LOAD16_BYTE( "mod_4-2_ld_402.ic16",   0x00001, 0x08000, CRC(c1553eb6) SHA1(c92b234c4a748fb34eaa79ad6b60626652ba9877) ) //  1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x80000, "mod4_a", ROMREGION_INVERT ) // on MOD 4/3-A
	ROM_LOAD32_BYTE( "mod_4-3_a_ld_4a01.ic17", 0x00003, 0x20000, CRC(719e9d72) SHA1(9026126f6a6ad0b694de05873f75bd61180cdce1) )
	ROM_LOAD32_BYTE( "mod_4-3_a_ld_4a02.ic16", 0x00002, 0x20000, CRC(4668dfd9) SHA1(5822efd049deede08dcc96d2ace2afa378ab6d56) )
	ROM_LOAD32_BYTE( "mod_4-3_a_ld_4a03.ic15", 0x00001, 0x20000, CRC(372459ad) SHA1(5b5ee17d103748622dcfc57a4b58f40b703a03bf) )
	ROM_LOAD32_BYTE( "mod_4-3_a_ld_4a04.ic14", 0x00000, 0x20000, CRC(81762541) SHA1(a7d8bf6478d239ea8a5d9d0459481d9423c81109) )

	ROM_REGION( 0x40000, "mod4_b", ROMREGION_INVERT ) // on MOD 4/3-B
	ROM_LOAD32_BYTE( "mod_4-3_b_ld_4b01.ic17", 0x00003, 0x10000, CRC(f631aa30) SHA1(a4517749fc15cd2ad6d58aa3da8983f3c07a1d87) )
	ROM_LOAD32_BYTE( "mod_4-3_b_ld_4b02.ic16", 0x00002, 0x10000, CRC(1b0e9136) SHA1(749fb028183025b0d5c622e67e58243f843aa2b8) )
	ROM_LOAD32_BYTE( "mod_4-3_b_ld_4b03.ic15", 0x00001, 0x10000, CRC(31a73db0) SHA1(8339537aeba25740ff5025d288c7a83fac862071) )
	ROM_LOAD32_BYTE( "mod_4-3_b_ld_4b04.ic14", 0x00000, 0x10000, CRC(9f420227) SHA1(39dc101fd9ed9fc26ee0b959adc20268a2a207d0) )

	ROM_REGION( 0x80000, "sprites", ROMREGION_INVERT ) // on MOD 51/3
	ROM_LOAD32_BYTE( "mod_51-3_ld_501.ic43",  0x00003, 0x20000, CRC(d3737ce2) SHA1(62ee62d29eefab51503be1acc796b4233a938ff9) )
	ROM_LOAD32_BYTE( "mod_51-3_ld_502.ic42",  0x00002, 0x20000, CRC(5a9e9d39) SHA1(5e4e521fc9322b1b95baa3ed36d47f2d62ae1383) )
	ROM_LOAD32_BYTE( "mod_51-3_ld_503.ic41",  0x00001, 0x20000, CRC(055a1575) SHA1(fbcf5fa79eaf3b1e363b9ce8b20c17d5fe3f7bf2) )
	ROM_LOAD32_BYTE( "mod_51-3_ld_504.ic40",  0x00000, 0x20000, CRC(227415bc) SHA1(b7bdcfc45a67074d77fa8d5ce8372b2834ce81ce) )

	ROM_REGION( 0x200, "mod21", 0 ) // on MOD 21/1 board
	ROM_LOAD( "mod_21-1_204_82s129.ic4",      0x00000, 0x00100, CRC(74470450) SHA1(40b0e0991090733f8190ad7efcb500bd109c2a7e) )
	ROM_LOAD( "mod_21-1_203_82s129.ic12",     0x00100, 0x00100, CRC(d56e29b4) SHA1(d94157a9cd75d1f6d305f3d48291d0ae4c41e006) )

	ROM_REGION( 0x100, "mod51", 0 ) // on MOD 51/3 board
	ROM_LOAD( "mod_51-3_502_82s129.ic10",     0x00000, 0x00100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )

	ROM_REGION( 0x0200, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "mod_6-1_604_gal16v8.ic13",     0x00000, 0x00117, CRC(8b725626) SHA1(69caeaf3681452e6743605aa05cc5651eabaca3b) )
	ROM_LOAD( "mod_6-1_629_gal16v8.ic7",      0x00000, 0x00117, CRC(54b87ae5) SHA1(846bcf25d3e327f96120bd7346dd5dcae0d9a07d) )
	ROM_LOAD( "mod_4-2_403_gal16v8.ic29",     0x00000, 0x00104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) ) // Same as IC29 on MOD 4/3-A
	ROM_LOAD( "mod_4-3_a_p0403_pal16r8.ic29", 0x00000, 0x00104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) ) // Same as IC29 on MOD 4/2
	ROM_LOAD( "mod_4-3_b_403_gal16v8.ic29",   0x00000, 0x00117, CRC(c136de93) SHA1(116f6d3b456d20621ab07a005c1421f57569915c) )
	ROM_LOAD( "mod_51-3_503_gal16v8.ic46",    0x00000, 0x00117, CRC(11470ea1) SHA1(cfcafbcc7e55be717348f895df61e144fdd0cc9b) )
ROM_END

} // anonymous namespace


GAME( 199?, lastduelm, lastduel, lastduelm, lastduelm, lastduel_ms_state, empty_init, ROT270, "bootleg (Gaelco / Ervisa)", "Last Duel (Modular System)", MACHINE_NOT_WORKING )
