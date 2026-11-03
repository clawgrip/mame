// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD21_H
#define MAME_GAELCO_GAELCO_MS_MOD21_H

#pragma once

#include "screen.h"


class gaelco_ms_mod21_device : public device_t
{
public:
	gaelco_ms_mod21_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	template <typename T> void set_screen(T &&tag) { m_screen.set_tag(std::forward<T>(tag)); }
	// where the first visible pixel and line go in the screen bitmap
	void set_origin(u16 x, u16 y) { m_origin_x = x; m_origin_y = y; }

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;

private:
	u16 entries(offs_t base) const;
	u16 visible(offs_t base, u16 entries, u8 blank_bit) const;

	required_device<screen_device> m_screen;
	required_region_ptr<u8> m_prom;

	u16 m_origin_x;
	u16 m_origin_y;
};


DECLARE_DEVICE_TYPE(GAELCO_MS_MOD21, gaelco_ms_mod21_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD21_H
