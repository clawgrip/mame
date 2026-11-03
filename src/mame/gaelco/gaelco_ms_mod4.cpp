// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 4 tilemap board

    One scrolling tile layer per board: an 8K x 8 SRAM, up to four EPROMs
    (IC17, IC16, IC15 and IC14), one per bitplane and each shifted out by its
    own 74LS299, a PAL16R8 (P0403 and others) and counters. A long thin sub
    board, different for each game, connects one edge to the cage.

    Tile map entries are 16 bits: usually bits 0-11 tile and bits 12-15
    colour, but some games split them differently. 8-bit hosts see the low
    byte (tile code) and the high byte (attribute) in two separate areas.

    Boards with four EPROMs draw 4bpp tiles in 16 colours, boards with three
    (Twin Cobra's text layer, IC14 empty) 3bpp tiles in 32 colours and boards
    with two 2bpp tiles in 64 colours.

    The tiles are 8x8 or 16x16. 8x8 tiles take the tile row from EPROM A0-A2
    and the tile number from A3 up. 16x16 tiles are made of four 8x8 tiles:
    the lower half comes from A11 and the right half from A12 (A9 and A10 on
    Black Tiger's background board), and the tile number from the other lines
    from A3 up.

    Black Tiger's background board also has an 82S123 (IC13) that picks, for
    each colour, the pens drawn in front of the sprites. Its address lines
    are colour bit 3, pen bits 2 and 3, and colour bits 2 and 1. Those pens
    are drawn with TILEMAP_DRAW_LAYER0 and all pens with TILEMAP_DRAW_LAYER1.

    The EPROMs are interleaved in the region with the most significant
    bitplane first: ROM_LOAD32_BYTE for four EPROMs, ROM_SKIP(2) for three and
    ROM_LOAD16_BYTE for two.

    TODO:
    - scroll registers (the drivers apply their own offsets)
    - how the RAM is organised: the games scan it in different orders, maybe
      set by the edge sub board

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod4.h"


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD4, gaelco_ms_mod4_device, "gaelco_ms_mod4", "Gaelco Modular System MOD 4 tilemap board")


gaelco_ms_mod4_device::gaelco_ms_mod4_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MOD4, tag, owner, clock),
	device_gfx_interface(mconfig, *this),
	m_rom(*this, DEVICE_SELF),
	m_prom(*this, "prom"),
	m_ram(*this, "ram", 0x2000, ENDIANNESS_BIG),
	m_tile_cb(*this),
	m_mapper_cb(*this),
	m_tilemap(nullptr),
	m_tile_size(16),
	m_half_line(11),
	m_planes(4),
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
	m_tile_cb.resolve();

	offs_t const length = m_rom.bytes();
	u8 const *tiles = m_rom.target();
	if (m_tile_size == 16)
	{
		// put the 8x8 quarters of each tile together, as the EPROM address wiring does
		unsigned const low_bits = m_half_line - 3;
		m_tiles = std::make_unique<u8[]>(length);
		for (offs_t i = 0; i < length; i++)
		{
			offs_t const address = i / m_planes;
			offs_t const tile = BIT(address, 3, low_bits) | ((address >> (m_half_line + 2)) << low_bits);
			offs_t const dest = (tile << 5) | (BIT(address, m_half_line + 1) << 4) | (BIT(address, m_half_line) << 3) | (address & 7);
			m_tiles[dest * m_planes + i % m_planes] = m_rom[i];
		}
		tiles = m_tiles.get();
	}

	gfx_layout layout{};
	layout.width = layout.height = m_tile_size;
	layout.planes = m_planes;
	for (unsigned plane = 0; plane < m_planes; plane++)
		layout.planeoffset[plane] = plane * 8;
	for (unsigned i = 0; i < m_tile_size; i++)
	{
		// the right half of a 16x16 tile follows the 16 rows of the left half
		layout.xoffset[i] = (i & 7) + (i >> 3) * 16 * 8 * m_planes;
		layout.yoffset[i] = i * 8 * m_planes;
	}
	layout.charincrement = m_tile_size * m_tile_size * m_planes;
	layout.total = length * 8 / layout.charincrement;
	set_gfx(0, std::make_unique<gfx_element>(&palette(), layout, tiles, 0, 0x100 >> m_planes, m_color_base));

	tilemap_get_info_delegate const get_info(*this, FUNC(gaelco_ms_mod4_device::get_tile_info));
	if (m_mapper == TILEMAP_STANDARD_COUNT)
	{
		m_mapper_cb.resolve();
		m_tilemap = &machine().tilemap().create(*this, get_info, m_mapper_cb, m_tile_size, m_tile_size, m_columns, m_rows);
	}
	else
	{
		m_tilemap = &machine().tilemap().create(*this, get_info, m_mapper, m_tile_size, m_tile_size, m_columns, m_rows);
	}

	if (m_prom)
	{
		// one transparency group per colour
		u16 const transparent = (m_transparent_pen >= 0) ? (1 << m_transparent_pen) : 0;
		for (unsigned color = 0; color < 16; color++)
		{
			u16 front = 0;
			for (unsigned pen = 0; pen < 16; pen++)
				front |= BIT(m_prom[bitswap<5>((color << 4) | pen, 5, 6, 3, 2, 7)], 0) << pen;
			m_tilemap->set_transmask(color, u16(~front) | transparent, transparent);
		}
	}
	else if (m_transparent_pen >= 0)
	{
		m_tilemap->set_transparent_pen(m_transparent_pen);
	}
}


TILE_GET_INFO_MEMBER(gaelco_ms_mod4_device::get_tile_info)
{
	u16 const data = m_ram[tile_index * m_step];
	u32 code = data & 0x0fff;
	u32 color = data >> 12;
	u8 flags = 0;
	if (!m_tile_cb.isnull())
		m_tile_cb(data, code, color, flags);

	tileinfo.set(0, code, color, flags);
	tileinfo.group = m_prom ? color : 0;
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

u8 gaelco_ms_mod4_device::code_r(offs_t offset)
{
	return u8(m_ram[offset]);
}

void gaelco_ms_mod4_device::code_w(offs_t offset, u8 data)
{
	vram_w(offset, data, 0x00ff);
}

u8 gaelco_ms_mod4_device::attr_r(offs_t offset)
{
	return m_ram[offset] >> 8;
}

void gaelco_ms_mod4_device::attr_w(offs_t offset, u8 data)
{
	vram_w(offset, data << 8, 0xff00);
}

u8 gaelco_ms_mod4_device::vram8_r(offs_t offset)
{
	return BIT(offset, 0) ? attr_r(offset >> 1) : code_r(offset >> 1);
}

void gaelco_ms_mod4_device::vram8_w(offs_t offset, u8 data)
{
	if (BIT(offset, 0))
		attr_w(offset >> 1, data);
	else
		code_w(offset >> 1, data);
}

void gaelco_ms_mod4_device::draw(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, u32 flags, u8 priority)
{
	m_tilemap->draw(screen, bitmap, cliprect, flags, priority);
}
