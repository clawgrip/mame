// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote
#ifndef MAME_GAELCO_GAELCO_MS_TIMING_H
#define MAME_GAELCO_GAELCO_MS_TIMING_H

#pragma once

#include "screen.h"


// the board that generates the video timing of a cage (MOD 21/1, MOD 9/2 or COMP MOD-A), which the tile and sprite
// boards position their pictures from
class device_gaelco_ms_timing_interface : public device_interface
{
public:
	// where the first visible pixel and line are in the screen bitmap
	u16 origin_x() const { return m_origin_x; }
	u16 origin_y() const { return m_origin_y; }
	// pixels or lines from the end of the sync pulses or the start of the horizontal blank to the first
	// visible ones, for the boards that position their pictures from those signals
	u16 hsync_to_visible() const { return m_hsync_to_visible; }
	u16 hblank_to_visible() const { return m_hblank_to_visible; }
	u16 vsync_to_visible() const { return m_vsync_to_visible; }

protected:
	device_gaelco_ms_timing_interface(const machine_config &mconfig, device_t &device);

	// takes the totals of a line and a frame, the first visible and the first blanked pixel and line, and the
	// first ones after the sync pulses, and checks the screen against them
	void set_timing(screen_device &screen, u32 pixel_clock, u16 htotal, u16 hvisible, u16 hblank, u16 hsync_end,
			u16 vtotal, u16 vvisible, u16 vblank, u16 vsync_end) ATTR_COLD;
	// takes them from the fuse map of a GAL20V8 sync generator, laid out as jedutil does
	void set_timing_from_gal(screen_device &screen, u32 pixel_clock, u8 const *fuses, u32 length) ATTR_COLD;

private:
	u16 m_origin_x;
	u16 m_origin_y;
	u16 m_hsync_to_visible;
	u16 m_hblank_to_visible;
	u16 m_vsync_to_visible;
};

#endif // MAME_GAELCO_GAELCO_MS_TIMING_H
