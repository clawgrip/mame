// license:BSD-3-Clause
// copyright-holders:

/*
    Fighting Fantasy (Modular System)
    Dragon Ninja (Modular System)
    Secret Agent (Modular System)
    Automat (Modular System) [bootleg of Robocop]

    As with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver.


    For Fighting Fantasy the Modular System cage contains 8 main boards and 1 sub board.

    MOD-6/1 - TSC68000CP12, 4 ROMs, RAMs, 20 MHz XTAL.
    MOD 21/1(?) - 20 MHz XTAL.
    MOD 1/5 - Sound board (Z80B, 2 x YM2203C). 2 8-dips banks + small sub board with OKI M5205.
    MOD 51/3 - Sprite board, has logic + 4 empty ROM sockets. Sprite ROMs are actually on the below board.
    MODULAR SYSTEM 2 MOD 5/1 - red sprite ROM board, 8 sprite ROMs populated (maximum 24 ROMs)
    MOD 4/3 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0467 SOLD) with no chips, just routing along one edge.
    MOD 4/3 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0468 SOLD) with no chips, just routing along one edge.
    MOD 4/3 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0469 SOLD) with no chips, just routing along one edge.

    PCBs pictures and dip listing are available at: http://www.recreativas.org/modular-system-fighting-fantasy-5694-gaelco-sa


    For Dragon Ninja the Modular System cage contains 7 main boards and 1 sub board.

    MOD-6/1 - MC68000P10, 2 ROMs, RAMs, 20 MHz XTAL.
    MOD 21/1(?) - 20 MHz XTAL.
    MOD 1/B 4 - Sound board (Z0840006PSC, 2 x YM2203C). 2 8-dips banks + small sub board with OKI M5205.
    MOD 51/3 - Sprite board, has logic + 4 ROM, short thin sub-board (C0528) with no chips, just routing along one edge.
    MOD 4/3 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0462) with no chips, just routing along one edge.
    MOD 4/3 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0463) with no chips, just routing along one edge.
    MOD 4/2 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0464) with no chips, just routing along one edge.

    PCBs pictures and dip listing are available at: https://www.recreativas.org/modular-system-dragon-ninja-4319-gaelco-sa


    For Secret Agent the Modular System cage contains 8 main boards and 1 sub board.

    MOD-6/1 - MC68000P10, 4 ROMs, RAMs, 22.1184 MHz XTAL.
    MOD 21/1(?) - 20 MHz XTAL.
    MOD 1/2 - Sound board (NEC D780C-2 Z80 compatible, 2 x YM2203C, 2 x Y3014B). 2 8-dips banks + small sub board with OKI M5205.
    MOD 51/3 - Sprite board, has logic + 4 empty ROM sockets. Sprite ROMs are actually on the below board.
    MODULAR SYSTEM 2 MOD 5/1 - red sprite ROM board, 8 sprite ROMs populated (maximum 24 ROMs)
    MOD 4/3 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0462) with no chips, just routing along one edge.
    MOD 4/3 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0463) with no chips, just routing along one edge.
    MOD 4/2 - Tilemap board, has logic + 4 tilemap ROMs, long thin sub-board (C0464) with no chips, just routing along one edge.

    PCBs pictures and dip listing are available at: https://www.recreativas.org/modular-system-secret-agent-11088-gaelco-sa


    For Automat the Modular System cage contains 6 main boards and 1 sub board.

    MOD 1/5 - Sound board (Z80B, 2 x YM2203C). Two 8 dip switches banks + small sub board with OKI M5205.
    MOD 6/1 - TSC68000CP12, 4 ROMs, RAMs, 20 MHz XTAL.
    MOD 2   - Logic + 20 MHz XTAL.
    MOD 7/8 - Logic + RAM (2 x + Sony CXK58256PM-10) + PLDs.
    MOD 8   - ROM (eight EPROMs)
    MOD 51/3 - Sprite board, has logic + 4 EPROMs.

    PCBs pictures and dip listing are available at: https://www.recreativas.org/modular-system-automat-robocop-14508-gaelco-sa


    The Dragon Ninja, Fighting Fantasy and Secret Agent programs are the ones of the original games with
    the hardware accesses moved. The tile maps stay in the RAM layout of the BAC06 chips of the original
    games, pages of 16 x 16 tiles, two by two on Dragon Ninja and Secret Agent and side by side on
    Fighting Fantasy, as the original games set them; the MOD 4 boards show them through their edge sub
    boards. Every frame the programs convert the sprites of the original games into the sprite board
    list, and write the scroll values into the RAM areas of the MOD 4 boards, low byte at the even
    address. Dragon Ninja writes sixteen X values for each board, one for each band of 16 lines, and the
    Y value after them; Secret Agent does the same on its front 16x16 board. The palettes are converted
    with the pens reversed, as the boards invert the EPROM pixels.

    With the fitted positions of the MOD 4 and MOD 51 boards (see those devices), Secret Agent matches the
    original game horizontally; the other two programs write other scroll offsets for each board, which
    put their layers up to 4 pixels away from the original games.

    Dragon Ninja puts the original game's priority register in Y bits 10-11 of its back board: 2 (bit 10)
    on stages 1, 2, 5 and 7, 5 (both bits) on stages 3, 4 and 6 and 4 (bit 11) on the title screen. As in
    the original game, 5 puts the back board in front, 2 draws pens 8-15 of colours 8-15 of the back
    board over the front one, and 4 and 5 draw those pens of the front layer over the sprites. Fighting
    Fantasy and Secret Agent keep the priority value in RAM and don't use it.

    The Automat program is the one of the single board Automat bootleg in dec0.cpp (the second pair of
    program EPROMs is the same) with the hardware accesses moved. The MOD 7/8 board shows the three layers
    of Robocop from its RAM, at the addresses of the original BAC06 chips, with the graphics of the MOD 8
    board: EPROMs 301-304 hold the characters (A15 low) and playfield 2 (A15 high) and 305-308 playfield
    1, one EPROM for each bitplane, the original graphics inverted. The program writes the playfield
    scroll values inverted to 0x400000-0x400007, and always 0x8001 (0x8000 and the original game's
    priority value 0, bit 0 inverted), 0, 0x8031 and 0x3ff to 0x400008-0x40000f.

    The YM2203s at 1.5 MHz give the pitch of the original Fighting Fantasy and Secret Agent music; Dragon
    Ninja and Automat play their own music, which is in tune at that clock. With the Z80 at 4 MHz the
    music plays 8 % faster than the original game on Fighting Fantasy and 1 % on Secret Agent. Fighting
    Fantasy drops some tunes that come right after command 01, which its sound program plays with a higher
    priority; how often depends on the clocks. The M5205 samples last as long as the original games' ones
    at 3.4 to 4.6 kHz.

    TODO:
    - where the MOD 4 boards take the scroll values from: the programs write them at different places in
      each board's RAM area; whether the bands of the X values follow the screen lines or the tile map
    - which board applies Dragon Ninja's priority values: its back board has a GAL16V8 (403) instead of
      the P0403 PAL16R8 of the other boards, with four more outputs
    - Automat: how the MOD 7/8 board counts the scroll values (the 48 and 50 pixels are fitted to the
      original game), and the meaning of 0x400008-0x40000f
    - clocks: the MOD 1 boards have no crystal
*/


#include "emu.h"

#include "../gaelco/gaelco_ms_mod1.h"
#include "../gaelco/gaelco_ms_mod21.h"
#include "../gaelco/gaelco_ms_mod4.h"
#include "../gaelco/gaelco_ms_mod51.h"

#include "cpu/m68000/m68000.h"

#include "screen.h"
#include "speaker.h"
#include "tilemap.h"


namespace {

class dec0_ms_state : public driver_device
{
protected:
	dec0_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mod1(*this, "mod1"),
		m_mod21(*this, "mod21"),
		m_mod51(*this, "mod51"),
		m_screen(*this, "screen"),
		m_gfxdecode(*this, "gfxdecode"),
		m_spritelist(*this, "spritelist")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<gaelco_ms_mod1_adpcm_device> m_mod1;
	required_device<gaelco_ms_mod21_device> m_mod21;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<screen_device> m_screen;
	required_device<gfxdecode_device> m_gfxdecode;

	required_shared_ptr<u16> m_spritelist;

	void base(machine_config &config) ATTR_COLD;

	TILEMAP_MAPPER_MEMBER(pages_2x2_scan);
	u8 sprite_byte(offs_t offset) const { return m_spritelist[offset >> 1] >> (BIT(offset, 0) ? 0 : 8); }
	void sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8);
	void vblank_irq(int state);
	void irq_ack_w(u16 data);
	u16 irq_ack_r();
};


