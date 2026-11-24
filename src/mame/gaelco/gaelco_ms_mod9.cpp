// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 9/2 board ("SYSTEM 2")

    Sound and I/O board of the later cages: a Z80 with YM2203 or YM3812
    sound and an OKI M5205, two banks of 8 dip switches, optocouplers for
    the inputs, and the video timing. One of its two crystals, divided by
    4, clocks counters that a GAL20V8 (IC18, labelled 92 and the game code)
    decodes into the blanking and the composite sync, see
    gaelco_ms_timing.cpp. This device has the video timing.

    Euro League's GAL gives the 256 x 224 picture of its original game, Big
    Karnak's the 320 pixels wide one of its original game; Splash's and
    World Rally's give 384 x 240, 16 pixels wider than their original games.

    TODO:
    - sound and I/O, still in the drivers

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod9.h"


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD9, gaelco_ms_mod9_device, "gaelco_ms_mod9", "Gaelco Modular System MOD 9/2 board")


gaelco_ms_mod9_device::gaelco_ms_mod9_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MOD9, tag, owner, clock),
	device_gaelco_ms_timing_interface(mconfig, *this),
	m_screen(*this, finder_base::DUMMY_TAG),
	m_gal(*this, DEVICE_SELF)
{
}

void gaelco_ms_mod9_device::device_start()
{
	set_timing_from_gal(*m_screen, clock() / 4, m_gal.target(), m_gal.bytes());
}
