// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD4_H
#define MAME_GAELCO_GAELCO_MS_MOD4_H

#pragma once

#include "tilemap.h"


class gaelco_ms_mod4_device : public device_t, public device_gfx_interface
{
public:
	// receives the tile map entry, with code and color preset to its bits 0-11 and 12-15
	using tile_delegate = device_delegate<void (u16 data, u32 &code, u32 &color, u8 &flags)>;

	gaelco_ms_mod4_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	void set_tile_size(u8 size) { m_tile_size = size; }
	// the EPROM address lines that select the lower and right halves of 16x16 tiles
	void set_half_lines(u8 lower) { m_half_line = lower; }
	void set_planes(u8 planes) { m_planes = planes; }
	// the order of the tile map in the RAM depends on how the cage connects the board
	void set_layout(tilemap_standard_mapper mapper, u16 columns, u16 rows, u8 step = 1)
	{
		m_mapper = mapper;
		m_columns = columns;
		m_rows = rows;
		m_step = step;
	}
	template <typename F> void set_layout(F &&callback, const char *name, u16 columns, u16 rows)
	{
		m_mapper = TILEMAP_STANDARD_COUNT;
		m_mapper_cb.set(std::forward<F>(callback), name);
		m_columns = columns;
		m_rows = rows;
		m_step = 1;
	}
	void set_color_base(u16 base) { m_color_base = base; }
	void set_transparent_pen(int pen) { m_transparent_pen = pen; }
	// for games that split the tile map entry differently
	template <typename... T> void set_tile_callback(T &&... args) { m_tile_cb.set(std::forward<T>(args)...); }

	// 16-bit hosts see whole entries, 8-bit hosts see the code and attribute bytes apart or in pairs
	u16 vram_r(offs_t offset);
	void vram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u8 code_r(offs_t offset);
	void code_w(offs_t offset, u8 data);
	u8 attr_r(offs_t offset);
	void attr_w(offs_t offset, u8 data);
	u8 vram8_r(offs_t offset);
	void vram8_w(offs_t offset, u8 data);

	void mark_all_dirty() { m_tilemap->mark_all_dirty(); }
	void set_scroll_rows(u32 rows) { m_tilemap->set_scroll_rows(rows); }
	void set_scrollx(int which, int value) { m_tilemap->set_scrollx(which, value); }
	void set_scrollx(int value) { m_tilemap->set_scrollx(value); }
	void set_scrolly(int value) { m_tilemap->set_scrolly(value); }
	void draw(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, u32 flags = 0, u8 priority = 0);

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;

private:
	TILE_GET_INFO_MEMBER(get_tile_info);

	required_region_ptr<u8> m_rom;
	optional_region_ptr<u8> m_prom;
	memory_share_creator<u16> m_ram;
	tile_delegate m_tile_cb;
	tilemap_mapper_delegate m_mapper_cb;

	std::unique_ptr<u8[]> m_tiles;
	tilemap_t *m_tilemap;

	u8 m_tile_size;
	u8 m_half_line;
	u8 m_planes;
	tilemap_standard_mapper m_mapper;
	u16 m_columns;
	u16 m_rows;
	u8 m_step;
	u16 m_color_base;
	int m_transparent_pen;
};


DECLARE_DEVICE_TYPE(GAELCO_MS_MOD4, gaelco_ms_mod4_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD4_H
