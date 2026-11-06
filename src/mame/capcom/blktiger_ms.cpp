// license:BSD-3-Clause
// copyright-holders:

/*
    Black Tiger (Modular System)

    As with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver.

    The Modular System cage contains 6 main boards for this game.

    MOD3-4 - Z80 board (CPU + 5 ROMs + RAM + 24MHz XTAL).
    MOD21/1 - Logic, 2 PROMs, palette RAM, 20 MHz XTAL.
    MOD1/5 - Sound board (Z80, 2xYM2203C). 2 8-dips banks.
    MOD51/1 - Sprite board, has logic + 4 sprite ROMs.
    MOD 4/3 - Tilemap board, has logic + 2 tilemap ROMs, long thin sub-board (CAR-0484/1 SOLD) with no chips, just routing along one edge.
    MOD 4/3 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (CAR-0484/1 SOLD) with no chips, just routing along one edge.

    The program keeps the whole background map of the original game in 16 KB of RAM and, as the screen
    scrolls, copies the tiles around it to the background MOD 4 board, whose tile map is 32 x 16 tiles.

    The decoder PROM on the CPU sub board (P0322) takes A11-A15, the write signal and bits 0, 2 and 4 of
    the latch at 0xd800. For the 16 KB bank at 0x8000-0xbfff, latch bits 2 and 4 pick the EPROM and bits 0
    and 1 its quarter (bit 1 drives EPROM A15 directly); the program writes the bank of the original game
    plus 8. The latch also selects what appears at 0xc000-0xcfff:
    0x00       background MOD 4 board: tile map at 0xc000-0xc3ff, scroll registers at 0xc400-0xc403
    0x04       sprite board
    0x06       palette, 0xc000 is read once per frame
    0x07       MOD 1 board and palette
    0x10-0x13  background map RAM
    0x17       text MOD 4 board scroll registers at 0xc800-0xc803
    others     palette

    TODO: practically everything:
    - derive from blktiger_state? (eventually, if differences aren't too much)
    - sprite related functions are copied from blktiger.cpp. Verify differences
    - the sprite board (latch 0x04) and the read at 0xc000 with latch 0x06
    - lots of other stuff
*/


#include "emu.h"

#include "../gaelco/gaelco_ms_mod1.h"
#include "../gaelco/gaelco_ms_mod21.h"
#include "../gaelco/gaelco_ms_mod3.h"
#include "../gaelco/gaelco_ms_mod4.h"
#include "../gaelco/gaelco_ms_mod51.h"

#include "cpu/z80/z80.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"


