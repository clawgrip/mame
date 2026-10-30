// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Tomás García-Merás Capote (ClawGrip)

/*
    Legend of Hero Tonma (Gaelco bootleg, Modular System)

    As with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver. The boards are drawn
    above the ROM definition.

    The CPU board sees the 16 bytes shared with the MOD 1 sound board (105 decoder PROM, samples
    in the Z80 EPROM) at I/O 0x00-0x0f and 0x20-0x2f.

    TODO:
    - video is the M72 one, which this program mostly fits; colors are wrong and the tilemaps
      don't scroll (I/O 0x1000-0x100e and 0x2000 are written every frame, maybe the MOD 8
      registers)
    - sprites: copying the list at vblank is a guess and they look misplaced
    - clocks
*/

#include "emu.h"

#include "../gaelco/gaelco_ms_mod1.h"

#include "cpu/nec/nec.h"
#include "video/bufsprite.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"


namespace {

class loht_ms_state : public driver_device
{
public:
	loht_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mod1(*this, "mod1"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette"),
		m_spriteram(*this, "spriteram"),
		m_videoram(*this, "videoram%u", 1U),
		m_paletteram(*this, "paletteram%u", 1U)
	{ }

	void lohtm(machine_config &config) ATTR_COLD;

protected:
	virtual void video_start() override ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<gaelco_ms_mod1_adpcm_device> m_mod1;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;
	required_device<buffered_spriteram16_device> m_spriteram;
	required_shared_ptr_array<u16, 2> m_videoram;
	required_shared_ptr_array<u16, 2> m_paletteram;

	tilemap_t *m_fg_tilemap = nullptr;
	tilemap_t *m_bg_tilemap = nullptr;

	template <unsigned Which> u16 palette_r(offs_t offset);
	template <unsigned Which> void palette_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	template <unsigned Which> void videoram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	template <unsigned Which> TILE_GET_INFO_MEMBER(get_tile_info);

	void draw_sprites(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void program_map(address_map &map) ATTR_COLD;
	void io_map(address_map &map) ATTR_COLD;
};


// word 0: fftt tttt tttt tttt, word 1: ---- ---- zz-- pppp (f = flips, t = tile, z = priority, p = palette)
template <unsigned Which>
TILE_GET_INFO_MEMBER(loht_ms_state::get_tile_info)
{
	u16 const code = m_videoram[Which][tile_index * 2];
	u16 const attr = m_videoram[Which][tile_index * 2 + 1];

	tileinfo.set(Which + 1, code & 0x3fff, attr & 0x000f, TILE_FLIPYX(BIT(code, 14, 2)));
	tileinfo.group = BIT(attr, 6, 2);
}

void loht_ms_state::video_start()
{
	m_fg_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(loht_ms_state::get_tile_info<0>)), TILEMAP_SCAN_ROWS, 8, 8, 64, 64);
	m_bg_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(loht_ms_state::get_tile_info<1>)), TILEMAP_SCAN_ROWS, 8, 8, 64, 64);

	m_fg_tilemap->set_transmask(0, 0xffff, 0x0001);
	m_fg_tilemap->set_transmask(1, 0x00ff, 0xff01);
	m_fg_tilemap->set_transmask(2, 0x0001, 0xffff);
	m_fg_tilemap->set_transmask(3, 0x0001, 0xffff);

	m_bg_tilemap->set_transmask(0, 0xffff, 0x0000);
	m_bg_tilemap->set_transmask(1, 0x00ff, 0xff00);
	m_bg_tilemap->set_transmask(2, 0x0001, 0xfffe);
	m_bg_tilemap->set_transmask(3, 0x0001, 0xfffe);

	m_fg_tilemap->set_scrolldy(-128, -128);
	m_bg_tilemap->set_scrolldy(-128, -128);
}

template <unsigned Which>
void loht_ms_state::videoram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_videoram[Which][offset]);
	(Which ? m_bg_tilemap : m_fg_tilemap)->mark_tile_dirty(offset >> 1);
}

