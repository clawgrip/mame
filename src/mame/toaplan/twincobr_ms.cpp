// license:BSD-3-Clause
// copyright-holders:

/*
    Twin Cobra (Modular System)

    As with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver.

    For this game the Modular System cage contains 7 boards:

    MOD-6/1   - MC68000P10, 3 ROMs, RAMs, 20 MHz Xtal.
    MOD 1/3   - Sound board (Z840006PSC, 2 x YM2203C, 2 x Y3014B, 1 ROM). Two 8 dip switches banks.
    MOD 21/1  - Logic, 2 PROMs, 24 MHz Xtal.
    MOD 4/3   - Tilemap board, has logic + 3 tilemap ROMs, long thin sub-board (C0477 SOLD) with no chips, just routing along one edge.
    MOD 4/3-A - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0477 SOLD) with no chips, just routing along one edge.
    MOD 4/3-B - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0477 SOLD) with no chips, just routing along one edge.
    MOD 51/3  - Sprite board, has logic + 4 ROMs and 1 PROM.

    PCB pics and more info: https://www.recreativas.org/modular-system-twin-cobra-4353-gaelco-sa

    The program is the one of twincobr with the hardware accesses moved. 68000 code replaces the
    TMS320C10 DSP, and the data the original game gets from the sound CPU is in the program ROMs.
    The tile maps are written straight to the MOD 4 boards, in the order of the original game. Every
    frame the program converts the palette, reversing the pens because the MOD 4 and MOD 51 boards
    invert the pixels, and the sprite list, giving 16 sprite palettes to the colours in use. It sets
    the flip screen switch of the original game and undoes the flip on the sprites.

    The second halves of 603 and 606 hold the original second program pair (b30_26_ii.8j and
    b30_27_ii.8h), not used.

    TODO:
    - sprite priority: the program takes the sprite palettes from the bottom for the sprites the
      original game draws in front of the foreground and from the top for the others
    - the program still writes a few colour changes to the palette addresses of the original game
      (0x509e0, 0x50b30 and 0x50bd0), past the palette RAM; its converted copy at 0x2f27e is never
      called
*/


#include "emu.h"

#include "../gaelco/gaelco_ms_mod1.h"
#include "../gaelco/gaelco_ms_mod4.h"

#include "cpu/m68000/m68000.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"