namespace {

class blktiger_ms_state : public driver_device
{
public:
	blktiger_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mod1(*this, "mod1"),
		m_mod3(*this, "mod3"),
		m_mod4_bg(*this, "mod4_bg"),
		m_mod4_tx(*this, "mod4_tx"),
		m_mod51(*this, "mod51"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette"),
		m_gfxdecode(*this, "gfxdecode"),
		m_rom(*this, "maincpu"),
		m_ram(*this, "ram", 0x2000, ENDIANNESS_LITTLE),
		m_mapram(*this, "mapram", 0x4000, ENDIANNESS_LITTLE),
		m_paletteram(*this, "palette", 0x400, ENDIANNESS_LITTLE),
		m_paletteram_ext(*this, "palette_ext", 0x400, ENDIANNESS_LITTLE)
	{ }

	void blktigerm(machine_config &config);

	void init_blktigerm();

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<gaelco_ms_mod1_device> m_mod1;
	required_device<gaelco_ms_mod3_decoder_device> m_mod3;
	required_device<gaelco_ms_mod4_device> m_mod4_bg;
	required_device<gaelco_ms_mod4_device> m_mod4_tx;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;
	required_device<gfxdecode_device> m_gfxdecode;

	required_region_ptr<u8> m_rom;
	memory_share_creator<u8> m_ram;
	memory_share_creator<u8> m_mapram;
	memory_share_creator<u8> m_paletteram;
	memory_share_creator<u8> m_paletteram_ext;

	u8 m_bg_scroll[4]{};
	u8 m_tx_scroll[4]{};

	TILEMAP_MAPPER_MEMBER(bg_scan);
	void bg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch);
	void install_palette(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write);
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void main_map(address_map &map) ATTR_COLD;
};


void blktiger_ms_state::main_map(address_map &map)
{
	map(0x0000, 0xffff).m(m_mod3, FUNC(gaelco_ms_mod3_decoder_device::map));
}

void blktiger_ms_state::install_palette(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write)
{
	// A10 selects the blue byte
	for (offs_t area = start; area < end; area += 0x400)
	{
		if (write)
		{
			view.install_write_handler(area, area + 0x3ff, BIT(area, 10) ?
					write8sm_delegate(*m_palette, FUNC(palette_device::write8_ext)) :
					write8sm_delegate(*m_palette, FUNC(palette_device::write8)));
		}
		else
		{
			view.install_rom(area, area + 0x3ff, BIT(area, 10) ? &m_paletteram_ext[0] : &m_paletteram[0]);
		}
	}
}

void blktiger_ms_state::mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch)
{
	// the boards at 0xc000-0xcfff tell from the latch whether they are selected
	bool const map_ram = BIT(latch, 4) && !BIT(latch, 2);
	bool const bg_board = !BIT(latch, 4) && ((latch & 0x0f) == 0x00);
	bool const mod1 = !BIT(latch, 4) && ((latch & 0x0f) == 0x07);
	u8 *const map_bank = &m_mapram[(latch & 0x03) << 12];

	if (BIT(code, 7))
	{
		offs_t const low = (BIT(code, 2, 2) << 13) | (start & 0x1fff);
		switch (BIT(code, 4, 3))
		{
		case 0: // banked EPROMs 305, 304, 302 and 303, latch bit 1 drives A15
		case 1:
		case 2:
		case 3:
			if (!write)
			{
				static constexpr offs_t base[4] = { 0x30000, 0x40000, 0x10000, 0x20000 };
				view.install_rom(start, end, &m_rom[base[BIT(code, 4, 2)] | (BIT(latch, 1) << 15) | low]);
			}
			break;

		case 5: // 301, output 0 clocks the latch
			if (!write)
				view.install_rom(start, end, &m_rom[low]);
			else if (!BIT(code, 0))
				view.install_write_handler(start, end, write8smo_delegate(*m_mod3, FUNC(gaelco_ms_mod3_decoder_device::latch_w)));
			break;

		case 6: // 8 KB RAM, the sprite list at the top
			if (write)
				view.install_writeonly(start, end, &m_ram[start & 0x1fff]);
			else
				view.install_rom(start, end, &m_ram[start & 0x1fff]);
			break;

		case 7: // writes to the boards at 0xc000-0xcfff
			for (offs_t area = start; area < end; area += 0x400)
			{
				switch (BIT(area, 10, 2))
				{
				case 0:
					if (bg_board)
						view.install_write_handler(area, area + 0x3ff, write8sm_delegate(*m_mod4_bg, FUNC(gaelco_ms_mod4_device::vram8_w)));
					else if (mod1)
						view.install_write_handler(area, area + 0x00f, write8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_device::host_w)));
					break;
				case 1:
					if (bg_board)
						view.install_write_handler(area, area + 0x003, write8sm_delegate(*this, NAME([this] (offs_t offset, u8 data) { m_bg_scroll[offset] = data; })));
					break;
				default:
					install_palette(view, area, area + 0x3ff, true);
					break;
				}
			}
			break;
		}
		return;
	}

	switch (code)
	{
	case 0x71: // background MOD 4 board or MOD 1 board
		if (write)
			break;
		if (bg_board)
			view.install_read_handler(start, start + 0x3ff, read8sm_delegate(*m_mod4_bg, FUNC(gaelco_ms_mod4_device::vram8_r)));
		else if (mod1)
			view.install_read_handler(start, start + 0x00f, read8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_device::host_r)));
		break;

	case 0x61: // palette, or the upper half of a background map RAM bank
		if (write)
			break;
		if (!BIT(latch, 4))
			install_palette(view, start, end, false);
		else if (map_ram)
			view.install_rom(start, end, &map_bank[start & 0xfff]);
		break;

	case 0x65: // background map RAM
	case 0x6b:
		if (!map_ram)
			break;
		if (write)
			view.install_writeonly(start, end, &map_bank[start & 0xfff]);
		else
			view.install_rom(start, end, &map_bank[start & 0xfff]);
		break;

	case 0x6d: // text MOD 4 board, its scroll registers at 0xc800-0xc803
		if (BIT(start, 12))
		{
			for (offs_t area = start; area < end; area += 0x400)
			{
				if (write)
				{
					view.install_write_handler(area, area + 0x3ff, BIT(area, 10) ?
							write8sm_delegate(*m_mod4_tx, FUNC(gaelco_ms_mod4_device::attr_w)) :
							write8sm_delegate(*m_mod4_tx, FUNC(gaelco_ms_mod4_device::code_w)));
				}
				else
				{
					view.install_read_handler(area, area + 0x3ff, BIT(area, 10) ?
							read8sm_delegate(*m_mod4_tx, FUNC(gaelco_ms_mod4_device::attr_r)) :
							read8sm_delegate(*m_mod4_tx, FUNC(gaelco_ms_mod4_device::code_r)));
				}
			}
		}
		else if (write)
		{
			view.install_write_handler(start, start + 0x003, write8sm_delegate(*this, NAME([this] (offs_t offset, u8 data) { m_tx_scroll[offset] = data; })));
		}
		break;
	}
}