// 5 bits per gun in separate blocks, A9 isn't decoded
template <unsigned Which>
u16 loht_ms_state::palette_r(offs_t offset)
{
	return m_paletteram[Which][offset & ~0x100] | 0xffe0;
}

template <unsigned Which>
void loht_ms_state::palette_w(offs_t offset, u16 data, u16 mem_mask)
{
	offset &= ~0x100;
	COMBINE_DATA(&m_paletteram[Which][offset]);

	offset &= 0x0ff;
	m_palette->set_pen_color((Which << 8) + offset,
			pal5bit(m_paletteram[Which][offset + 0x000]),
			pal5bit(m_paletteram[Which][offset + 0x200]),
			pal5bit(m_paletteram[Which][offset + 0x400]));
}

void loht_ms_state::draw_sprites(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	u16 const *const spriteram = m_spriteram->buffer();

	// the list is drawn backwards for the priority buffer
	std::vector<unsigned> spritelist;
	for (unsigned i = 0, w = 0; i < m_spriteram->length(); i += w * 4)
	{
		spritelist.push_back(i);
		w = 1 << BIT(spriteram[i + 2], 14, 2);
	}

	for (auto it = spritelist.rbegin(); spritelist.rend() != it; ++it)
	{
		unsigned const offs = *it;
		u16 const code = spriteram[offs + 1];
		u16 const attr = spriteram[offs + 2];
		u32 const color = attr & 0x0f;
		bool const flipx = BIT(attr, 11);
		bool const flipy = BIT(attr, 10);
		int const w = 1 << BIT(attr, 14, 2);
		int const h = 1 << BIT(attr, 12, 2);
		int const sx = -256 + (spriteram[offs + 3] & 0x3ff);
		int const sy = 384 - (spriteram[offs + 0] & 0x1ff) - 16 * h;

		for (int x = 0; x < w; x++)
		{
			for (int y = 0; y < h; y++)
			{
				u32 const c = code + 8 * (flipx ? (w - 1 - x) : x) + (flipy ? (h - 1 - y) : y);
				m_gfxdecode->gfx(0)->prio_transpen(bitmap, cliprect, c, color, flipx, flipy, sx + 16 * x, sy + 16 * y, screen.priority(), ~1, 0);
			}
		}
	}
}

u32 loht_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	screen.priority().fill(0, cliprect);

	// prepare the screen priority for the sprites
	m_bg_tilemap->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER0, 1);
	m_fg_tilemap->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER0, 1);

	m_bg_tilemap->set_transmask(2, 0xffff, 0x0000);

	// low priority tiles
	m_bg_tilemap->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER1, 0);
	m_fg_tilemap->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER1, 0);

	// high priority tiles
	m_bg_tilemap->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER0, 0);
	m_fg_tilemap->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER0, 0);

	m_bg_tilemap->set_transmask(2, 0x0001, 0xfffe);

	draw_sprites(screen, bitmap, cliprect);

	return 0;
}


void loht_ms_state::program_map(address_map &map)
{
	map(0x00000, 0x7ffff).rom();
	map(0x80000, 0x803ff).ram().share("spriteram");
	map(0xa0000, 0xa3fff).ram();
	map(0xaa800, 0xaafff).ram();
	map(0xc8000, 0xc8bff).rw(FUNC(loht_ms_state::palette_r<0>), FUNC(loht_ms_state::palette_w<0>)).share(m_paletteram[0]);
	map(0xcc000, 0xccbff).rw(FUNC(loht_ms_state::palette_r<1>), FUNC(loht_ms_state::palette_w<1>)).share(m_paletteram[1]);
	map(0xd0000, 0xd3fff).ram().w(FUNC(loht_ms_state::videoram_w<0>)).share(m_videoram[0]);
	map(0xd8000, 0xdbfff).ram().w(FUNC(loht_ms_state::videoram_w<1>)).share(m_videoram[1]);
	map(0xffff0, 0xfffff).rom();
}

void loht_ms_state::io_map(address_map &map)
{
	map(0x0000, 0x000f).mirror(0x0020).rw(m_mod1, FUNC(gaelco_ms_mod1_device::host16_r), FUNC(gaelco_ms_mod1_device::host16_w));
}