namespace {

class twincobr_ms_state : public driver_device
{
public:
	twincobr_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mod1(*this, "mod1"),
		m_mod4(*this, "mod4_%c", 'a'),
		m_mod4_tx(*this, "mod4_tx"),
		m_screen(*this, "screen"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette"),
		m_spriteram(*this, "spriteram"),
		m_paletteram(*this, "palette", 0x400, ENDIANNESS_BIG),
		m_paletteram_ext(*this, "palette_ext", 0x400, ENDIANNESS_BIG)
	{ }

	void twincobrm(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<gaelco_ms_mod1_device> m_mod1;
	required_device_array<gaelco_ms_mod4_device, 2> m_mod4; // a fg, b bg
	required_device<gaelco_ms_mod4_device> m_mod4_tx;
	required_device<screen_device> m_screen;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;

	required_shared_ptr<u16> m_spriteram;
	memory_share_creator<u8> m_paletteram;
	memory_share_creator<u8> m_paletteram_ext;

	u16 m_scroll[3][2]{}; // fg, bg and text boards: x, y

	void fg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	template <unsigned Which> void scroll_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u8 irq_ack_r();
	u8 sprite_byte(offs_t offset) const { return m_spriteram[offset >> 1] >> (BIT(offset, 0) ? 0 : 8); }
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void vblank_irq(int state);

	void main_map(address_map &map) ATTR_COLD;
};


void twincobr_ms_state::machine_start()
{
	save_item(NAME(m_scroll));
}


void twincobr_ms_state::fg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// y scroll bit 11 selects the EPROM half, the foreground ROM bank of the original game
	code |= BIT(m_scroll[0][1], 11) << 12;
}

void twincobr_ms_state::tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	code = data & 0x07ff;
	color = data >> 11;
}

template <unsigned Which>
void twincobr_ms_state::scroll_w(offs_t offset, u16 data, u16 mem_mask)
{
	u16 const old = m_scroll[Which][offset];
	COMBINE_DATA(&m_scroll[Which][offset]);
	if ((Which == 0) && offset && BIT(old ^ m_scroll[Which][offset], 11))
		m_mod4[0]->mark_all_dirty();
}

u8 twincobr_ms_state::irq_ack_r()
{
	if (!machine().side_effects_disabled())
		m_maincpu->set_input_line(4, CLEAR_LINE);

	return 0xff;
}

void twincobr_ms_state::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// 4 bytes per sprite (code, y, x, attributes) and the colour 0x200 bytes later,
	// positions lined up with the original game
	for (int offs = 0x200 - 4; offs >= 0; offs -= 4)
	{
		u8 const attr = sprite_byte(offs + 3);
		u32 const code = sprite_byte(offs) | ((attr & 0x0f) << 8);
		u32 const color = sprite_byte(offs + 0x200) & 0x0f;
		int const sx = (((sprite_byte(offs + 2) | (BIT(~attr, 7) << 8)) + 199 + 16) & 0x1ff) - 16;
		int const sy = 0xff - 8 - sprite_byte(offs + 1);

		m_gfxdecode->gfx(0)->transpen(bitmap, cliprect, code, color, BIT(attr, 5), BIT(attr, 6), sx, sy, 15);
	}
}

uint32_t twincobr_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the program writes the flip screen scroll of the original game plus 0x70 (x) and minus 0x116 (y)
	for (int i = 0; i < 3; i++)
	{
		gaelco_ms_mod4_device &board = (i < 2) ? *m_mod4[i] : *m_mod4_tx;
		board.set_scrollx(m_scroll[i][0] + 22);
		board.set_scrolly(m_scroll[i][1] - 7);
	}

	m_mod4[1]->draw(screen, bitmap, cliprect);
	m_mod4[0]->draw(screen, bitmap, cliprect);
	draw_sprites(bitmap, cliprect);
	m_mod4_tx->draw(screen, bitmap, cliprect);

	return 0;
}

void twincobr_ms_state::vblank_irq(int state)
{
	if (state)
		m_maincpu->set_input_line(4, ASSERT_LINE);
}


void twincobr_ms_state::main_map(address_map &map)
{
	map(0x000000, 0x02ffff).mirror(0x080000).rom();
	map(0x030000, 0x033fff).ram();
	map(0x040000, 0x041fff).ram(); // the first 4 KB is the sprite RAM of the original game, copied to the sprite board
	map(0x050000, 0x0503ff).lr8(NAME([this] (offs_t offset) { return m_paletteram[offset]; })).w(m_palette, FUNC(palette_device::write8));
	map(0x050400, 0x0507ff).lr8(NAME([this] (offs_t offset) { return m_paletteram_ext[offset]; })).w(m_palette, FUNC(palette_device::write8_ext));
	map(0x054000, 0x055fff).rw(m_mod4_tx, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x056000, 0x056003).w(FUNC(twincobr_ms_state::scroll_w<2>));
	map(0x058000, 0x059fff).rw(m_mod4[0], FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x059ffc, 0x059fff).w(FUNC(twincobr_ms_state::scroll_w<0>));
	map(0x05c000, 0x05dfff).rw(m_mod4[1], FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x05dffc, 0x05dfff).w(FUNC(twincobr_ms_state::scroll_w<1>));
	map(0x060000, 0x0603ff).ram().share(m_spriteram);
	map(0x078000, 0x07800f).rw(m_mod1, FUNC(gaelco_ms_mod1_device::host16_r), FUNC(gaelco_ms_mod1_device::host16_w));
	map(0x07c000, 0x07c000).r(FUNC(twincobr_ms_state::irq_ack_r));
}