class automat_ms_state : public dec0_ms_state
{
public:
	automat_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		dec0_ms_state(mconfig, type, tag),
		m_vram(*this, "vram%u", 0U),
		m_regs(*this, "regs")
	{ }

	void automatm(machine_config &config) ATTR_COLD;

protected:
	virtual void video_start() override ATTR_COLD;

private:
	enum : u8 { TX, PF1, PF2 };

	required_shared_ptr_array<u16, 3> m_vram;
	required_shared_ptr<u16> m_regs;

	tilemap_t *m_tilemap[3]{};

	void automatm_map(address_map &map) ATTR_COLD;

	TILE_GET_INFO_MEMBER(get_tx_tile_info);
	TILE_GET_INFO_MEMBER(get_pf1_tile_info);
	TILE_GET_INFO_MEMBER(get_pf2_tile_info);
	template <u32 Layer> void vram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
};


class ffantasy_ms_state : public dec0_ms_state
{
public:
	ffantasy_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		dec0_ms_state(mconfig, type, tag),
		m_mod4_tx(*this, "mod4_tx"),
		m_mod4_a(*this, "mod4_a"),
		m_mod4_b(*this, "mod4_b")
	{ }

	void drgninjam(machine_config &config) ATTR_COLD;
	void ffantasym(machine_config &config) ATTR_COLD;
	void secretagm(machine_config &config) ATTR_COLD;

private:
	required_device<gaelco_ms_mod4_device> m_mod4_tx;
	required_device<gaelco_ms_mod4_device> m_mod4_a; // front 16x16 board
	required_device<gaelco_ms_mod4_device> m_mod4_b; // back 16x16 board

	void video(machine_config &config) ATTR_COLD;

	void drgninjam_map(address_map &map) ATTR_COLD;
	void ffantasym_map(address_map &map) ATTR_COLD;
	void secretagm_map(address_map &map) ATTR_COLD;

	TILEMAP_MAPPER_MEMBER(pages_4x1_scan);
	void secretagm_tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void mod5_1_sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8);
	static u16 scroll_value(gaelco_ms_mod4_device &board, offs_t offset);
	static void set_scroll(gaelco_ms_mod4_device &board, offs_t x, offs_t y);
	static void set_band_scroll(gaelco_ms_mod4_device &board, offs_t x, offs_t y, u32 rows);
	void draw(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	uint32_t screen_update_drgninjam(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	uint32_t screen_update_ffantasym(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	uint32_t screen_update_secretagm(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
};


TILEMAP_MAPPER_MEMBER(dec0_ms_state::pages_2x2_scan)
{
	// four pages of 16 x 16 tiles, two by two, the left ones first
	return (col & 0x0f) | ((row & 0x1f) << 4) | ((col & 0x10) << 5);
}

void dec0_ms_state::sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8)
{
	// code bits 8-11 in attribute bits 0-3, flip X in bit 5 and flip Y in bit 6, palette in colour bits 4-7
	// and X bit 8 in colour bit 0
	code |= (attr & 0x0f) << 8;
	palette = color >> 4;
	flipx = BIT(attr, 5);
	flipy = BIT(attr, 6);
	x8 = BIT(color, 0);
}

void dec0_ms_state::vblank_irq(int state)
{
	if (state)
		m_maincpu->set_input_line(4, ASSERT_LINE);
}

void dec0_ms_state::irq_ack_w(u16 data)
{
	m_maincpu->set_input_line(4, CLEAR_LINE);
}

u16 dec0_ms_state::irq_ack_r()
{
	if (!machine().side_effects_disabled())
		m_maincpu->set_input_line(4, CLEAR_LINE);
	return 0;
}


TILE_GET_INFO_MEMBER(automat_ms_state::get_tx_tile_info)
{
	// MOD 8 EPROM lines A11-A14 take character bits 10, 11, 8 and 9
	u16 const data = m_vram[TX][tile_index];
	tileinfo.set(TX, bitswap<12>(data, 9, 8, 11, 10, 7, 6, 5, 4, 3, 2, 1, 0), data >> 12, 0);
}

TILE_GET_INFO_MEMBER(automat_ms_state::get_pf1_tile_info)
{
	// tile bits 8-10 on MOD 8 EPROM lines A13-A15, above the lines that pick the quarter
	u16 const data = m_vram[PF1][tile_index];
	tileinfo.set(PF1, (data & 0x0ff) | ((data & 0x700) << 2), data >> 12, 0);
}

TILE_GET_INFO_MEMBER(automat_ms_state::get_pf2_tile_info)
{
	u16 const data = m_vram[PF2][tile_index];
	tileinfo.set(PF2, (data & 0x0ff) | ((data & 0x300) << 2), data >> 12, 0);
}

template <u32 Layer>
void automat_ms_state::vram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_vram[Layer][offset]);
	m_tilemap[Layer]->mark_tile_dirty(offset);
}

void automat_ms_state::video_start()
{
	dec0_ms_state::video_start();

	m_tilemap[TX] = &machine().tilemap().create(
			*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(automat_ms_state::get_tx_tile_info)),
			TILEMAP_SCAN_ROWS, 8, 8, 32, 32);
	m_tilemap[PF1] = &machine().tilemap().create(
			*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(automat_ms_state::get_pf1_tile_info)),
			tilemap_mapper_delegate(*this, FUNC(automat_ms_state::pages_2x2_scan)), 16, 16, 32, 32);
	m_tilemap[PF2] = &machine().tilemap().create(
			*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(automat_ms_state::get_pf2_tile_info)),
			tilemap_mapper_delegate(*this, FUNC(automat_ms_state::pages_2x2_scan)), 16, 16, 32, 32);
	for (tilemap_t *tilemap : m_tilemap)
		tilemap->set_transparent_pen(15);
}

uint32_t automat_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the program writes the scroll values inverted, the X values minus 48 and 50
	m_tilemap[PF2]->set_scrollx((~m_regs[0] + 48) & 0x3ff);
	m_tilemap[PF2]->set_scrolly(~m_regs[1] & 0x3ff);
	m_tilemap[PF1]->set_scrollx((~m_regs[2] + 50) & 0x3ff);
	m_tilemap[PF1]->set_scrolly(~m_regs[3] & 0x3ff);

	m_tilemap[PF2]->draw(screen, bitmap, cliprect, TILEMAP_DRAW_OPAQUE);
	m_tilemap[PF1]->draw(screen, bitmap, cliprect, 0);
	m_mod51->draw(bitmap, cliprect, *m_gfxdecode->gfx(3));
	m_tilemap[TX]->draw(screen, bitmap, cliprect, 0);
	return 0;
}


void automat_ms_state::automatm_map(address_map &map)
{
	map(0x000000, 0x03ffff).rom();
	map(0x240000, 0x243fff).ram();
	map(0x244000, 0x2447ff).ram().w(FUNC(automat_ms_state::vram_w<TX>)).share(m_vram[TX]);
	map(0x24a000, 0x24a7ff).ram().w(FUNC(automat_ms_state::vram_w<PF1>)).share(m_vram[PF1]);
	map(0x24d000, 0x24d7ff).ram().w(FUNC(automat_ms_state::vram_w<PF2>)).share(m_vram[PF2]);
	map(0x30c000, 0x30c00f).rw(m_mod1, FUNC(gaelco_ms_mod1_device_base::host16_r), FUNC(gaelco_ms_mod1_device_base::host16_w));
	map(0x30c010, 0x30c011).noprw(); // the program still clears the original game's priority register
	map(0x310000, 0x3103ff).rw(m_mod21, FUNC(gaelco_ms_mod21_device::green_red_r), FUNC(gaelco_ms_mod21_device::green_red_w));
	map(0x310400, 0x3107ff).rw(m_mod21, FUNC(gaelco_ms_mod21_device::blue_r), FUNC(gaelco_ms_mod21_device::blue_w));
	map(0x400000, 0x40000f).ram().share(m_regs);
	// interrupt acknowledge: any access to the area the CPU board's decoder (IC7) selects for it
	map(0x500000, 0x500fff).rw(FUNC(automat_ms_state::irq_ack_r), FUNC(automat_ms_state::irq_ack_w));
	map(0xff8000, 0xffbfff).ram();
	map(0xffc000, 0xffc3ff).ram().share(m_spritelist);
}


TILEMAP_MAPPER_MEMBER(ffantasy_ms_state::pages_4x1_scan)
{
	// four pages of 16 x 16 tiles side by side
	return (col & 0x0f) | ((row & 0x0f) << 4) | ((col & 0x30) << 4);
}

