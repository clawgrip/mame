// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD9_H
#define MAME_GAELCO_GAELCO_MS_MOD9_H

#pragma once

#include "gaelco_ms_timing.h"

#include "screen.h"


class gaelco_ms_mod9_device : public device_t, public gaelco_ms_timing_interface
{
public:
	gaelco_ms_mod9_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	// the screen, configured with the timing of the GAL
	template <typename T> void set_screen(T &&tag) { m_screen.set_tag(std::forward<T>(tag)); }

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;

private:
	required_device<screen_device> m_screen;
	required_region_ptr<u8> m_gal;
};


DECLARE_DEVICE_TYPE(GAELCO_MS_MOD9, gaelco_ms_mod9_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD9_H
