// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote
/***************************************************************************

    Gaelco Modular System COMP MOD-A board

    The frame buffer board of Gals Panic: RAM for its two bitmap layers,
    four GALs and a 24 MHz crystal. The crystal divided by 4 clocks counters
    that a GAL20V8 (IC8, labelled a-247) decodes into the blanking and the
    composite sync, as on the MOD 9/2 board, see gaelco_ms_timing.cpp. This
    device has the video timing: 256 x 240 pixels, 16 lines more than the
    original game shows.

    TODO:
    - the bitmap layers, still in the driver

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_moda.h"


DEFINE_DEVICE_TYPE(GAELCO_MS_MODA, gaelco_ms_moda_device, "gaelco_ms_moda", "Gaelco Modular System COMP MOD-A board")


gaelco_ms_moda_device::gaelco_ms_moda_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MODA, tag, owner, clock),
	device_gaelco_ms_timing_interface(mconfig, *this),
	m_screen(*this, finder_base::DUMMY_TAG),
	m_gal(*this, DEVICE_SELF)
{
}

void gaelco_ms_moda_device::device_start()
{
	set_timing_from_gal(*m_screen, clock() / 4, m_gal.target(), m_gal.bytes());
}