static INPUT_PORTS_START( twincobrm )
	PORT_INCLUDE( gaelco_ms_mod1 )

	// the sound program passes these as the switches of the original game, but the 68000 does the coinage
	PORT_START("SW1") // MOD 1 YM2203 #2 port B
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:3,4")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x20, "SW1:6" ) // service mode in the original game
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW1:7" ) // flip screen in the original game, the program sets it
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:8" )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A
	PORT_DIPUNUSED_DIPLOC( 0x01, 0x01, "SW2:1" )
	PORT_DIPUNUSED_DIPLOC( 0x02, 0x02, "SW2:2" ) // DIP switch display in the original game
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW2:3,4")
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW2:5,6")
	PORT_DIPSETTING(    0x30, "50k 200k 150k+" )
	PORT_DIPSETTING(    0x10, "70k 270k 200k+" )
	PORT_DIPSETTING(    0x20, "50k Only" )
	PORT_DIPSETTING(    0x00, "100k Only" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:7,8")
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
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

static GFXDECODE_START( gfx_twincobr_ms )
	GFXDECODE_ENTRY( "sprites", 0, sprite_layout, 0x200, 16 )
GFXDECODE_END


void twincobr_ms_state::twincobrm(machine_config &config)
{
	// basic machine hardware
	M68000(config, m_maincpu, 20_MHz_XTAL / 2); // divisor unknown
	m_maincpu->set_addrmap(AS_PROGRAM, &twincobr_ms_state::main_map);

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_raw(24_MHz_XTAL / 4, 384, 0, 320, 266, 16, 256); // from the MOD 21/1 PROMs, 207 horizontal and 205 vertical
	m_screen->set_screen_update(FUNC(twincobr_ms_state::screen_update));
	m_screen->screen_vblank().set(FUNC(twincobr_ms_state::vblank_irq));
	m_screen->set_palette(m_palette);

	PALETTE(config, m_palette).set_format(palette_device::xBGR_444, 0x400);

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_twincobr_ms);

	GAELCO_MS_MOD4(config, m_mod4[0]);
	m_mod4[0]->set_palette(m_palette);
	m_mod4[0]->set_color_base(0x100);
	m_mod4[0]->set_tile_size(8);
	m_mod4[0]->set_layout(TILEMAP_SCAN_ROWS, 64, 64);
	m_mod4[0]->set_transparent_pen(15);
	m_mod4[0]->set_tile_callback(FUNC(twincobr_ms_state::fg_tile_cb));

	GAELCO_MS_MOD4(config, m_mod4[1]);
	m_mod4[1]->set_palette(m_palette);
	m_mod4[1]->set_color_base(0x300);
	m_mod4[1]->set_tile_size(8);
	m_mod4[1]->set_layout(TILEMAP_SCAN_ROWS, 64, 64);

	GAELCO_MS_MOD4(config, m_mod4_tx);
	m_mod4_tx->set_palette(m_palette);
	m_mod4_tx->set_tile_size(8);
	m_mod4_tx->set_planes(3);
	m_mod4_tx->set_layout(TILEMAP_SCAN_ROWS, 64, 32);
	m_mod4_tx->set_transparent_pen(7);
	m_mod4_tx->set_tile_callback(FUNC(twincobr_ms_state::tx_tile_cb));

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1(config, m_mod1, 20_MHz_XTAL / 5); // divisor unknown
	m_mod1->set_int_period(attotime::from_hz(972)); // unknown source, gives the tempo of the original game (the sound program runs every 10th interrupt)
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<2>().set_ioport("SW2");
	m_mod1->in_callback<3>().set_ioport("SW1");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 0.30);
}

