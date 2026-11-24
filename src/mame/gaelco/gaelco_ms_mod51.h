// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD51_H
#define MAME_GAELCO_GAELCO_MS_MOD51_H

#pragma once

#include "gaelco_ms_timing.h"


class gaelco_ms_mod51_device : public device_t
{
public:
	// receives the attribute and colour bytes of a list entry, with code preset to the code byte and palette
	// to the low half of the colour byte
	using sprite_delegate = device_delegate<void (u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8)>;
	// receives the attribute and colour bytes of a list entry and returns the GFX_PMASK_n bits of the
	// priority bitmap values whose tile layer pixels cover the sprite
	using priority_delegate = device_delegate<u32 (u8 attr, u8 color)>;

	gaelco_ms_mod51_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// the MOD 21/1 or MOD 9/2 board whose timing the line buffer follows
	template <typename T> void set_timing(T &&tag) { m_timing.set_tag(std::forward<T>(tag)); }
	// list length, where the colour bytes start (0 for none) and whether X has a bit 8
	void set_list(u16 entries, offs_t color_offset, bool x_bit8)
	{
		m_entries = entries;
		m_color_offset = color_offset;
		m_x_bit8 = x_bit8;
	}
	// added to the sprite positions for the games whose pictures the fitted positions don't explain
	void set_position_correction(int x, int y) { m_correction_x = x; m_correction_y = y; }
	// the attribute and colour bits each game's EPROM sub board takes the other code bits, the flips, X bit 8
	// and the palette from
	template <typename... T> void set_sprite_callback(T &&... args) { m_sprite_cb.set(std::forward<T>(args)...); }
	// the attribute and colour bits that put each game's sprites behind tile layers
	template <typename... T> void set_priority_callback(T &&... args) { m_priority_cb.set(std::forward<T>(args)...); }
	// the list RAM as the board sees it: code, Y, X and attributes for each entry, and the colour bytes
	auto list_r() { return m_list_r.bind(); }

	// pens kept out of the line buffer, for sprites decoded from inverted EPROM data
	u32 transmask() const { return m_transmask; }

	void draw(bitmap_ind16 &bitmap, const rectangle &cliprect, gfx_element &gfx);
	// with the tile layers already drawn into the priority bitmap
	void draw(bitmap_ind16 &bitmap, const rectangle &cliprect, gfx_element &gfx, bitmap_ind8 &priority);

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;

private:
	void entry(u32 index, u32 &code, u32 &palette, bool &flipx, bool &flipy, u16 &x, u8 &y, u32 &pmask);
	void draw_list(bitmap_ind16 &bitmap, const rectangle &cliprect, gfx_element &gfx, bitmap_ind8 *priority);

	required_region_ptr<u8> m_prom;
	optional_device<device_gaelco_ms_timing_interface> m_timing;
	devcb_read8 m_list_r;
	sprite_delegate m_sprite_cb;
	priority_delegate m_priority_cb;

	u32 m_transmask;
	u16 m_entries;
	offs_t m_color_offset;
	bool m_x_bit8;
	int m_correction_x;
	int m_correction_y;
	int m_x_delta;
	int m_y_base;
	int m_origin_x;
	int m_origin_y;
};


DECLARE_DEVICE_TYPE(GAELCO_MS_MOD51, gaelco_ms_mod51_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD51_H