void ffantasy_ms_state::secretagm_tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// EPROM A14 is held high, the characters are in the upper halves
	code |= 0x800;
}

void ffantasy_ms_state::mod5_1_sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8)
{
	// MOD 5/1 boards: palette in colour bits 0-3, the programs copy X bit 8 to colour bits 4-7
	code |= (attr & 0x0f) << 8;
	flipx = BIT(attr, 5);
	flipy = BIT(attr, 6);
	x8 = BIT(color, 4);
}

u16 ffantasy_ms_state::scroll_value(gaelco_ms_mod4_device &board, offs_t offset)
{
	return swapendian_int16(board.vram_r(offset));
}

void ffantasy_ms_state::set_scroll(gaelco_ms_mod4_device &board, offs_t x, offs_t y)
{
	board.set_scrollx(scroll_value(board, x));
	board.set_scrolly(scroll_value(board, y));
}

void ffantasy_ms_state::set_band_scroll(gaelco_ms_mod4_device &board, offs_t x, offs_t y, u32 rows)
{
	// one X value for each band of 16 lines, taken as tile map lines like the row scroll of the BAC06
	board.set_scroll_rows(rows);
	for (u32 row = 0; row < rows; row++)
		board.set_scrollx(row, scroll_value(board, x + (row & 0x0f)));
	board.set_scrolly(scroll_value(board, y));
}

void ffantasy_ms_state::draw(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_mod4_b->draw(screen, bitmap, cliprect);
	m_mod4_a->draw(screen, bitmap, cliprect);
	m_mod51->draw(bitmap, cliprect, *m_gfxdecode->gfx(0));
	m_mod4_tx->draw(screen, bitmap, cliprect);
}

uint32_t ffantasy_ms_state::screen_update_drgninjam(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	set_band_scroll(*m_mod4_tx, 0x000, 0x010, 16);
	set_band_scroll(*m_mod4_a, 0x400, 0x410, 32);
	set_band_scroll(*m_mod4_b, 0x400, 0x410, 32);

	// Y bits 10-11 of the back board: the original game's priority register 2, 4 or 5, or 0 for any other value
	u8 const mode = BIT(scroll_value(*m_mod4_b, 0x410), 10, 2);
	gaelco_ms_mod4_device &back = (mode == 3) ? *m_mod4_a : *m_mod4_b;
	gaelco_ms_mod4_device &front = (mode == 3) ? *m_mod4_b : *m_mod4_a;

	back.draw(screen, bitmap, cliprect, TILEMAP_DRAW_OPAQUE);
	front.draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER1);
	if (mode == 1)
		back.draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER0);
	m_mod51->draw(bitmap, cliprect, *m_gfxdecode->gfx(0));
	if (mode >= 2)
		front.draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER0);
	m_mod4_tx->draw(screen, bitmap, cliprect);
	return 0;
}

uint32_t ffantasy_ms_state::screen_update_ffantasym(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	set_scroll(*m_mod4_tx, 0x600, 0x601);
	set_scroll(*m_mod4_a, 0x600, 0x601);
	set_scroll(*m_mod4_b, 0x600, 0x601);
	draw(screen, bitmap, cliprect);
	return 0;
}

uint32_t ffantasy_ms_state::screen_update_secretagm(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	set_scroll(*m_mod4_tx, 0x600, 0x601);
	set_band_scroll(*m_mod4_a, 0x610, 0x601, 32);
	set_scroll(*m_mod4_b, 0x600, 0x601);
	draw(screen, bitmap, cliprect);
	return 0;
}


void ffantasy_ms_state::drgninjam_map(address_map &map)
{
	// each EPROM half holds one of the two program areas of the original game
	map(0x000000, 0x01ffff).rom();
	map(0x040000, 0x05ffff).rom().region("maincpu", 0x20000);
	map(0x244000, 0x245fff).rw(m_mod4_tx, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x24a000, 0x24afff).rw(m_mod4_b, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x24d000, 0x24dfff).rw(m_mod4_a, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x280000, 0x2803ff).ram().share(m_spritelist);
	// interrupt acknowledge: any access to the area the CPU board's decoder (IC7) selects for it
	map(0x290000, 0x290fff).rw(FUNC(ffantasy_ms_state::irq_ack_r), FUNC(ffantasy_ms_state::irq_ack_w));
	map(0x30c000, 0x30c00f).rw(m_mod1, FUNC(gaelco_ms_mod1_device_base::host16_r), FUNC(gaelco_ms_mod1_device_base::host16_w));
	map(0x310000, 0x3103ff).rw(m_mod21, FUNC(gaelco_ms_mod21_device::green_red_r), FUNC(gaelco_ms_mod21_device::green_red_w));
	map(0x310400, 0x3107ff).rw(m_mod21, FUNC(gaelco_ms_mod21_device::blue_r), FUNC(gaelco_ms_mod21_device::blue_w));
	map(0xff8000, 0xffffff).ram();
}

void ffantasy_ms_state::ffantasym_map(address_map &map)
{
	map(0x000000, 0x03ffff).rom();
	map(0x242000, 0x243fff).ram();
	map(0x244000, 0x244fff).rw(m_mod4_tx, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x24a000, 0x24afff).rw(m_mod4_a, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x24d000, 0x24dfff).rw(m_mod4_b, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	// interrupt acknowledge: any access to the area the CPU board's decoder (IC7) selects for it
	map(0x290000, 0x290fff).rw(FUNC(ffantasy_ms_state::irq_ack_r), FUNC(ffantasy_ms_state::irq_ack_w));
	map(0x30c000, 0x30c00f).rw(m_mod1, FUNC(gaelco_ms_mod1_device_base::host16_r), FUNC(gaelco_ms_mod1_device_base::host16_w));
	map(0x310000, 0x3103ff).rw(m_mod21, FUNC(gaelco_ms_mod21_device::green_red_r), FUNC(gaelco_ms_mod21_device::green_red_w));
	map(0x310400, 0x3107ff).rw(m_mod21, FUNC(gaelco_ms_mod21_device::blue_r), FUNC(gaelco_ms_mod21_device::blue_w));
	map(0xfc0000, 0xfc03ff).ram().share(m_spritelist);
	map(0xff8000, 0xffffff).ram();
}

void ffantasy_ms_state::secretagm_map(address_map &map)
{
	map(0x000000, 0x03ffff).rom();
	// the code of the original game still writes the registers and scroll RAM of its BAC06 chips
	map(0x240000, 0x24001f).nopw();
	map(0x242000, 0x2427ff).nopw();
	map(0x248000, 0x24801f).nopw();
	map(0x24c000, 0x24c7ff).nopw();
	map(0x300000, 0x30001f).nopw();
	map(0x300800, 0x30087f).nopw();
	map(0x246000, 0x246fff).rw(m_mod4_a, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x24e000, 0x24efff).rw(m_mod4_tx, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x301000, 0x301fff).rw(m_mod4_b, FUNC(gaelco_ms_mod4_device::vram_r), FUNC(gaelco_ms_mod4_device::vram_w));
	map(0x304000, 0x307fff).ram();
	map(0x310000, 0x3103ff).rw(m_mod21, FUNC(gaelco_ms_mod21_device::green_red_r), FUNC(gaelco_ms_mod21_device::green_red_w));
	map(0x310400, 0x3107ff).rw(m_mod21, FUNC(gaelco_ms_mod21_device::blue_r), FUNC(gaelco_ms_mod21_device::blue_w));
	map(0x314000, 0x31400f).rw(m_mod1, FUNC(gaelco_ms_mod1_device_base::host16_r), FUNC(gaelco_ms_mod1_device_base::host16_w));
	map(0x380000, 0x3803ff).ram().share(m_spritelist);
	// interrupt acknowledge: any access to the area the CPU board's decoder (IC7) selects for it
	map(0x500000, 0x500fff).rw(FUNC(ffantasy_ms_state::irq_ack_r), FUNC(ffantasy_ms_state::irq_ack_w));
}


// The switches are wired to the YM2203 ports the other way round: switch 1 is bit 7 and switch 8 is bit 0
// (DIP switch sheets of Dragon Ninja and Fighting Fantasy)

static INPUT_PORTS_START( automatm )
	PORT_INCLUDE( gaelco_ms_mod1 )

	PORT_START("SW1") // MOD 1 YM2203 #2 port B
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x10, "SW1:4" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	// flip screen and cabinet in the original game, the program only writes them to the registers of the original video chips
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW1:2" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:1" )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A
	PORT_DIPNAME( 0x03, 0x03, "Player Energy" ) PORT_DIPLOCATION("SW2:8,7")
	PORT_DIPSETTING(    0x01, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x02, DEF_STR( High ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_High ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Allow_Continue ) ) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Bonus Stage Energy" ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x20, DEF_STR( High ) )
	PORT_DIPNAME( 0x40, 0x40, "Brink Time" ) PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Less" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW2:1" )