static INPUT_PORTS_START( lohtm )
	PORT_INCLUDE( gaelco_ms_mod1 )

	PORT_START("SW1") // MOD 1 YM2203 #2 port B
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPUNUSED_DIPLOC( 0x08, 0x08, "SW1:4" )
	// coin mode 1
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) ) PORT_CONDITION("SW2", 0x04, NOTEQUALS, 0x00) PORT_DIPLOCATION("SW1:5,6,7,8")
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	// coin mode 2
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) PORT_CONDITION("SW2", 0x04, EQUALS, 0x00) PORT_DIPLOCATION("SW1:5,6")
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) PORT_CONDITION("SW2", 0x04, EQUALS, 0x00) PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) ) PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode" ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:4,5")
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x20, 0x20, "Stop Mode" ) PORT_DIPLOCATION("SW2:6") // in stop mode, press 2 to stop and 1 to restart
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability" ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC( 0x80, IP_ACTIVE_LOW, "SW2:8" )
INPUT_PORTS_END


static const gfx_layout tilelayout =
{
	8, 8,
	RGN_FRAC(1, 4),
	4,
	{ RGN_FRAC(3, 4), RGN_FRAC(2, 4), RGN_FRAC(1, 4), RGN_FRAC(0, 4) },
	{ STEP8(0, 1) },
	{ STEP8(0, 8) },
	8 * 8
};

static const gfx_layout spritelayout =
{
	16, 16,
	RGN_FRAC(1, 4),
	4,
	{ RGN_FRAC(3, 4), RGN_FRAC(2, 4), RGN_FRAC(1, 4), RGN_FRAC(0, 4) },
	{ STEP8(0, 1), STEP8(16 * 8, 1) },
	{ STEP16(0, 8) },
	32 * 8
};

static GFXDECODE_START( gfx_loht_ms )
	GFXDECODE_ENTRY( "sprites", 0, spritelayout,   0, 16 )
	GFXDECODE_ENTRY( "tiles0",  0, tilelayout,   256, 16 )
	GFXDECODE_ENTRY( "tiles1",  0, tilelayout,   256, 16 )
GFXDECODE_END


void loht_ms_state::lohtm(machine_config &config)
{
	// basic machine hardware
	V30(config, m_maincpu, 8'000'000); // unknown clock
	m_maincpu->set_addrmap(AS_PROGRAM, &loht_ms_state::program_map);
	m_maincpu->set_addrmap(AS_IO, &loht_ms_state::io_map);

	// video hardware
	BUFFERED_SPRITERAM16(config, m_spriteram);

	screen_device &screen(SCREEN(config, "screen"));
	screen.set_raw(8'000'000, 512, 64, 448, 284, 0, 256); // timings copied from M72
	screen.set_screen_update(FUNC(loht_ms_state::screen_update));
	screen.set_palette(m_palette);
	screen.screen_vblank().set_inputline(m_maincpu, 0);
	screen.screen_vblank().append(m_spriteram, FUNC(buffered_spriteram16_device::vblank_copy_rising));

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_loht_ms);
	PALETTE(config, m_palette).set_entries(512);

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1_ADPCM(config, m_mod1, 4'000'000); // unknown clock
	m_mod1->set_decoder(gaelco_ms_mod1_device::decoder::P105);
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<2>().set_ioport("SW2");
	m_mod1->in_callback<3>().set_ioport("SW1");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 0.30);
}


