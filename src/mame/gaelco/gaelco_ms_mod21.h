// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote
#ifndef MAME_GAELCO_GAELCO_MS_MOD21_H
#define MAME_GAELCO_GAELCO_MS_MOD21_H

#pragma once

#include "gaelco_ms_timing.h"

#include "screen.h"


class gaelco_ms_mod21_device : public device_t, public device_palette_interface, public device_gaelco_ms_timing_interface
{
public:
	gaelco_ms_mod21_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	// the screen, configured with the timing of the PROMs
	template <typename T> void set_screen(T &&tag) { m_screen.set_tag(std::forward<T>(tag)); }

	// the palette RAM as the CPU boards see it: a green and red byte and a blue byte for each entry
	u8 green_red_r(offs_t offset);
	void green_red_w(offs_t offset, u8 data);
	u8 blue_r(offs_t offset);
	void blue_w(offs_t offset, u8 data);

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;

	// device_palette_interface implementation
	virtual u32 palette_entries() const noexcept override;

private:
	u16 entries(offs_t base) const ATTR_COLD;
	void low_run(offs_t base, u16 entries, u8 bit, u16 &start, u16 &end) const ATTR_COLD;
	void update_pen(offs_t entry);

	required_device<screen_device> m_screen;
	required_region_ptr<u8> m_prom;

	u8 m_green_red[0x400];
	u8 m_blue[0x400];
};


DECLARE_DEVICE_TYPE(GAELCO_MS_MOD21, gaelco_ms_mod21_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD21_H
