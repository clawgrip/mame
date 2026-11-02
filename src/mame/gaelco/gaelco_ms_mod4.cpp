// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 4 tilemap board

    One scrolling tile layer per board: an 8K x 8 SRAM, up to four EPROMs
    (IC17, IC16, IC15 and IC14), one per bitplane and each shifted out by its
    own 74LS299, a PAL16R8 (P0403 and others) and counters. A long thin sub
    board, different for each game, connects one edge to the cage.

    Tile map entries are 16 bits: bits 0-11 tile, bits 12-15 colour.

    16x16 tiles are made of four 8x8 tiles: the board drives the EPROMs with
    the tile row in A0-A2, tile bits 0-7 in A3-A10, the lower half in A11,
    the right half in A12 and tile bits 8 and up from A13.

    The EPROMs are loaded with ROM_LOAD32_BYTE, IC17 in the highest byte,
    and their data is inverted.

    TODO:
    - 8x8 tiles (Master Boy, Power Spikes, Slap Fight, text layers)
    - scroll registers (the drivers apply their own offsets)
    - how the RAM is organised: the games scan it in different orders, maybe
      set by the edge sub board

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod4.h"


namespace {

const gfx_layout tiles16x16x4_layout =
{
	16, 16,
	0, // from the EPROM size
	4,
	{ 0, 8, 16, 24 },
	{ STEP8(0, 1), STEP8(16 * 32, 1) },
	{ STEP16(0, 32) },
	16 * 16 * 4
};

} // anonymous namespace


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD4, gaelco_ms_mod4_device, "gaelco_ms_mod4", "Gaelco Modular System MOD 4 tilemap board")


gaelco_ms_mod4_device::gaelco_ms_mod4_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MOD4, tag, owner, clock),
	device_gfx_interface(mconfig, *this),
	m_rom(*this, DEVICE_SELF),
	m_ram(*this, "ram", 0x800, ENDIANNESS_BIG),
	m_tilemap(nullptr),
	m_mapper(TILEMAP_SCAN_ROWS),
	m_columns(32),
	m_rows(32),
	m_step(1),
	m_color_base(0),
	m_transparent_pen(-1)
{
}

void gaelco_ms_mod4_device::device_start()
{
	// put the 8x8 quarters of each tile together, as the A11 and A12 wiring does
	offs_t const length = m_rom.bytes();
	m_tiles = std::make_unique<u8[]>(length);
	for (offs_t i = 0; i < length; i++)
		m_tiles[(i & ~0x7fff) | ((i & 0x1fe0) << 2) | ((i & 0x6000) >> 8) | (i & 0x1f)] = m_rom[i];

	gfx_layout layout = tiles16x16x4_layout;
	layout.total = length / (16 * 16 * 4 / 8);
	set_gfx(0, std::make_unique<gfx_element>(&palette(), layout, m_tiles.get(), 0, 16, m_color_base));

	m_tilemap = &machine().tilemap().create(*this, tilemap_get_info_delegate(*this, FUNC(gaelco_ms_mod4_device::get_tile_info)), m_mapper, 16, 16, m_columns, m_rows);
	if (m_transparent_pen >= 0)
		m_tilemap->set_transparent_pen(m_transparent_pen);
}


TILE_GET_INFO_MEMBER(gaelco_ms_mod4_device::get_tile_info)
{
	u16 const data = m_ram[tile_index * m_step];
	tileinfo.set(0, data & 0x0fff, data >> 12, 0);
}

u16 gaelco_ms_mod4_device::vram_r(offs_t offset)
{
	return m_ram[offset];
}

void gaelco_ms_mod4_device::vram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_ram[offset]);
	m_tilemap->mark_tile_dirty(offset / m_step);
}

void gaelco_ms_mod4_device::draw(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, u32 flags, u8 priority)
{
	m_tilemap->draw(screen, bitmap, cliprect, flags, priority);
}
