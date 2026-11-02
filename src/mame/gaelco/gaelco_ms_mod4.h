// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD4_H
#define MAME_GAELCO_GAELCO_MS_MOD4_H

#pragma once

#include "tilemap.h"


class gaelco_ms_mod4_device : public device_t, public device_gfx_interface
{
public:
	gaelco_ms_mod4_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// the order of the tile map in the RAM depends on how the cage connects the board
	void set_layout(tilemap_standard_mapper mapper, u16 columns, u16 rows, u8 step = 1)
	{
		m_mapper = mapper;
		m_columns = columns;
		m_rows = rows;
		m_step = step;
	}
	void set_color_base(u16 base) { m_color_base = base; }
	void set_transparent_pen(int pen) { m_transparent_pen = pen; }

	u16 vram_r(offs_t offset);
	void vram_w(offs_t offset, u16 data, u16 mem_mask = ~0);

	void set_scrollx(int x) { m_tilemap->set_scrollx(0, x); }
	void set_scrolly(int y) { m_tilemap->set_scrolly(0, y); }
	void draw(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, u32 flags = 0, u8 priority = 0);

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;

private:
	TILE_GET_INFO_MEMBER(get_tile_info);

	required_region_ptr<u8> m_rom;
	memory_share_creator<u16> m_ram;

	std::unique_ptr<u8[]> m_tiles;
	tilemap_t *m_tilemap;

	tilemap_standard_mapper m_mapper;
	u16 m_columns;
	u16 m_rows;
	u8 m_step;
	u16 m_color_base;
	int m_transparent_pen;
};


DECLARE_DEVICE_TYPE(GAELCO_MS_MOD4, gaelco_ms_mod4_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD4_H
