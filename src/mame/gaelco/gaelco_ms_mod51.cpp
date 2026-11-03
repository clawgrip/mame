// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 51 sprite board

    Line buffer logic, a PAL (P0503), four EPROM sockets (or a MODULAR
    SYSTEM 2 MOD 5/1 ROM board on top) and an 82S129 PROM (IC10), 502 in
    every known set. The board inverts the EPROM pixels.

    The PROM decides which pixels are written into the line buffer: its
    outputs are all low for pen 15 (0 in the EPROMs) in all 16 rows.

    TODO:
    - what drives PROM address bits 4-7
    - the sprite list format and the drawing, still done by the drivers

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod51.h"


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD51, gaelco_ms_mod51_device, "gaelco_ms_mod51", "Gaelco Modular System MOD 51 sprite board")


gaelco_ms_mod51_device::gaelco_ms_mod51_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MOD51, tag, owner, clock),
	m_prom(*this, DEVICE_SELF),
	m_transmask(0)
{
}

void gaelco_ms_mod51_device::device_start()
{
	// the pixel is on address bits 0-3
	m_transmask = 0;
	for (unsigned pen = 0; pen < 16; pen++)
	{
		if (!(m_prom[pen] & 0x0f))
			m_transmask |= 1 << pen;
	}
}