INPUT_PORTS_END

static INPUT_PORTS_START( drgninjam )
	PORT_INCLUDE( gaelco_ms_mod1 )

	PORT_START("SW1") // MOD 1 YM2203 #2 port B, the sound program sets bits 4 and 6 and clears bit 7
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x10, "SW1:4" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW1:2" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:1" )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A, the sound program sets bits 5-7
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW2:8,7")
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "Infinite (Cheat)" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Allow_Continue ) ) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x20, "SW2:3" )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW2:2" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW2:1" )
INPUT_PORTS_END

static INPUT_PORTS_START( ffantasym )
	PORT_INCLUDE( gaelco_ms_mod1 )

	PORT_START("SW1") // MOD 1 YM2203 #2 port B
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x10, "SW1:4" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW1:2" ) // flip screen in the original game, skipped by the program
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:1" )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW2:8,7")
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x30, 0x30, "Player & Enemy Energy" ) PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(    0x10, DEF_STR( Very_Low ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x00, DEF_STR( High ) )
	PORT_DIPNAME( 0x40, 0x40, "Enemy Power Decrease on Continue" ) PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x40, "2 Dots" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW2:1" )
INPUT_PORTS_END

static INPUT_PORTS_START( secretagm )
	PORT_INCLUDE( gaelco_ms_mod1 )

	PORT_START("SW1") // MOD 1 YM2203 #2 port B
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_SERVICE_DIPLOC( 0x10, IP_ACTIVE_LOW, "SW1:4" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	// flip screen and cabinet in the original game, the program only writes them to the registers of the original video chips
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW1:2" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:1" )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A
	PORT_DIPNAME( 0x03, 0x03, "Energy" ) PORT_DIPLOCATION("SW2:8,7")
	PORT_DIPSETTING(    0x02, "Low - 8 bars" )
	PORT_DIPSETTING(    0x03, "Medium - 10 bars" )
	PORT_DIPSETTING(    0x01, "High - 12 bars" )
	PORT_DIPSETTING(    0x00, "Very High - 14 bars" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:6,5")
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Allow_Continue ) ) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x20, "SW2:3" )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW2:2" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW2:1" )
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

static GFXDECODE_START( gfx_drgninjam )
	GFXDECODE_ENTRY( "sprites", 0, sprite_layout, 0x300, 16 )
GFXDECODE_END

static GFXDECODE_START( gfx_ffantasy_ms )
	GFXDECODE_ENTRY( "sprites", 0, sprite_layout, 0x100, 16 )
GFXDECODE_END

// MOD 8: one EPROM for each bitplane; A0-A2 row, A3-A10 tile bits 0-7, A11 lower half and A12 left half of 16x16 tiles
static const gfx_layout automatm_char_layout =
{
	8, 8,
	0x1000,
	4,
	{ 0x30000 * 8, 0x20000 * 8, 0x10000 * 8, 0 },
	{ STEP8(0, 1) },
	{ STEP8(0, 8) },
	8 * 8
};

static const gfx_layout automatm_pf1_layout =
{
	16, 16,
	0x1d00,
	4,
	{ 0x30000 * 8, 0x20000 * 8, 0x10000 * 8, 0 },
	{ STEP8(0x1000 * 8, 1), STEP8(0, 1) },
	{ STEP8(0, 8), STEP8(0x800 * 8, 8) },
	8 * 8
};

static const gfx_layout automatm_pf2_layout =
{
	16, 16,
	0x0d00,
	4,
	{ 0x30000 * 8, 0x20000 * 8, 0x10000 * 8, 0 },
	{ STEP8(0x1000 * 8, 1), STEP8(0, 1) },
	{ STEP8(0, 8), STEP8(0x800 * 8, 8) },
	8 * 8
};

static GFXDECODE_START( gfx_automatm )
	GFXDECODE_ENTRY( "mod8",    0x00000, automatm_char_layout, 0x000, 16 ) // EPROMs 301-304, A15 low
	GFXDECODE_ENTRY( "mod8",    0x40000, automatm_pf1_layout,  0x100, 16 ) // EPROMs 305-308
	GFXDECODE_ENTRY( "mod8",    0x08000, automatm_pf2_layout,  0x300, 16 ) // EPROMs 301-304, A15 high
	GFXDECODE_ENTRY( "sprites", 0,       sprite_layout,        0x200, 16 )
GFXDECODE_END


void dec0_ms_state::base(machine_config &config)
{
	// basic machine hardware
	M68000(config, m_maincpu, 20_MHz_XTAL / 2); // divisor unknown

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_raw(20_MHz_XTAL / 4, 320, 0, 256, 266, 8, 248); // MOD 21/1 PROMs 201 and 205
	m_screen->set_palette(m_mod21);

	GAELCO_MS_MOD21(config, m_mod21, 20_MHz_XTAL);
	m_mod21->set_screen(m_screen);

	GAELCO_MS_MOD51(config, m_mod51);
	m_mod51->set_timing(m_mod21);

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1_ADPCM(config, m_mod1, 4'000'000); // unknown, no crystal on the board
	m_mod1->set_ym_clock(1'500'000); // unknown, gives the pitch of the original Fighting Fantasy and Secret Agent music
	m_mod1->set_adpcm_prescaler(msm5205_device::S96_4B); // unverified, the samples last as long as the original games' ones at about 4 kHz
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<2>().set_ioport("SW2");
	m_mod1->in_callback<3>().set_ioport("SW1");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 0.30);
}

void automat_ms_state::automatm(machine_config &config)
{
	base(config);

	m_maincpu->set_addrmap(AS_PROGRAM, &automat_ms_state::automatm_map);

	m_screen->set_screen_update(FUNC(automat_ms_state::screen_update));
	m_screen->screen_vblank().set(FUNC(automat_ms_state::vblank_irq));

	GFXDECODE(config, m_gfxdecode, m_mod21, gfx_automatm);

	m_mod51->list_r().set([this] (offs_t offset) { return sprite_byte(offset); });
	m_mod51->set_sprite_callback(FUNC(automat_ms_state::sprite_cb));
}

void ffantasy_ms_state::video(machine_config &config)
{
	m_screen->screen_vblank().set(FUNC(ffantasy_ms_state::vblank_irq));

	m_mod51->list_r().set([this] (offs_t offset) { return sprite_byte(offset); });

	GAELCO_MS_MOD4(config, m_mod4_tx);
	m_mod4_tx->set_timing(m_mod21);
	m_mod4_tx->set_palette(m_mod21);
	m_mod4_tx->set_tile_size(8);
	m_mod4_tx->set_layout(TILEMAP_SCAN_ROWS, 32, 32);
	m_mod4_tx->set_transparent_pen(15);

	// the 16x16 boards show the tile maps in the shape the original game sets in its BAC06 chips
	GAELCO_MS_MOD4(config, m_mod4_a);
	m_mod4_a->set_timing(m_mod21);
	m_mod4_a->set_palette(m_mod21);
	m_mod4_a->set_layout(FUNC(ffantasy_ms_state::pages_2x2_scan), 32, 32);
	m_mod4_a->set_transparent_pen(15);

	GAELCO_MS_MOD4(config, m_mod4_b);
	m_mod4_b->set_timing(m_mod21);
	m_mod4_b->set_palette(m_mod21);
	m_mod4_b->set_layout(FUNC(ffantasy_ms_state::pages_2x2_scan), 32, 32);
}

void ffantasy_ms_state::drgninjam(machine_config &config)
{
	base(config);
	video(config);

	m_maincpu->set_addrmap(AS_PROGRAM, &ffantasy_ms_state::drgninjam_map);

	m_screen->set_screen_update(FUNC(ffantasy_ms_state::screen_update_drgninjam));

	GFXDECODE(config, m_gfxdecode, m_mod21, gfx_drgninjam);

	m_mod51->set_sprite_callback(FUNC(ffantasy_ms_state::sprite_cb));

	// pens 8-15 of the original game in colours 8-15 can go in front of the other layer or the sprites
	m_mod4_a->set_color_base(0x100);
	m_mod4_a->set_half_lines(9);
	m_mod4_a->set_front_pens(0xff00, 0x00ff);
	m_mod4_b->set_color_base(0x200);
	m_mod4_b->set_half_lines(9);
	m_mod4_b->set_transparent_pen(15);
	m_mod4_b->set_front_pens(0xff00, 0x00ff);
}