ROM_START( twincobrm )
	ROM_REGION( 0x40000, "maincpu", 0 ) // on MOD 6/1 board
	ROM_LOAD16_BYTE( "mod_6-1_tw_601.ic26",    0x00000, 0x10000, CRC(07f64d13) SHA1(864ce0f9369c40c3ae792fc4ab2444a168214749) )
	ROM_LOAD16_BYTE( "mod_6-1_tw_604.ic25",    0x00001, 0x10000, CRC(41be6978) SHA1(4784804b738a332c7f24a43bcbb7a1e607365735) )
	ROM_LOAD16_BYTE( "mod_6-1_tw_603.ic17",    0x20000, 0x10000, CRC(c50a31ae) SHA1(06fd0f1710de88c72cfece5e8202c9a3c726caaf) ) // 2nd half = b30_26_ii.8j
	ROM_LOAD16_BYTE( "mod_6-1_tw_606.ic8",     0x20001, 0x10000, CRC(fa8df9ed) SHA1(7b713edf6e5a9d2a0a930501b1f66be7eed95074) ) // 2nd half = b30_27_ii.8h

	ROM_REGION( 0x08000, "mod1", 0 ) // on MOD 1/3 board
	ROM_LOAD( "mod_1-3_tw_101.ic12",           0x00000, 0x08000, CRC(e1aafdf5) SHA1(3441fc32c4eba46fa84010a437d5627fe7cdb2d0) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "mod_1-3_112_82s123.ic20", 0x00, 0x20, CRC(e37acb44) SHA1(c3bd11da0a13658edff6e795e112356307e16290) )

	ROM_REGION( 0x18000, "mod4_tx", ROMREGION_INVERT ) // on MOD 4/3 board, IC14 empty
	ROMX_LOAD( "mod_4-3_tw_401.ic17",    0x00002, 0x08000, CRC(da1efca2) SHA1(f8685872b266ff7b9bd657bcc538217159938823), ROM_SKIP(2) ) // 1ST AND 2ND HALF IDENTICAL
	ROMX_LOAD( "mod_4-3_tw_402.ic16",    0x00001, 0x04000, CRC(e9e2d4b1) SHA1(e0a19dd46a9ba85d95bba7fbf81d8dc36dbfeabd), ROM_SKIP(2) )
	ROM_RELOAD(                          0x0c001, 0x04000 )
	ROMX_LOAD( "mod_4-3_tw_403.ic15",    0x00000, 0x08000, CRC(bf4ed7aa) SHA1(b4c479b153c6f146d0d846cec04f0a52cbccef3e), ROM_SKIP(2) ) // 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x40000, "mod4_a", ROMREGION_INVERT ) // on MOD 4/3-A board
	ROM_LOAD32_BYTE( "mod_4-3_a_tw_4a01.ic17", 0x00003, 0x10000, CRC(8cc79357) SHA1(31064df2b796ca85ad3caccf626b684dff1104a1) )
	ROM_LOAD32_BYTE( "mod_4-3_a_tw_4a02.ic16", 0x00002, 0x10000, CRC(13daeac8) SHA1(1cb103f434e2ecf193fa936ca7ea9194064c5b39) )
	ROM_LOAD32_BYTE( "mod_4-3_a_tw_4a03.ic15", 0x00001, 0x10000, CRC(d9e2e55d) SHA1(0409e6df836d1d5198b64b21b42192631aa6d096) )
	ROM_LOAD32_BYTE( "mod_4-3_a_tw_4a04.ic14", 0x00000, 0x10000, CRC(15b3991d) SHA1(f5e7ed7a7721ed7e6dfd440634160390b7a294e4) )

	ROM_REGION( 0x20000, "mod4_b", ROMREGION_INVERT ) // on MOD 4/3-B board
	ROM_LOAD32_BYTE( "mod_4-3_b_tw_4b01.ic17", 0x00003, 0x08000, CRC(44f5accd) SHA1(2f9bdebe71c8be195332356df68992fd38d86994) )
	ROM_LOAD32_BYTE( "mod_4-3_b_tw_4b02.ic16", 0x00002, 0x08000, CRC(170c01db) SHA1(f4c5a1600f6cbb48abbace66c6f7514f79138e8b) )
	ROM_LOAD32_BYTE( "mod_4-3_b_tw_4b03.ic15", 0x00001, 0x08000, CRC(97f20fdc) SHA1(7cb3cd0637b0db889a3d552fd7c1a916eee5ca27) )
	ROM_LOAD32_BYTE( "mod_4-3_b_tw_4b04.ic14", 0x00000, 0x08000, CRC(b5d48389) SHA1(a00c5b9c231d3d580fa20c7ad3f8b6fd990e6594) )

	ROM_REGION( 0x40000, "sprites", ROMREGION_INVERT ) // on MOD 51/3 board
	ROM_LOAD32_BYTE( "mod_51-3_tw_501.ic43",   0x00003, 0x10000, CRC(7cab3a5a) SHA1(2ab994fac033046d85fe9e31dbae80b9530f9a72) )
	ROM_LOAD32_BYTE( "mod_51-3_tw_502.ic42",   0x00002, 0x10000, CRC(0c7e2b95) SHA1(401168061a1c05f8f436d0be5f32b81c527c3837) )
	ROM_LOAD32_BYTE( "mod_51-3_tw_503.ic41",   0x00001, 0x10000, CRC(41de100a) SHA1(cceba1083e89d667cd67a71e992913f8ab314e62) )
	ROM_LOAD32_BYTE( "mod_51-3_tw_504.ic40",   0x00000, 0x10000, CRC(f318d76e) SHA1(c124aec667e98489cc7adcfa4b48b5a147101cb7) )

	ROM_REGION( 0x300, "proms", 0 ) // PROMs (function unknown)
	ROM_LOAD( "mod_21-1_205_tbp24s10.ic12",    0x00000, 0x00100, CRC(204a7aee) SHA1(322164134aa65c37a9389024f921364a81d13e88) )
	ROM_LOAD( "mod_21-1_207_tbp24s10.ic4",     0x00100, 0x00100, CRC(7a63d740) SHA1(5b6d73153d0c058e02c55cbcaa40c4f5e5cb5ce4) )
	ROM_LOAD( "mod_51-3_502_82s129.ic10",      0x00200, 0x00100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )

	ROM_REGION( 0x00117, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "mod_4-3_403_pal16r8.ic29",      0x00000, 0x00104, CRC(16379b0d) SHA1(5379560b0ec7c67cbe131a581a347b86395f34ac) )
	ROM_LOAD( "mod_4-3_a_405_gal16v8.ic29",    0x00000, 0x00117, CRC(9e360ea4) SHA1(095932874ce385e1079194ebdb20cd4933ca6395) ) // Same as on MOD 4/3-B
	ROM_LOAD( "mod_4-3_b_404_gal16v8.ic29",    0x00000, 0x00117, CRC(9e360ea4) SHA1(095932874ce385e1079194ebdb20cd4933ca6395) ) // Same as on MOD 4/3-A
	ROM_LOAD( "mod_51-3_p0503_pal16r6.ic46",   0x00000, 0x00104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
	ROM_LOAD( "mod_6-1_604_gal16v8.ic13",      0x00000, 0x00117, CRC(45b8e164) SHA1(2e227ac5171ebf14e1d1f7214626ef4df0d5ae99) )
	ROM_LOAD( "mod_6-1_624_gal16v8.ic7",       0x00000, 0x00117, CRC(e044b4dc) SHA1(e83211758303a1e1fc50b78f6cfacc9c4b7c71c4) )
ROM_END

} // anonymous namespace


GAME( 199?, twincobrm, twincobr, twincobrm, twincobrm, twincobr_ms_state, empty_init, ROT270, "bootleg (Gaelco / Ervisa)", "Twin Cobra (Modular System)", MACHINE_NOT_WORKING )