void blktiger_ms_state::machine_start()
{
	save_item(NAME(m_bg_scroll));
	save_item(NAME(m_tx_scroll));
}

void blktiger_ms_state::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	u8 const *const spriteram = &m_ram[0x1e00];
	for (int offs = 0x200 - 4; offs >= 0; offs -= 4)
	{
		int attr = spriteram[offs + 1];
		int sx = spriteram[offs + 3] - ((attr & 0x10) << 4);
		int sy = spriteram[offs + 2];
		int code = spriteram[offs] | ((attr & 0xe0) << 3);
		int color = attr & 0x07;
		int flipx = attr & 0x08;

		if (flip_screen())
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
		}

		m_gfxdecode->gfx(0)->transmask(bitmap, cliprect,
				code,
				color,
				flipx, flip_screen(),
				sx, sy, m_mod51->transmask());
	}
}

uint32_t blktiger_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the program writes the scroll of the original game minus 26 and 1
	m_mod4_bg->set_scrollx(26 + (m_bg_scroll[0] | (m_bg_scroll[1] << 8)));
	m_mod4_bg->set_scrolly(1 + (m_bg_scroll[2] | (m_bg_scroll[3] << 8)));
	m_mod4_tx->set_scrollx(26 + (m_tx_scroll[0] | (m_tx_scroll[1] << 8)));
	m_mod4_tx->set_scrolly(1 + (m_tx_scroll[2] | (m_tx_scroll[3] << 8)));

	bitmap.fill(m_palette->black_pen(), cliprect);

	m_mod4_bg->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER1);
	draw_sprites(bitmap, cliprect);
	m_mod4_bg->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER0);
	m_mod4_tx->draw(screen, bitmap, cliprect);

	return 0;
}

TILEMAP_MAPPER_MEMBER(blktiger_ms_state::bg_scan)
{
	// two pages of 16 x 16 tiles
	return (col & 0x0f) | ((row & 0x0f) << 4) | ((col & 0x10) << 4);
}

void blktiger_ms_state::bg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// the original game format, the program copies the entries unchanged
	code = data & 0x07ff;
	color = BIT(data, 11, 4);
	flags = BIT(data, 15) ? TILE_FLIPX : 0;
}

void blktiger_ms_state::tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	code = (data & 0xff) | ((data & 0xe000) >> 5);
	color = BIT(data, 8, 5);
}

static INPUT_PORTS_START( blktigerm )
	PORT_INCLUDE( gaelco_ms_mod1 )

	PORT_START("SW1") // MOD 1 YM2203 #2 port B, the sound program sets bits 6-7
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:4,5,6")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW1:7" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:8" )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A, the sound program clears bit 7
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7")
	PORT_DIPNAME( 0x1c, 0x0c, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:3,4,5")
	PORT_DIPSETTING(    0x1c, "1 (Easiest)")
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x14, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x0c, "5 (Normal)" )
	PORT_DIPSETTING(    0x08, "6" )
	PORT_DIPSETTING(    0x04, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hardest)" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Allow_Continue ) ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW2:8" )
INPUT_PORTS_END

static const gfx_layout tiles16x16x4_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7, 512,513,514,515,516,517,518,519 },
	{ STEP16(0,32) },
	16 * 16 * 4
};

static GFXDECODE_START( gfx_blktiger_ms )
	GFXDECODE_ENTRY( "sprites", 0, tiles16x16x4_layout, 0x200, 32 )
GFXDECODE_END