void ffantasy_ms_state::ffantasym(machine_config &config)
{
	base(config);
	video(config);

	m_maincpu->set_addrmap(AS_PROGRAM, &ffantasy_ms_state::ffantasym_map);

	m_screen->set_screen_update(FUNC(ffantasy_ms_state::screen_update_ffantasym));

	GFXDECODE(config, m_gfxdecode, m_mod21, gfx_ffantasy_ms);

	m_mod51->set_sprite_callback(FUNC(ffantasy_ms_state::mod5_1_sprite_cb));

	m_mod4_a->set_color_base(0x200);
	m_mod4_a->set_layout(FUNC(ffantasy_ms_state::pages_4x1_scan), 64, 16);
	m_mod4_b->set_color_base(0x300);
	m_mod4_b->set_layout(FUNC(ffantasy_ms_state::pages_4x1_scan), 64, 16);
}

void ffantasy_ms_state::secretagm(machine_config &config)
{
	ffantasym(config);

	// Exactly the same hardware configuration as 'ffantasym', but with a 22.1184 MHz xtal on the CPU PCB.
	m_maincpu->set_clock(22.1184_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &ffantasy_ms_state::secretagm_map);

	m_screen->set_screen_update(FUNC(ffantasy_ms_state::screen_update_secretagm));

	m_mod4_tx->set_tile_callback(FUNC(ffantasy_ms_state::secretagm_tx_tile_cb));
	m_mod4_a->set_layout(FUNC(ffantasy_ms_state::pages_2x2_scan), 32, 32);
	m_mod4_b->set_layout(FUNC(ffantasy_ms_state::pages_2x2_scan), 32, 32);
}

ROM_START( automatm )
	ROM_REGION( 0x40000, "maincpu", 0 ) // on MOD 6/1 board
	ROM_LOAD16_BYTE( "mod_6-1_automat_2au_603.ic17", 0x00000, 0x10000, CRC(efce0f8e) SHA1(c27a2db6c5cf92b9682be12d00d4c8b2998a88ee) )
	ROM_LOAD16_BYTE( "mod_6-1_automat_2au_606.ic26", 0x00001, 0x10000, CRC(f19fc5b6) SHA1(3d27ed69a4ab4d799c4afa779ac5d5083cd73cc2) )
	ROM_LOAD16_BYTE( "mod_6-1_automat_2au_602.ic20", 0x20000, 0x10000, CRC(9d7b79e0) SHA1(e0d901b9b3cd62f7c947da04f7447ebfa88bf44a) )
	ROM_LOAD16_BYTE( "mod_6-1_automat_2au_605.ic11", 0x20001, 0x10000, CRC(e655f9c3) SHA1(d5e99d542303d009277ccfc245f877e4e28603c9) )

	ROM_REGION( 0x10000, "mod1", 0 ) // on MOD 1/5 board
	ROM_LOAD( "mod_1-5_automat_2au_101.ic12", 0x00000, 0x10000, CRC(fc80cac1) SHA1(0f31d6c3f2ca1aa1c6d9ac5da28b75a6fbf8444b) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "mod_1-5_automat_110_82s123.ic20", 0x00, 0x20, CRC(e26e680a) SHA1(9bbe30e98e952a6113c64e1171330153ddf22ce7) )

	ROM_REGION( 0x80000, "mod8", 0 ) // on MOD 8 board
	ROM_LOAD( "mod_8_automat_2au_301.ic15", 0x00000, 0x10000, CRC(fd3b363b) SHA1(af63d57e362f159c5b54a764e2069ca24849164b) )
	ROM_LOAD( "mod_8_automat_2au_302.ic22", 0x10000, 0x10000, CRC(05755c02) SHA1(e1fa7d464eb7bbf2f22ca132e7df8e92911a0685) )
	ROM_LOAD( "mod_8_automat_2au_303.ic30", 0x20000, 0x10000, CRC(9440bb38) SHA1(aee9372bfc3b2b036a208d91a706312b77042001) )
	ROM_LOAD( "mod_8_automat_2au_304.ic37", 0x30000, 0x10000, CRC(a1c9d781) SHA1(8e062af8a9c2eef74c8e578624f3c961750ac260) )
	ROM_LOAD( "mod_8_automat_2au_305.ic14", 0x40000, 0x10000, CRC(53cb118e) SHA1(cf2329cb8f41c6ddc81dea8fa8def0e47e25b723) )
	ROM_LOAD( "mod_8_automat_2au_306.ic21", 0x50000, 0x10000, CRC(21b682f7) SHA1(1e9bdae1317f215ffdffdd2c1a3c0e08c14ab00c) )
	ROM_LOAD( "mod_8_automat_2au_307.ic29", 0x60000, 0x10000, CRC(be822d87) SHA1(d5f9bb55a16f0ab2d6d11fea5d22b7ced59f9128) )
	ROM_LOAD( "mod_8_automat_2au_308.ic36", 0x70000, 0x10000, CRC(9459159a) SHA1(20fc3f33f2e23d88b2bd21f248ed7814f03b387f) )

	ROM_REGION( 0x80000, "sprites", ROMREGION_INVERT ) // on MOD 51/3 board
	ROM_LOAD32_BYTE( "mod_51-3_automat_2au_501.ic43", 0x00003, 0x20000, CRC(8e7ec3fc) SHA1(0c40cf90bb105dca2629a61ff13227b82d3b10de) )
	ROM_LOAD32_BYTE( "mod_51-3_automat_2au_502.ic42", 0x00002, 0x20000, CRC(d0a46eae) SHA1(3efe3b1462b442b4a16704d15e201a4d7bd402c8) )
	ROM_LOAD32_BYTE( "mod_51-3_automat_2au_503.ic41", 0x00001, 0x20000, CRC(e174cdaf) SHA1(e66c8adc6df08e30365bd0e360e78397b495582c) )
	ROM_LOAD32_BYTE( "mod_51-3_automat_2au_504.ic40", 0x00000, 0x20000, CRC(e8662060) SHA1(f59f55fe5bf79be0c66f4984a0ecab17504438c7) )

	ROM_REGION( 0x200, "mod21", ROMREGION_ERASE00 ) // on MOD 2 board
	ROM_LOAD( "82s129.ic4",  0x000, 0x100, NO_DUMP )
	ROM_LOAD( "82s129.ic12", 0x100, 0x100, NO_DUMP )
	// not dumped, the 201 and 205 contents of the other Data East games
	ROM_FILL( 0x000, 0x01, 0x04 )
	ROM_FILL( 0x001, 0x02, 0x05 )
	ROM_FILL( 0x003, 0x0c, 0x01 )
	ROM_FILL( 0x00f, 0x10, 0x05 )
	ROM_FILL( 0x01f, 0x02, 0x07 )
	ROM_FILL( 0x021, 0x7e, 0x06 )
	ROM_FILL( 0x09f, 0x01, 0x0c )
	ROM_FILL( 0x100, 0x04, 0x03 )
	ROM_FILL( 0x104, 0x04, 0x07 )
	ROM_FILL( 0x108, 0x78, 0x05 )
	ROM_FILL( 0x180, 0x04, 0x07 )
	ROM_FILL( 0x184, 0x01, 0x0b )
	// bit 0 pulses
	ROM_FILL( 0x107, 0x01, 0x06 )
	ROM_FILL( 0x10f, 0x01, 0x04 )
	ROM_FILL( 0x117, 0x01, 0x04 )
	ROM_FILL( 0x11f, 0x01, 0x04 )
	ROM_FILL( 0x127, 0x01, 0x04 )
	ROM_FILL( 0x12f, 0x01, 0x04 )
	ROM_FILL( 0x137, 0x01, 0x04 )
	ROM_FILL( 0x13f, 0x01, 0x04 )
	ROM_FILL( 0x147, 0x01, 0x04 )
	ROM_FILL( 0x14f, 0x01, 0x04 )
	ROM_FILL( 0x157, 0x01, 0x04 )
	ROM_FILL( 0x15f, 0x01, 0x04 )
	ROM_FILL( 0x167, 0x01, 0x04 )
	ROM_FILL( 0x170, 0x01, 0x04 )
	ROM_FILL( 0x179, 0x01, 0x04 )
	ROM_FILL( 0x182, 0x01, 0x06 )

	ROM_REGION( 0x1000, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "mod_6-1_automat_604_gal16v8.ic13", 0x000, 0x117, NO_DUMP )
	ROM_LOAD( "mod_6-1_automat_633_gal16v8.ic7",  0x200, 0x117, CRC(b12c8fc7) SHA1(5bbae33f146fb9ae935e790afda17324dd7b0e55) )
	ROM_LOAD( "mod_7-8_7133_gal20v8.ic7",         0x400, 0x157, CRC(75c0363e) SHA1(0d3daf816a4fb549ef9e4a2d3cf2530acab3e6de) )
	ROM_LOAD( "mod_7-8_7233_gal20v8.ic54",        0x600, 0x157, CRC(ca393ed5) SHA1(8ba8b85b1bf415f2f7ad3c75de271c8013254c7f) )
	ROM_LOAD( "mod_7-8_7333_gal16v8.ic55",        0x800, 0x117, CRC(0c61be97) SHA1(8c6c5f29e8b91f90ff67caeed30e9e55860b335f) )
	ROM_LOAD( "mod_7-8_7433_gal16v8.ic9",         0xa00, 0x117, CRC(73e881cf) SHA1(43099330b37fac559f1984f11199d45f28aa4c01) )
	ROM_LOAD( "mod_7-8_7533_gal16v8.ic59",        0xc00, 0x117, CRC(f4cdd3f7) SHA1(9095f4618099a9d7fc65a181b569f5d25a342b3b) )
	ROM_LOAD( "mod_7-8_7633_gal20v9.ic44",        0xe00, 0x157, CRC(9ffe8cbe) SHA1(3a0ad6a9ea30bc534202b2289da164c970749b1f) )
