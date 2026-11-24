// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System video timing

    The MOD 21/1 board generates the video timing in the earlier cages and
    the MOD 9/2 ("SYSTEM 2") board in the later ones. The tile and sprite
    boards position their pictures from its sync and blanking signals.

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_timing.h"


gaelco_ms_timing_interface::gaelco_ms_timing_interface(const machine_config &mconfig, device_t &device) :
	device_interface(device, "gaelco_ms_timing"),
	m_origin_x(0),
	m_origin_y(0),
	m_hsync_to_visible(0),
	m_hblank_to_visible(0),
	m_vsync_to_visible(0)
{
}


void gaelco_ms_timing_interface::set_timing(screen_device &screen, u32 pixel_clock, u16 htotal, u16 hvisible, u16 hblank,
		u16 hsync_end, u16 vtotal, u16 vvisible, u16 vblank, u16 vsync_end)
{
	m_hsync_to_visible = (hvisible + htotal - hsync_end) % htotal;
	m_hblank_to_visible = (hvisible + htotal - hblank) % htotal;
	m_vsync_to_visible = (vvisible + vtotal - vsync_end) % vtotal;

	// the visible area of the screen starts at the first visible pixel and line
	u16 const width = (hblank + htotal - hvisible) % htotal;
	u16 const height = (vblank + vtotal - vvisible) % vtotal;
	rectangle const &visarea = screen.visible_area();
	m_origin_x = visarea.left();
	m_origin_y = visarea.top();
	if ((screen.clock() != pixel_clock) || (screen.width() != htotal) || (screen.height() != vtotal) ||
			(visarea.width() != width) || (visarea.height() != height))
	{
		fatalerror("%s: the screen doesn't match the video timing, set_raw(%u, %u, hbend, hbend + %u, %u, vbend, vbend + %u)\n",
				device().tag(), pixel_clock, htotal, width, vtotal, height);
	}
}