void blktiger_ms_state::blktigerm(machine_config &config)
{
	// basic machine hardware
	Z80(config, m_maincpu, 24_MHz_XTAL / 4); // divisor unknown
	m_maincpu->set_addrmap(AS_PROGRAM, &blktiger_ms_state::main_map);

	using decoder = gaelco_ms_mod3_decoder_device;
	GAELCO_MS_MOD3_DECODER(config, m_mod3);
	m_mod3->set_lines({ decoder::WRITE, decoder::A(11), decoder::A(12), decoder::A(13), decoder::A(14), decoder::A(15), decoder::LATCH(0), decoder::LATCH(2), decoder::LATCH(4) });
	m_mod3->set_latch_mask(0x1f);
	m_mod3->set_install_callback(FUNC(blktiger_ms_state::mod3_install));
	m_maincpu->set_vblank_int("screen", FUNC(blktiger_ms_state::irq0_line_hold));

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_screen_update(FUNC(blktiger_ms_state::screen_update));
	m_screen->set_palette("palette");

	// NOTE: swapped R and G compared to original set
	PALETTE(config, "palette").set_format(palette_device::xBGR_444, 1024);

	GFXDECODE(config, "gfxdecode", "palette", gfx_blktiger_ms);

	gaelco_ms_mod21_device &mod21(GAELCO_MS_MOD21(config, "mod21", 20_MHz_XTAL));
	mod21.set_screen(m_screen);
	mod21.set_origin(0, 16);

	GAELCO_MS_MOD51(config, m_mod51);

	GAELCO_MS_MOD4(config, m_mod4_bg);
	m_mod4_bg->set_palette(m_palette);
	m_mod4_bg->set_color_base(0x100);
	m_mod4_bg->set_half_lines(9);
	m_mod4_bg->set_layout(FUNC(blktiger_ms_state::bg_scan), 32, 16);
	m_mod4_bg->set_transparent_pen(15);
	m_mod4_bg->set_tile_callback(FUNC(blktiger_ms_state::bg_tile_cb));

	GAELCO_MS_MOD4(config, m_mod4_tx);
	m_mod4_tx->set_palette(m_palette);
	m_mod4_tx->set_tile_size(8);
	m_mod4_tx->set_planes(2);
	m_mod4_tx->set_transparent_pen(3);
	m_mod4_tx->set_tile_callback(FUNC(blktiger_ms_state::tx_tile_cb));

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1(config, m_mod1, 4'000'000); // unknown, no crystal on the board
	m_mod1->set_ym_clock(1'789'773); // unknown, gives the pitch of the original game
	m_mod1->set_int_period(attotime::from_hz(950)); // unknown source, about 950 Hz gives the tempo of the original game
	m_mod1->set_host_nmi(true);
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<2>().set_ioport("SW2");
	m_mod1->in_callback<3>().set_ioport("SW1");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 0.30);
}