ROM_END

ROM_START( drgninjam )
	ROM_REGION( 0x40000, "maincpu", 0 ) // on MOD 6/1 board, extremely similar to drgninjab in dec0.cpp
	ROM_LOAD16_BYTE( "6-1_cpu_dn_603.ic17", 0x00000, 0x20000, CRC(1c3670df) SHA1(0e287a0b4494e702080e80a911c4762bbc6a5815) )
	ROM_LOAD16_BYTE( "6-1_cpu_dn_606.ic8",  0x00001, 0x20000, CRC(07669458) SHA1(c5dcf72eba5e345228ee3f9e0228cba526e1156a) )

	ROM_REGION( 0x10000, "mod1", 0 ) // on MOD 1/2 board
	ROM_LOAD( "1-4_snd_dn_101.ic12", 0x00000, 0x10000, CRC(0b82c205) SHA1(0be243b19693c54914eccefbf6a8b8b513eec299) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "1-4_snd_110_82s123.ic20", 0x00, 0x20, CRC(e26e680a) SHA1(9bbe30e98e952a6113c64e1171330153ddf22ce7) )

	ROM_REGION( 0x10000, "mod4_tx", ROMREGION_INVERT ) // on MOD 4/3 board
	ROM_LOAD32_BYTE( "4-3_dn_401.ic17", 0x00003, 0x4000, CRC(45ab2822) SHA1(0cf0f74ad5325a66f74fc20cabd6a9d3d8c42f2a) )
	ROM_LOAD32_BYTE( "4-3_dn_402.ic16", 0x00002, 0x4000, CRC(26f0095b) SHA1(9e568a23df2b0ca05d8b87fea445f5f98e55fac5) )
	ROM_LOAD32_BYTE( "4-3_dn_403.ic15", 0x00001, 0x4000, CRC(c2061c37) SHA1(b06737ecece7da4785c2e98977f6dc35986a445e) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_IGNORE(                                  0x4000 )
	ROM_LOAD32_BYTE( "4-3_dn_404.ic14", 0x00000, 0x4000, CRC(1da44e82) SHA1(ccb9464b711025a5c2c18ecb765f7f0e15f7d4a5) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_IGNORE(                                  0x4000 )

	ROM_REGION( 0x20000, "mod4_a", ROMREGION_INVERT ) // on MOD 4/3-A board
	ROM_LOAD32_BYTE( "4-3-a_dn_4a01.ic17", 0x00003, 0x8000, CRC(c89982ac) SHA1(7509322bf4dcaceefdbaf6e9f4087a576133fe39) )
	ROM_LOAD32_BYTE( "4-3-a_dn_4a02.ic16", 0x00002, 0x8000, CRC(144b0359) SHA1(46565c5694f0cfcf06c1ee7c131ab3c88c804d35) )
	ROM_LOAD32_BYTE( "4-3-a_dn_4a03.ic15", 0x00001, 0x8000, CRC(ae98a684) SHA1(d6fe7237b82125a21b9804500ba2166906475a0e) )
	ROM_LOAD32_BYTE( "4-3-a_dn_4a04.ic14", 0x00000, 0x8000, CRC(8292c4ad) SHA1(f83acf64b101022927b678749d754374a20f5444) )

	ROM_REGION( 0x40000, "mod4_b", ROMREGION_INVERT ) // on MOD 4/2-B board
	ROM_LOAD32_BYTE( "4-2-b_dn_4b01.ic17", 0x00003, 0x10000, CRC(65002e5c) SHA1(e1fa0a0395d3fbcf31e15b63b7eaf478063aa971) )
	ROM_LOAD32_BYTE( "4-2-b_dn_4b02.ic16", 0x00002, 0x10000, CRC(dd6acd2d) SHA1(e2a0ccf49dce421b10ad0ed54d02ba1ca6525404) )
	ROM_LOAD32_BYTE( "4-2-b_dn_4b03.ic15", 0x00001, 0x10000, CRC(63e337c1) SHA1(ca347d6a46ee643edfdc8614760dc9b0994e1745) )
	ROM_LOAD32_BYTE( "4-2-b_dn_4b04.ic14", 0x00000, 0x10000, CRC(5cc7ec8c) SHA1(3f17e13af7152c51ce025ca06ece70b4a6f65f94) )

	ROM_REGION( 0x80000, "sprites", ROMREGION_INVERT ) // on MOD 51/3 board
	ROM_LOAD32_BYTE( "51-3_dn_501.ic43", 0x00003, 0x20000, CRC(0fccce1f) SHA1(e2e5625b62ddd73a4363596a4b1b2d72c2fe1c38) )
	ROM_LOAD32_BYTE( "51-3_dn_502.ic42", 0x00002, 0x20000, CRC(361f4616) SHA1(40279c76d027bcf698b8ff694afc2afb15279381) )
	ROM_LOAD32_BYTE( "51-3_dn_503.ic41", 0x00001, 0x20000, CRC(b3c97ad1) SHA1(f77426e1ee347b8cae9c5aef1e31058344c019c2) )
	ROM_LOAD32_BYTE( "51-3_dn_504.ic40", 0x00000, 0x20000, CRC(1445dccb) SHA1(fddbb9d136ea6dbb2704ac93e5536ed0e6bfb19c) )

	ROM_REGION( 0x200, "mod21", 0 ) // on MOD 21/1 board
	ROM_LOAD( "21-1_p0201_tbp24s10n.ic4", 0x000, 0x100, CRC(2697da58) SHA1(e62516b886ff6e204b718e5f0c6ce2712e4b7fc5) )
	ROM_LOAD( "21-1_p0205_82s129n.ic12",  0x100, 0x100, CRC(204a7aee) SHA1(322164134aa65c37a9389024f921364a81d13e88) )

	ROM_REGION( 0xc00, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "6-1_cpu_604_gal16v8.ic13", 0x000, 0x117, NO_DUMP )
	ROM_LOAD( "6-1_cpu_630_gal16v8.ic7",  0x200, 0x117, CRC(d4d95305) SHA1(c22d027f9b3720c1c91aa03d430ce75624c73012) )
	ROM_LOAD( "4-3_p0403_pal16r8a.ic29",  0x400, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "4-3-a_p0403.ic29",         0x600, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "4-2-b_403_gal16v8.ic29",   0x800, 0x117, CRC(c136de93) SHA1(116f6d3b456d20621ab07a005c1421f57569915c) )
	ROM_LOAD( "51-3_503_gal16v8.ic46",    0xa00, 0x117, CRC(11470ea1) SHA1(cfcafbcc7e55be717348f895df61e144fdd0cc9b) )
ROM_END

ROM_START( ffantasym )
	ROM_REGION( 0x40000, "maincpu", 0 ) // on MOD 6/1 board, extremely similar to ffantasybl in dec0.cpp
	ROM_LOAD16_BYTE( "ff_6-1_5fa_603.ic17", 0x00000, 0x10000, CRC(124ebff8) SHA1(20423990903dc3d682e2df6fb4dec0ae6b49036d) )
	ROM_LOAD16_BYTE( "ff_6-1_5fa_606.ic8",  0x00001, 0x10000, CRC(94fa4a64) SHA1(fa1405144fc525debd1b62f78b5434cf7670e1cb) )
	ROM_LOAD16_BYTE( "ff_6-1_5fa_602.ic20", 0x20000, 0x10000, CRC(68d50c5d) SHA1(d4a96c9ecb565c3c84ca76537492101264007cc1) )
	ROM_LOAD16_BYTE( "ff_6-1_5fa_605.ic11", 0x20001, 0x10000, CRC(dee5475e) SHA1(57a063834dc2fe97f4372eee06114f873a372786) )

	ROM_REGION( 0x10000, "mod1", 0 ) // on MOD 1/2 board
	ROM_LOAD( "ff_1-5_5fa_101.ic12", 0x00000, 0x10000, CRC(891bc223) SHA1(737248ea23c42076da9ce6c4abb9506f396892a0) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "ff_1-5_110_82s123.ic20", 0x00, 0x20, CRC(e26e680a) SHA1(9bbe30e98e952a6113c64e1171330153ddf22ce7) )

	ROM_REGION( 0x20000, "mod4_tx", ROMREGION_INVERT ) // on MOD 4/3 board
	ROM_LOAD32_BYTE( "ff_4-3_5fa_401.ic17", 0x00003, 0x8000, CRC(26f57324) SHA1(079745385f1240edf4ce712262352547b20282d2) )
	ROM_LOAD32_BYTE( "ff_4-3_5fa_402.ic16", 0x00002, 0x8000, CRC(fd8030d7) SHA1(84a9d775c0be1d068795ae8889cb5cec2842d622) )
	ROM_LOAD32_BYTE( "ff_4-3_5fa_403.ic15", 0x00001, 0x8000, CRC(0c572113) SHA1(3d035f1a985d1e04e64eb124c83c44e17bc07d53) )
	ROM_LOAD32_BYTE( "ff_4-3_5fa_404.ic14", 0x00000, 0x8000, CRC(819d8197) SHA1(1303d94f6b3048e1108d25662de971a77f6e3ae6) )

	ROM_REGION( 0x20000, "mod4_a", ROMREGION_INVERT ) // on MOD 4/3-A board
	ROM_LOAD32_BYTE( "ff_4-3-a_5fa_4a1.ic17", 0x00003, 0x8000, CRC(4be99b3a) SHA1(d4c576dda0bd855c8f361c96cd4615815c8c36c8) )
	ROM_LOAD32_BYTE( "ff_4-3-a_5fa_4a2.ic16", 0x00002, 0x8000, CRC(206239f1) SHA1(f5d2f7c888f01db57c9b78eff733b22301836025) )
	ROM_LOAD32_BYTE( "ff_4-3-a_5fa_4a3.ic15", 0x00001, 0x8000, CRC(c3eeb8a4) SHA1(c097034e2d3c3f55c935aff1d2c14a1dacc1f5da) )
	ROM_LOAD32_BYTE( "ff_4-3-a_5fa_4a4.ic14", 0x00000, 0x8000, CRC(a4a8ba47) SHA1(2bf3379a33850703db49cbcdd61d5e4777006726) )

	ROM_REGION( 0x20000, "mod4_b", ROMREGION_INVERT ) // on MOD 4/3-B board
	ROM_LOAD32_BYTE( "ff_4-3-b_4fa_4b1.ic17", 0x00003, 0x8000, CRC(51c274f0) SHA1(145a2da91db9d26a8ecaa6d4267c9aa0e4883fdc) )
	ROM_LOAD32_BYTE( "ff_4-3-b_4fa_4b2.ic16", 0x00002, 0x8000, CRC(53fade55) SHA1(d7ae60c908338f746fb585bf81c89036a6a9c8e0) )
	ROM_LOAD32_BYTE( "ff_4-3-b_4fa_4b3.ic15", 0x00001, 0x8000, CRC(49668a54) SHA1(f6129aa46f8e766a5012fe1b90c98a78581a773b) )
	ROM_LOAD32_BYTE( "ff_4-3-b_4fa_4b4.ic14", 0x00000, 0x8000, BAD_DUMP CRC(bd0d2e07) SHA1(3165fb23ee9cd5912fd17e557dd0d1513bd189ef) )

	ROM_REGION( 0x80000, "sprites", ROMREGION_INVERT ) // on MODULAR SYSTEM 2 MOD 5/1 board, two EPROMs for each bitplane
	ROM_LOAD32_BYTE( "ff_5-1_5fa_505.ic5",  0x00003, 0x10000, CRC(24503b71) SHA1(4b303bbfa7f7ede1679cdf660e5cf53806ca3531) )
	ROM_LOAD32_BYTE( "ff_5-1_5fa_506.ic6",  0x40003, 0x10000, CRC(b47d3b99) SHA1(ee6c79a93a0640692fcc49a0c247d15f35e3a7ca) )
	ROM_LOAD32_BYTE( "ff_5-1_5fa_511.ic14", 0x00002, 0x10000, CRC(01b8cc18) SHA1(434989b5cf00845788d4a855fc027684e8c79ace) )
	ROM_LOAD32_BYTE( "ff_5-1_5fa_512.ic15", 0x40002, 0x10000, CRC(36fa650e) SHA1(8226bba7d75a5112883e30c8012d18bc7c1acd11) )
	ROM_LOAD32_BYTE( "ff_5-1_5fa_517.ic20", 0x00001, 0x10000, CRC(ed7d42e5) SHA1(52a453da9bbf11846bbe8e1121b56a869b93ac65) )
	ROM_LOAD32_BYTE( "ff_5-1_5fa_518.ic21", 0x40001, 0x10000, CRC(1106d842) SHA1(002b785ed575c3ae6514d866c070dab83323a8b6) )
	ROM_LOAD32_BYTE( "ff_5-1_5fa_523.ic26", 0x00000, 0x10000, CRC(a541c879) SHA1(53133cb2ad5378f101025fc758d08a54e23f3c87) )
	ROM_LOAD32_BYTE( "ff_5-1_5fa_524.ic27", 0x40000, 0x10000, CRC(1e7351c5) SHA1(88d83b45f25fe58726e5158b97395443cf065564) )

	ROM_REGION( 0x200, "mod21", 0 ) // on MOD 21/1 board
	ROM_LOAD( "ff_21-1_201_82s129.ic4",  0x000, 0x100, CRC(2697da58) SHA1(e62516b886ff6e204b718e5f0c6ce2712e4b7fc5) )
	ROM_LOAD( "ff_21-1_205_82s129.ic12", 0x100, 0x100, CRC(204a7aee) SHA1(322164134aa65c37a9389024f921364a81d13e88) )

	ROM_REGION( 0x1000, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "ff_6-1_604_gal16v8.ic13",      0x000, 0x117, NO_DUMP )
	ROM_LOAD( "ff_6-1_635_gal16v8.ic7",       0x200, 0x117, CRC(e23c6566) SHA1(2f2455246ff3478649560815a7b700cba64cdf71) )
	ROM_LOAD( "ff_4-3_p0403_pal16r8a.ic29",   0x400, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "ff_4-3-a_p0402_pal16r8a.ic29", 0x600, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "ff_4-3-b_p0403_pal16r8a.ic29", 0x800, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "ff_51-3_p0503_pal16r6a.ic46",  0xa00, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
	ROM_LOAD( "ff_5-1_5135_gal16v8.ic9",      0xc00, 0x117, CRC(80d10466) SHA1(b35633737eb3565f94e5c90c5a2ad507f8185f11) )
	ROM_LOAD( "ff_5-1_5235_gal16v8.ic8",      0xe00, 0x117, CRC(26d8db4b) SHA1(ced3d00d72756f9370ec69f7fec4327b98d13c91) )
ROM_END

ROM_START( secretagm )
	ROM_REGION( 0x40000, "maincpu", 0 ) // on MOD 6/1 board
	ROM_LOAD16_BYTE( "mod_6-1_ag_603.ic17", 0x00000, 0x10000, CRC(4a61489b) SHA1(41f3b99bbd60ebac567477b98dc7c0158f972d60) )
	ROM_LOAD16_BYTE( "mod_6-1_ag_606.ic8",  0x00001, 0x10000, CRC(94dfe5a1) SHA1(01a4c25fa883b0a4bea5d8555e38eb40c302b6d5) )
	ROM_LOAD16_BYTE( "mod_6-1_ag_602.ic20", 0x20000, 0x10000, CRC(bc67dfa0) SHA1(0e32ab1ef9008e95c2e0b8a6916766b255681987) )
	ROM_LOAD16_BYTE( "mod_6-1_ag_605.ic11", 0x20001, 0x10000, CRC(ba149ce8) SHA1(6c405a670983a5c3b5c98248298924b87c350274) )

	ROM_REGION( 0x10000, "mod1", 0 ) // on MOD 1/2 board
	ROM_LOAD( "mod_1-2_ag_110.ic12", 0x00000, 0x10000, CRC(7446d17e) SHA1(be0c2c5359af20e17f76c0a5ea59de9357735ab8) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "mod_1-2_110_82s123.ic20", 0x00, 0x20, CRC(e26e680a) SHA1(9bbe30e98e952a6113c64e1171330153ddf22ce7) )

	ROM_REGION( 0x20000, "mod4_tx", ROMREGION_INVERT ) // on MOD 4/3 board
	ROM_LOAD32_BYTE( "mod_4-3_0_ag_401.ic17", 0x00003, 0x8000, CRC(0a56829f) SHA1(444d3ef0c701810d8f2a53fc42565d5d04073b1c) )
	ROM_LOAD32_BYTE( "mod_4-3_0_ag_402.ic16", 0x00002, 0x8000, CRC(7a4bf6b0) SHA1(63a1d0e10903fc2548a24126961d057e8b5806bd) )
	ROM_LOAD32_BYTE( "mod_4-3_0_ag_403.ic15", 0x00001, 0x8000, CRC(dec5f0dc) SHA1(6df6c0d1c3f83b0cc896083f0dce3ce93f67a49b) )
	ROM_LOAD32_BYTE( "mod_4-3_0_ag_404.ic14", 0x00000, 0x8000, CRC(4b00d933) SHA1(dd8113f24f8a9f408fbf3e955c8bf61c89744e53) )

	ROM_REGION( 0x20000, "mod4_a", ROMREGION_INVERT ) // on MOD 4/3-A board
	ROM_LOAD32_BYTE( "mod_4-3_a_ag_4a1.ic17", 0x00003, 0x8000, CRC(58e496f2) SHA1(2cabd84bff6047be81f79c3cd55f480828aa3d3d) )
	ROM_LOAD32_BYTE( "mod_4-3_a_ag_4a2.ic16", 0x00002, 0x8000, CRC(3bbd9e82) SHA1(79926cd1f227762174de50f9af62f54060021a63) )
	ROM_LOAD32_BYTE( "mod_4-3_a_ag_4a3.ic15", 0x00001, 0x8000, CRC(3fd968a0) SHA1(ddecb3f01ce6227653607878c933bd17c96483e6) )
	ROM_LOAD32_BYTE( "mod_4-3_a_ag_4a4.ic14", 0x00000, 0x8000, CRC(960007ad) SHA1(9e7058385110acc149ba42c8ec64bd871debd0b9) )

	ROM_REGION( 0x40000, "mod4_b", ROMREGION_INVERT ) // on MOD 4/3-B board
	ROM_LOAD32_BYTE( "mod_4-3_b_ag_4b1.ic17", 0x00003, 0x10000, CRC(e06bc56d) SHA1(5a189c7f6e131fc6d6eebd2bb1c9c8c3cff9d6f8) )
	ROM_LOAD32_BYTE( "mod_4-3_b_ag_4b2.ic16", 0x00002, 0x10000, CRC(f46fd920) SHA1(925f0c9c5d2990dcff09a18363b88a69c19f0d01) )
	ROM_LOAD32_BYTE( "mod_4-3_b_ag_4b3.ic15", 0x00001, 0x10000, CRC(51ef5441) SHA1(eb06022174e177f6785d1b621e6e3fdaca67c122) )
	ROM_LOAD32_BYTE( "mod_4-3_b_ag_4b4.ic14", 0x00000, 0x10000, CRC(3beeddde) SHA1(1127d78a9b29787aaae47e0b4359be7b2528f6d9) )

	ROM_REGION( 0x80000, "sprites", ROMREGION_INVERT ) // on MODULAR SYSTEM 2 MOD 5/1 board, two EPROMs for each bitplane
	ROM_LOAD32_BYTE( "mod_5-1_ag_501.ic5",  0x00003, 0x10000, CRC(d234cae5) SHA1(0cd07bf087a4da19a5da29785385de9eee52d0fb) )
	ROM_LOAD32_BYTE( "mod_5-1_ag_502.ic6",  0x40003, 0x10000, CRC(dc6a38df) SHA1(9043df911389d3f085299f2f2202cab356473a32) )
	ROM_LOAD32_BYTE( "mod_5-1_ag_503.ic14", 0x00002, 0x10000, CRC(447e4f0b) SHA1(97db103e505a6e11eb9bdb3622e4aa3b796a9714) )
	ROM_LOAD32_BYTE( "mod_5-1_ag_504.ic15", 0x40002, 0x10000, CRC(d29bc22e) SHA1(ce0935d09f7e94fa32247c86e14a74b73514b29e) )
	ROM_LOAD32_BYTE( "mod_5-1_ag_505.ic20", 0x00001, 0x10000, CRC(bd17ddc5) SHA1(edb847378c4a95b18bd3b49b6cbf8c06598d9d3b) )
	ROM_LOAD32_BYTE( "mod_5-1_ag_506.ic21", 0x40001, 0x10000, CRC(f61972c8) SHA1(fa9ddca3473091b4879171d8f3b302e8f2b45149) )
	ROM_LOAD32_BYTE( "mod_5-1_ag_507.ic26", 0x00000, 0x10000, CRC(ff72b838) SHA1(fdc48ecdd2225fc69472313f34973f6add8fb558) )
	ROM_LOAD32_BYTE( "mod_5-1_ag_508.ic27", 0x40000, 0x10000, CRC(54fcbc39) SHA1(293a6799193b01424c3eac86cf90cc023aa771db) )

	ROM_REGION( 0x200, "mod21", 0 ) // on MOD 21/1 board
	ROM_LOAD( "mod_21-1_201_82s129.ic4",    0x000, 0x100, CRC(2697da58) SHA1(e62516b886ff6e204b718e5f0c6ce2712e4b7fc5) )
	ROM_LOAD( "mod_21-1_205_82s129.ic12",   0x100, 0x100, CRC(204a7aee) SHA1(322164134aa65c37a9389024f921364a81d13e88) )

	ROM_REGION( 0x1000, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "mod_6-1_604_gal16v8.ic13",      0x000, 0x117, NO_DUMP )
	ROM_LOAD( "mod_6-1_637_gal16v8.ic7",       0x200, 0x117, CRC(c621d812) SHA1(fef3a6cdce3e42f9c6e8ad4590a1c6491f247c60) )
	ROM_LOAD( "mod_4-3_0_10403_pal16r8a.ic29", 0x400, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "mod_4-3_a_p0403_pal16r8.ic29",  0x600, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "mod_4-3_b_403_pal16r8.ic29",    0x800, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "mod_51-3_503_gal16v8.ic46",     0xa00, 0x117, CRC(11470ea1) SHA1(cfcafbcc7e55be717348f895df61e144fdd0cc9b) )
	ROM_LOAD( "mod_5-1_5137_gal16v8.ic9",      0xc00, 0x117, CRC(80d10466) SHA1(b35633737eb3565f94e5c90c5a2ad507f8185f11) )
	ROM_LOAD( "mod_5-1_5237_gal16v8.ic8",      0xe00, 0x117, CRC(26d8db4b) SHA1(ced3d00d72756f9370ec69f7fec4327b98d13c91) )
ROM_END

} // anonymous namespace


GAME( 199?, automatm,  robocop,  automatm,  automatm,  automat_ms_state,  empty_init, ROT0, "bootleg (Gaelco / Ervisa)", "Automat (bootleg of Robocop, Modular System)", MACHINE_NOT_WORKING )
GAME( 199?, drgninjam, baddudes, drgninjam, drgninjam, ffantasy_ms_state, empty_init, ROT0, "bootleg (Gaelco / Ervisa)", "Dragon Ninja (Modular System)",                MACHINE_NOT_WORKING )
GAME( 199?, ffantasym, hippodrm, ffantasym, ffantasym, ffantasy_ms_state, empty_init, ROT0, "bootleg (Gaelco / Ervisa)", "Fighting Fantasy (Modular System)",            MACHINE_NOT_WORKING )
GAME( 199?, secretagm, secretag, secretagm, secretagm, ffantasy_ms_state, empty_init, ROT0, "bootleg (Gaelco / Ervisa)", "Secret Agent (Modular System)",                MACHINE_NOT_WORKING )
