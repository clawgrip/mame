// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 21/1 board

    Video timing and palette: a crystal, two 82S129 timing PROMs (IC4
    horizontal, IC12 vertical) and the palette RAM, three UM2148 for 1024
    entries of 12 bits that the CPU sees as a green and red byte and a blue
    byte.

    The pixel clock is the crystal divided by 4 and each PROM entry lasts two
    pixels or two lines. The entry with bit 3 set is the last one, bit 2 is
    the sync and the picture is blanked by bit 0 of the horizontal PROM and
    bit 1 of the vertical one. Bit 1 of the horizontal PROM opens the window
    in which the other boards fetch their pixels, two entries before the
    visible area. Bit 0 of the vertical PROM pulses several times per frame.

    Known PROMs (the labels sometimes have a P0 prefix), total and visible
    pixels or lines:
    201  horizontal  320  256  20 MHz  Black Tiger, Cabal, Double Dragon, Dragon Ninja,
                                       Fighting Fantasy, Secret Agent, Shadow Warriors,
                                       Slap Fight, Toki
    204  horizontal  450  356  28 MHz  Last Duel, Legend of Hero Tonma, Power Spikes
    206  horizontal  384  280  24 MHz  Gryzor
    207  horizontal  384  320  24 MHz  Twin Cobra
    210  horizontal  450  338          Bad Lands
    211  horizontal  384  256  24 MHz  Blood Bros., Raiden
    202  vertical    266  224          Black Tiger, Blood Bros., Cabal, Gryzor, Raiden,
                                       Shadow Warriors, Toki
    203  vertical    266  224          Last Duel, Slap Fight
    205  vertical    266  240          Bad Lands, Double Dragon, Dragon Ninja, Fighting Fantasy,
                                       Power Spikes (labelled 209), Secret Agent, Twin Cobra
    209  vertical    278  256          Legend of Hero Tonma

    TODO:
    - the palette RAM is still mapped by the drivers
    - what uses bit 0 of the vertical PROM, maybe the MOD 1 interrupt

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod21.h"


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD21, gaelco_ms_mod21_device, "gaelco_ms_mod21", "Gaelco Modular System MOD 21/1 board")


gaelco_ms_mod21_device::gaelco_ms_mod21_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MOD21, tag, owner, clock),
	m_screen(*this, finder_base::DUMMY_TAG),
	m_prom(*this, DEVICE_SELF),
	m_origin_x(0),
	m_origin_y(0)
{
}

void gaelco_ms_mod21_device::device_start()
{
	if (!m_screen->started())
		throw device_missing_dependencies();

	// the horizontal PROM is at 0x000 and the vertical one at 0x100, two pixels or lines per entry
	u16 const hentries = entries(0x000);
	u16 const ventries = entries(0x100);
	u16 const htotal = hentries * 2;
	u16 const vtotal = ventries * 2;
	rectangle const visarea(
			m_origin_x, m_origin_x + visible(0x000, hentries, 0) * 2 - 1,
			m_origin_y, m_origin_y + visible(0x100, ventries, 1) * 2 - 1);
	if ((visarea.right() >= htotal) || (visarea.bottom() >= vtotal))
		fatalerror("%s: the visible area doesn't fit in the %d x %d frame from the origin\n", tag(), htotal, vtotal);

	// counted from the first visible pixel and line, which go to the origin
	m_screen->configure(htotal, vtotal, visarea, attotime::from_ticks(u64(htotal) * vtotal, clock() / 4));
}


u16 gaelco_ms_mod21_device::entries(offs_t base) const
{
	for (u16 i = 0; i < 0x100; i++)
	{
		if (BIT(m_prom[base + i], 3))
			return i + 1;
	}
	fatalerror("%s: no last entry in the timing PROM at %03x\n", tag(), base);
}

u16 gaelco_ms_mod21_device::visible(offs_t base, u16 entries, u8 blank_bit) const
{
	// a single run of entries, which can wrap around the end
	u16 count = 0;
	u16 runs = 0;
	for (u16 i = 0; i < entries; i++)
	{
		if (!BIT(m_prom[base + i], blank_bit))
		{
			count++;
			if (BIT(m_prom[base + (i ? i : entries) - 1], blank_bit))
				runs++;
		}
	}
	if (runs != 1)
		fatalerror("%s: the visible area isn't a single run in the timing PROM at %03x\n", tag(), base);

	return count;
}