ROM_START( blktigerm )
	ROM_REGION( 0x50000, "maincpu", 0 ) // on MOD 3/4 board
	ROM_LOAD( "3_bl_301.ic14",   0x00000, 0x08000, CRC(b4525312) SHA1(958ce9a7f41422f3011412e4a16dd3ad65019733) )
	ROM_LOAD( "3_bl_302_c.ic13", 0x10000, 0x10000, CRC(c943415f) SHA1(08e82d2557de01c979a0a58b5433e9bdcdb118cb) )
	ROM_LOAD( "3_bl_303_b.ic12", 0x20000, 0x10000, CRC(72114f6b) SHA1(049e747431cd95db2e7414467754424f68a8d757) )
	ROM_LOAD( "3_bl_305.ic19",   0x30000, 0x10000, CRC(b30a99af) SHA1(19d6dbe11ffeb9a21dbfe75eb39ce9ceaef5eb31) )
	ROM_LOAD( "3_bl_304.ic18",   0x40000, 0x10000, CRC(36f669c6) SHA1(e91fd222919904d4d4b3ef70a22ab599a7cfa263) )

	ROM_REGION( 0x8000, "mod1", 0 )
	ROM_LOAD( "1_bl_101.ic12",  0x0000, 0x8000, CRC(14028686) SHA1(64dc219d906f1bdd0c2bc05aff5aa73e001a6901) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "1_p0101_82s123.ic20", 0x00, 0x20, CRC(3fd60d3a) SHA1(8100fa7453638ac40193b5d92404f41b101ed2cc) )

	ROM_REGION( 0x8000, "mod4_tx", ROMREGION_INVERT ) // on one of the MOD 4/3 boards, both ROMs 0xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD16_BYTE( "4_bl_401.ic17",  0x0001, 0x4000, CRC(ab1afb3d) SHA1(555332ccfb69e65b776f94ffac9a4a051fb6f09e) )
	ROM_CONTINUE(                      0x0001, 0x4000 )
	ROM_LOAD16_BYTE( "4_bl_402.ic16",  0x0000, 0x4000, CRC(89445b11) SHA1(7d9ab6e88d7de3a0e31b3b8a5e57dd39afd7940d) )
	ROM_CONTINUE(                      0x0000, 0x4000 )

	ROM_REGION( 0x40000, "mod4_bg", ROMREGION_INVERT ) // on the other MOD 4/3 board
	ROM_LOAD32_BYTE( "4_a_bl_4a01.ic17",  0x00003, 0x10000, CRC(fc00fd6c) SHA1(066714421fa782c6cfad9f695f05e6e3551ffdc7) )
	ROM_LOAD32_BYTE( "4_a_bl_4a02.ic16",  0x00002, 0x10000, CRC(49cd8afa) SHA1(147b65dad4f05b940d1f7a4028f5748ba1b7daa5) )
	ROM_LOAD32_BYTE( "4_a_bl_4a03.ic15",  0x00001, 0x10000, CRC(50207dd4) SHA1(bee343fbefc817fa14b8b303f013bf5b89e911f8) )
	ROM_LOAD32_BYTE( "4_a_bl_4a04.ic14",  0x00000, 0x10000, CRC(4bb0c1ba) SHA1(9433c78f0de7cb1f92b22612b9f3c51d813b025a) )

	ROM_REGION( 0x20, "mod4_bg:prom", 0 ) // priority
	ROM_LOAD( "4_a_bl_82s123.ic13", 0x00, 0x20, CRC(671f1183) SHA1(dbc21c3922c5e69340daa9008e1200f1304b3e8f) )

	ROM_REGION( 0x40000, "sprites", ROMREGION_INVERT ) // on MOD 51/1 board
	ROM_LOAD32_BYTE( "51_bl_501.ic43",   0x00003, 0x10000, CRC(ace32b94) SHA1(4a09e8dc73bd16f7d378aefbe5d433dd1396f97d) )
	ROM_LOAD32_BYTE( "51_bl_502.ic42",   0x00002, 0x10000, CRC(f6c4cc0b) SHA1(678fd71e90237f8e19eae78fe150a4c5d3494c6c) )
	ROM_LOAD32_BYTE( "51_bl_503.ic41",   0x00001, 0x10000, CRC(61e3ae0b) SHA1(cbb83827f027acd6b905f4210476810bcd4b03a9) )
	ROM_LOAD32_BYTE( "51_bl_504.ic40",   0x00000, 0x10000, CRC(2cb45034) SHA1(b2fd3b7b7b9d68b6ff1985166b106e65b17dbb23) )

	ROM_REGION( 0x100, "mod51", 0 ) // on MOD 51/1 board
	ROM_LOAD( "51_p0502_82s129.ic10",      0x000, 0x0100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )

	ROM_REGION( 0x200, "mod21", 0 ) // on MOD 21/1 board
	ROM_LOAD( "21_p0201_82s129.ic4",       0x000, 0x0100, CRC(2697da58) SHA1(e62516b886ff6e204b718e5f0c6ce2712e4b7fc5) )
	ROM_LOAD( "21_p0202_82s129.ic12",      0x100, 0x0100, CRC(e434128a) SHA1(ef0f6d8daef8b25211095577a182cdf120a272c1) )

	ROM_REGION( 0x0200, "mod3", 0 ) // on the CPU sub board, address decoder
	ROM_LOAD( "3_subcpu_p0322_82s147.bin", 0x0000, 0x0200, CRC(6d427336) SHA1(6ce1ef41df2b450fafe11c3021991618bdb771ed) )

	ROM_REGION( 0x1000, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "4_p0403_pal16r8a-2cn.ic29",      0x000, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "4_a_p0403_pal16r8a-2cn.ic29",    0x000, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) ) // identical to the above one (same PCB type)
	ROM_LOAD( "51_p0503_pal16r8a.ic56",         0x000, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
ROM_END

void blktiger_ms_state::init_blktigerm()
{
	gaelco_ms_mod3_descramble(&m_rom[0], m_rom.bytes());
}

} // Anonymous namespace


GAME( 1991, blktigerm,  blktiger,  blktigerm,  blktigerm,  blktiger_ms_state, init_blktigerm, ROT0, "bootleg (Gaelco / Ervisa)", "Black Tiger (Modular System)", MACHINE_NOT_WORKING )