/*  Legend of Hero Tonma for Ervisa Modular System

 ROM Board (51)
 ___________________________________________________________
 |                      ________________  ________________  |
 |                      |               | |               | |
 |                      | LG 506        | | LG 508        | |
 |                      |_______________| |_______________| |
 |           ::::::::   ________________  ________________  |
 |                      |               | |               | |
 | __                   | LG 505        | | LG 507        | |
 | | |                  |_______________| |_______________| |
 | | |     __________   ________________  ________________  |
 | | |     |_GAL16V8|   |               | |               | |
 | | |                  | EMPTY         | | EMPTY         | |
 | | |                  |_______________| |_______________| |
 | | |     __________   ________________  ________________  |
 | | |     |_GAL16V8|   |               | |               | |
 | | |                  | EMPTY         | | EMPTY         | |
 | | |                  |_______________| |_______________| |
 | | |      _________   ________________  ________________  |
 | | |      |74LS138N   |               | |               | |
 | | |                  | EMPTY         | | EMPTY         | |
 | | |                  |_______________| |_______________| |
 | | |________________  ________________                    |
 | |_||               | |               |                   |
 |    | LG 502        | | LG 504        |                   |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |                   |
 |    | LG 501        | | LG 503        |                   |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |     MODULAR       |
 |    | EMPTY         | | EMPTY         |    SYSTEM 2       |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |                   |
 |    | EMPTY         | | EMPTY         |                   |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |                   |
 |    | EMPTY         | | EMPTY         |                   |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |                   |
 |    | EMPTY         | | EMPTY         |                   |
 |    |_______________| |_______________|                   |
 |__________________________________________________________|

 ROM Board (8)
 __________________________________________________________________________________
 |           :::::::: <- Jumpers                                                   |
 |          ________________  ________________  ________________  ________________ |
 | _______  |               | |               | |               | |               ||
 | 74LS175N | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 |          |_______________| |_______________| |_______________| |_______________||
 | _______  ________________  ________________  ________________  ________________ |
 | 74LS175N |               | |               | |               | |               ||
 |          | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 | _______  |_______________| |_______________| |_______________| |_______________||
 | 74LS175N ________________  ________________  ________________  ________________ |
 |          |               | |               | |               | |               ||
 | _______  | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 | 74LS175N |_______________| |_______________| |_______________| |_______________||
 |          ________________  ________________  ________________  ________________ |
 | _______  |               | |               | |               | |               ||
 | 74LS175N | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 |          |_______________| |_______________| |_______________| |_______________||
 |          ________________  ________________  ________________  ________________ |
 | _______  |               | |               | |               | |               ||
 | 74LS175N | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 |          |_______________| |_______________| |_______________| |_______________||
 |          ________________  ________________  ________________  ________________ |
 | _______  |               | |               | |               | |               ||
 | 74LS175N | LG 802        | | LG 804        | | LG 806        | | LG 808        ||
 |          |_______________| |_______________| |_______________| |_______________||
 | _______  ________________  ________________  ________________  ________________ |
 | 74LS175N |               | |               | |               | |               ||
 |          | LG 801        | | LG 803        | | LG 805        | | LG 807        ||
 |          |_______________| |_______________| |_______________| |_______________||
 |  _____________________________      _______  _________________________________  |
 |__|                            |____74LS138N_|                                |__|
    |____________________________|             |________________________________|

 CPU Board (6)                                       Board 21/1
 _____________________________________________       ______________________________________________
 |             _______________              __|_     |                                             |
 |             |              |             |   |    |                                             |
 |             | KM62245AP-10 |             |   |    |  ________   _______  ________               |
 |             |______________|   _______   |   |    |  |_EMPTY_| |_EMPTY_| |_EMPTY_|              |
 |  _______   ________________    74LS138N  |   |    |                                             |
 | 74LS367AN  |               |             |   |    |                                             |
 |            | LG 606        |             |   |    |                                             |
 |            |_______________|             |   |    |  ________   _______  ________               |
 |            ________________    _______   |   |    |  |74S74N_| |74LS393N |74LS393N              |
 |  _______   |               |   74LS245N  |   |    |                                             |
 | 74LS367AN  | LG 605        |             |   |    |                                             |
 |            |_______________|             |   |    |                                             |
 |            ________________              |   |    |                                             |
 |  _______   |               |   _______   |   |    |  74LS7273N |82S129_| |82S129_|              |
 | 74LS138N   | LG 604        |   74LS374P  |   |    |               209       204                 |
 |            |_______________|             |   |    |                                             |
 |             _______________              |   |    |                                             |
 |             |              |   _______   |___|    |  ________   _______  ________               |
 |  _______    | KM62245AP-10 |   74LS245N    |      |  74LS74AN    XTAL    74LS367AN              |
 |  GAL16V8    |______________|               |      |            28.00000 MHz                     |
 |    686     ________________                |      |                                             |
 |  _______   |               |   _______     |      |  ________   _______  ________               |
 | 74LS174AN  | LG 603        |   74LS374P    |      |  74LS732B1 74LS368AN AM2148-55DC          __|_
 |            |_______________|               |      |                                           |   |
 |  _______   ________________                |      |                                           |   |
 |  GAL16V8   |               |               |      |  ________   _______  ________             |   |
 |   8638     | LG 602        |   _______     |      |  74LS157N  74S112AN  AM2148-55DC          |   |
 |  _______   |_______________|   74LS138N    |      |                                           |   |
 |  GAL16V8   ________________                |      |                                           |   |
 |   8600     |               |               |      |  ________   _______  ________             |   |
 |  _______   | LG 601        |               |      |  74LS148N  74LS368AN AM2148-55DC          |   |
 |  74LS373N  |_______________|   _______     |      |                                           |   |
 |___________________________     74LS32N     |      |                                           |   |
 ||_________________ _______ |                |      |  ________   _______  ________   ________  |   |
 |||                |74LS737N|    _______     |      |  74LS298P  74LS298P  74LS298P   74LS174N  |   |
 ||| NEC V30        |_______ |    74LS20N     |      |                                           |   |
 |||________________|74LS737N|                |      |                                           |   |
 ||__________________________|                |      |  ________   _______  ________   ________  |   |
 |  _______  ______  _______      _______     |      |  74LS245N  74LS245N  |74LS08P   74LS174N  |   |
 |  |74S74N   XTAL  74LS368AN     74LS132N    |      |                                           |___|
 |__________20.000MHz_________________________|      |_____________________________________________|

 Board 5                                             Sound Board 1/3
 _____________________________________________       ______________________________________________
 |                                            |      |                                             |
 | __________  ________ ________ ________     |      |  ________  ________  ________               |
 | |TO SUB 51| 74LS299N 74LS169N UM2149-1     |      |  74LS107AN |74LS3Z_| |_EMPTY_|              |
 | |_________| ________ ________ ________     |      |                                             |
 |             74LS169J 74LS169N UM2149-1     |      |  ________  __________________               |
 |                                            |      | 74LS368AB1 |                 |              |
 | __________  ________ ________ ________     |      |            | EMPTY           |              |
 | |TO SUB 51| 74LS158N 74LS169N |82S129N <- 502     |  ________  |_________________|              |
 | |_________| ________ ________ ________     |      |  74LS74AN  __________________               |
 |             74LS299N 74LS169N 74LS244N     |      |            |  Z8400BB1       |              |
 |                                            |      |  ________  |  Z80 B CPU      |  ________    |
 | __________  ________ ________ ________     |      | 74HCT157E  |_________________|  |CA324E_|   |
 | |TO SUB 51| 74LS299N 74LS169N 74LS244N     |      |            ________________                 |
 | |_________| ________ ________ ________     |      |  ________  |               |             __ |
 |             |74LS20N UM2149-1 74LS298P     |    105->N82S123N  | LG 1          |             |D||
 |                                            |      |  ________  |_______________|             |I||
 | __________  ________ ________ ________     |      | 74HCT157E  ________________              |P||     SOUND SUB
 | |TO SUB 51| 74LS299N UM2149-1 74LS298P     |      | __________ |               | _____ _____ |S|| _________________
 | |_________|                                |      | |         || SRM2064C-15   |Y3014B Y3014B|-|| | ______          |
 |   ________  ________ ________ ________  __ |      | | SOUND   ||_______________|             |D|| | LM358N     ___  |
 |   74LS273P  |74LS00N 74LS86B1 74LS244N  | ||      | |  SUB    |                              |I|| | .......... |  | |
 |                                         | ||      | |         |                              |P|| |            |OKI |
_|_  ________  ________ ________ ________  |T||    __|_|_________|__________________            |S|| | .......... |M5205
|  | 74LS08B1  74LS158N 74LS74AN |74LS20N  |O||    |   |          | YAMAHA          |           |_|| | ________   |__| |
|  |                                       | ||    |   |          | YM2203C         |              | | 74LS377B1       |
|  | ________  ________ ________ ________  |S||    |   |          |_________________|              | |_________________|
IC64->PAL16R6A 74LS393N 74LS368AN 74LS377N |U||    |   |                                           |
|  |                                       |B||    |   | _______  ________                         |
|  | ________  ________ ________ ________  | ||    |   | |EMPTY_| 74LS74AN                         |
|  | 74LS138N  74LS283N AM2148-55DC EMPTY  |5||    |   |                                           |
|  |                                       |1||    |   |                                           |
|  | ________  ________ ________ ________  | ||    |   |          __________________    _____      |
|  | 74LS175P  74LS283N AM2148-55DC 74LS273P ||    |   | _______  | YAMAHA          |  TDA2003     |
|  |                                       | ||    |   | |EMPTY|  | YM2203C         |              |
|  | ________  ________ ________ ________  |_||    |   |          |_________________|              |
|  | 74LS298P  74LS157N 74LS157N 74LS273P     |    |   |                                           |
|  |                                          |    |   |                                           |
|  | ________  ________ ________ ________     |    |   |                                           |
|  | 74LS158N  74LS169N 74LS169N 74LS245P     |    |   |                                           |
|__|                                          |    |___|        __________________________         |
 |____________________________________________|      |__________| |_|_|_|_|_|_|_|_|_|_|_| |________|
                                                                          PRE-JAMMA
 Board 7/4
 __________________________________________________________
 | _________  _________ __________  _________  _________ __|_
 | 74LS163AP| 74LS163AP||_GAL20V8_| |74S174N_| |74S189N_||   |
 |                         7636                          |   |
 | _________  _________  _________  _________  _________ |   |
 | |74LS157N| |74LS288N| |74LS273P| 74LS290B1| |74S189N_||   |
 |                                                       |   |
 | _________  _________  _________  _________  _________ |   |
 | |74LS393N| 74LS283N_|74HCTLS373N 74LS298B1| |74S189N_||   |
 |                                                       |   |
 | _________  __________  _____________  _________       |   |
 | |74LS157N| |_GAL16V8_| |KM62256AP-10| |74S174N_|      |   |
 |              7536      |____________| _________       |   |
 | _________  __________  _____________  |74S174N_|      |   |
 | |74LS283N| |74LS245P_| |KM62256AP-10| __________      |   |
 | _________  __________  |____________| |74LS245P_|     |   |
 | |74LS20B1| |74LS245P_|                                |   |
 | _________  __________    __________   _________       |   |
 | |74LS20B1| |74LS374N_|   |74LS273P_|  |74LS74AP|      |   |
 | _________  __________    __________   _________       |___|
 | |74LS04N_| |_GAL16V8_|   |74LS374N_|  |74LS157N|        |
 |               7336                                      |
 | _________  __________    __________   _________         |
 | |74LS00N_| |_GAL16V8_|   |74LS273P_|  74LS367AN       __|_
 |               7236                                    |   |
 | _________  __________    __________   _________       |   |
 | |74LS08B1| |74LS273P_|   |74LS374N_|  |GAL16V8_|      |   |
 |                                          7436         |   |
 | _________  __________    __________   _________       |   |
 | |74LS74AP| |74LS273P_|   74LS367AB1|  74HCTLS373N     |   |
 |                                                       |   |
 | _________  __________    __________   _________       |   |
 | |74LS32P_| |74LS374N_|   74LS367AB1| GAL20V8-25LP     |   |
 |                                          7136         |   |
 | _________  _________  _________  _________  _________ |   |
 | 74LS139AN  |_74S74N_| |74LS157N| |74LS20B1| 74HCTLS373N   |
 | _________  _________  _________  _________  _________ |   |
 | |74LS32N_| |74LS157N|74HCTLS597N 74HCTLS597N 74HCTLS597N  |
 | _________  _________  _________  _________  _________ |   |
 | |74LS748P| |74LS86N_|74HCTLS597N 74HCTLS597N 74HCTLS597N  |
 | _________  _________  _________  _________  _________ |   |
 | |74LS748P| |74LS86N_|74HCTLS597N 74HCTLS597N 74HCTLS597N  |
 |            _________  _________  _________  _________ |   |
 |            |74LS377N|74HCTLS597N 74HCTLS597N 74HCTLS597N  |
 |            _________  _________  _________  _________ |___|
 |            |74LS273P| |74LS169N| |74LS169N| |74LS169N|  |
 |_________________________________________________________|

*/
ROM_START( lohtm ) // really similar to lohtb, even if it runs on 'Modular Hardware'
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "6_lg_604.ic17", 0x00001, 0x10000, CRC(7e84b6ce) SHA1(a9a54f045917a191b6d6bbe061fc7a3344efb3bb) )
	ROM_LOAD16_BYTE( "6_lg_605.ic20", 0x20001, 0x10000, CRC(abdcd211) SHA1(6ad79c4ef14d908032e4dbded7696a66fe5d31da) )
	ROM_LOAD16_BYTE( "6_lg_601.ic8",  0x00000, 0x10000, CRC(7e080cb8) SHA1(598bcd8ecebe1922735b61cc1305e8839d9d054e) )
	ROM_LOAD16_BYTE( "6_lg_602.ic11", 0x20000, 0x10000, CRC(150d1178) SHA1(71df3c9a49eb74cf40790e9b4cf7c6260fbd07d6) )
	ROM_LOAD16_BYTE( "6_lg_606.ic26", 0x40001, 0x20000, CRC(714778b5) SHA1(e2eaa35d6b5fa5df5163fe0d7b45fa66667f9947) )
	ROM_RELOAD(                       0xc0001, 0x20000 )
	ROM_LOAD16_BYTE( "6_lg_603.ic25", 0x40000, 0x20000, CRC(2f049b03) SHA1(21047cb10912b1fc23795673af3ea7de249328b7) )
	ROM_RELOAD(                       0xc0000, 0x20000 )

	ROM_REGION( 0x10000, "mod1", 0 ) // Z80 program and samples, on MOD 1 board
	ROM_LOAD( "1_lg_1.ic12",  0x00000, 0x10000, CRC(7aa26b54) SHA1(01df7ea6443bbc775d5592edc32816853c857189) )

	ROM_REGION( 0x080000, "sprites", 0 ) // Sprites
	ROM_LOAD( "5_lg_501.ic5",   0x00000, 0x10000, CRC(df5ac5ee) SHA1(5b45417ada402047d97dfb6cee6545686ad26e37) )
	ROM_LOAD( "5_lg_503.ic14",  0x20000, 0x10000, CRC(45220b01) SHA1(83715cf155f91c82067d69f14b3b01ed77777b7d) )
	ROM_LOAD( "5_lg_505.ic20",  0x40000, 0x10000, CRC(25b85cfc) SHA1(c7a9962165379193dc6553ed1f977795a79e0f78) )
	ROM_LOAD( "5_lg_507.ic26",  0x60000, 0x10000, CRC(763fa4ec) SHA1(2d72b1b41f24ae299fde23869942c0b6bbb82363) )
	ROM_LOAD( "5_lg_502.ic6",   0x10000, 0x10000, CRC(d7ecf849) SHA1(ab86a88eae21e054d4e8a740a60c7c6c198232d4) )
	ROM_LOAD( "5_lg_504.ic15",  0x30000, 0x10000, CRC(35d1a808) SHA1(9378ff000104ecfb842b3b884197be82c43a01b4) )
	ROM_LOAD( "5_lg_506.ic21",  0x50000, 0x10000, CRC(464d8579) SHA1(b5981f4865ee5439f0e330091927e6d97d29933f) )
	ROM_LOAD( "5_lg_508.ic27",  0x70000, 0x10000, CRC(a73568c7) SHA1(8fe1867256708cc1ed76d1bed5566b1852b47c40) )

	ROM_REGION( 0x040000, "tiles0", ROMREGION_INVERT ) // tiles #1
	ROM_LOAD( "8_lg_801.ic15",  0x00000, 0x10000, CRC(359f17d4) SHA1(2875ba48395e7faa1a58404475be936dcca45ed1) )
	ROM_LOAD( "8_lg_803.ic22",  0x10000, 0x10000, CRC(73391e8a) SHA1(53ca89b8a10895f817ecdb9fa5eef462edb94ae6) )
	ROM_LOAD( "8_lg_805.ic30",  0x20000, 0x10000, CRC(7096d390) SHA1(f4a16bf8aef7a1a65619ab022cbdb67d2f191888) )
	ROM_LOAD( "8_lg_807.ic37",  0x30000, 0x10000, CRC(1c113901) SHA1(3d4ce2ac6cdad0e1b0a21ffb062f9c92700adcf4) )

	ROM_REGION( 0x040000, "tiles1", ROMREGION_INVERT ) // tiles #2
	ROM_LOAD( "8_lg_802.ic14",  0x00000, 0x10000, CRC(4d5e9b53) SHA1(3e3977bab7a66ed0171afcd555d181960e338749) )
	ROM_LOAD( "8_lg_804.ic21",  0x10000, 0x10000, CRC(4f75a26a) SHA1(79c09a1ad3a6f9cfbd07cb527bbd89d2478ce582) )
	ROM_LOAD( "8_lg_806.ic29",  0x20000, 0x10000, CRC(34854262) SHA1(37436c12579fb41d22a1596b495f065959c14a26) )
	ROM_LOAD( "8_lg_808.ic36",  0x30000, 0x10000, CRC(f923183c) SHA1(a6b578191864aefa81e0cad3ba12a2ca491c91cf) )


	ROM_REGION( 0x320, "prom", ROMREGION_ERASEFF )
	ROM_LOAD( "51_502_82s129n.ic10",      0x000, 0x100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )
	ROM_LOAD( "21_204_82s129an.ic4",      0x100, 0x100, CRC(74470450) SHA1(40b0e0991090733f8190ad7efcb500bd109c2a7e) )
	ROM_LOAD( "21_209_82s129an.ic12",     0x200, 0x100, CRC(7922a7ab) SHA1(e7ec2b1fc61bd7d2cf921bfc50d975f91b938e8a) )
	ROM_LOAD( "1_105_82s123n.ic20",       0x300, 0x020, CRC(14d72781) SHA1(372dc021d8aaf4aa6fd46e69a3d8f1c68113426f) )

	ROM_REGION( 0x104, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "51_p0503_pal16r6.ic46",       0x000, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
	// these were read protected
	ROM_LOAD( "5_5136_gal16v8-25lp.ic9",   0x0, 0x1, NO_DUMP )
	ROM_LOAD( "5_5236_gal16v8-25lp.ic8",   0x0, 0x1, NO_DUMP )
	ROM_LOAD( "6_686_gal16v8.ic13",        0x0, 0x1, NO_DUMP )
	ROM_LOAD( "6_8638_gal16v8.ic7",        0x0, 0x1, NO_DUMP )
	ROM_LOAD( "6_subcpu_8600_gal16v8.ic3", 0x0, 0x1, NO_DUMP )
	ROM_LOAD( "7_7136_gal20v8-25lp.ic7",   0x0, 0x1, NO_DUMP )
	ROM_LOAD( "7_7236_gal20v8-25lp.ic54",  0x0, 0x1, NO_DUMP )
	ROM_LOAD( "7_7336_gal16v8.ic55",       0x0, 0x1, NO_DUMP )
	ROM_LOAD( "7_7436_gal16v8.ic9",        0x0, 0x1, NO_DUMP )
	ROM_LOAD( "7_7536_gal16v8.ic59",       0x0, 0x1, NO_DUMP )
	ROM_LOAD( "7_7636_gal20v8-25lp.ic44",  0x0, 0x1, NO_DUMP )
ROM_END

} // anonymous namespace


GAME( 1989, lohtm, loht, lohtm, lohtm, loht_ms_state, empty_init, ROT0, "bootleg (Gaelco / Ervisa)", "Legend of Hero Tonma (Gaelco bootleg, Modular System)", MACHINE_NOT_WORKING | MACHINE_NO_COCKTAIL | MACHINE_SUPPORTS_SAVE )
