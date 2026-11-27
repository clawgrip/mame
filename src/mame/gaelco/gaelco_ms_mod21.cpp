// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote
/***************************************************************************

    Gaelco Modular System MOD 21/1 board

    Video timing and palette: a crystal, two 82S129 timing PROMs (IC4
    horizontal, IC12 vertical) and the palette RAM, three UM2148 for 1024
    entries of 12 bits that the CPU sees as a green and red byte and a blue
    byte, selected by A10.

    The pixel clock is the crystal divided by 4 and each PROM entry lasts two
    pixels or two lines. The entry with bit 3 set is the last one, bit 2 is
    the sync and the picture is blanked by bit 0 of the horizontal PROM and
    bit 1 of the vertical one. Bit 1 of the horizontal PROM opens the window
    in which the other boards fetch their pixels, two entries before the
    visible area. Bit 0 of the vertical PROM pulses several times per frame.
    The drivers configure the screen with this timing, and the device checks
    it against the PROMs.

    The other boards position their pictures from these signals: the MOD 4
    boards count from the end of the sync pulses and the MOD 51 board from
    the start of the horizontal blank. There are no schematics; this fits
    what the games write (see those devices).

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
    - what uses bit 0 of the vertical PROM, maybe the MOD 1 interrupt

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod21.h"


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD21, gaelco_ms_mod21_device, "gaelco_ms_mod21", "Gaelco Modular System MOD 21/1 board")


gaelco_ms_mod21_device::gaelco_ms_mod21_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MOD21, tag, owner, clock),
	device_palette_interface(mconfig, *this),
	device_gaelco_ms_timing_interface(mconfig, *this),
	m_screen(*this, finder_base::DUMMY_TAG),
	m_prom(*this, DEVICE_SELF),
	m_green_red{ },
	m_blue{ }
{
}

void gaelco_ms_mod21_device::device_start()
{
	// the horizontal PROM is at 0x000 and the vertical one at 0x100, two pixels or lines per entry
	u16 const hentries = entries(0x000);
	u16 const ventries = entries(0x100);

	u16 hvisible, hblank, hsync, hsync_end, vvisible, vblank, vsync, vsync_end;
	low_run(0x000, hentries, 0, hvisible, hblank);
	low_run(0x000, hentries, 2, hsync, hsync_end);
	low_run(0x100, ventries, 1, vvisible, vblank);
	low_run(0x100, ventries, 2, vsync, vsync_end);
	set_timing(*m_screen, clock() / 4, hentries * 2, hvisible * 2, hblank * 2, hsync_end * 2,
			ventries * 2, vvisible * 2, vblank * 2, vsync_end * 2);

	save_item(NAME(m_green_red));
	save_item(NAME(m_blue));
}

u32 gaelco_ms_mod21_device::palette_entries() const noexcept
{
	return 0x400;
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

void gaelco_ms_mod21_device::low_run(offs_t base, u16 entries, u8 bit, u16 &start, u16 &end) const
{
	// a single run of entries with the bit low, which can wrap around the end
	unsigned runs = 0;
	start = end = 0;
	for (u16 i = 0; i < entries; i++)
	{
		bool const low = !BIT(m_prom[base + i], bit);
		bool const previous_low = !BIT(m_prom[base + (i ? i : entries) - 1], bit);
		if (low && !previous_low)
		{
			start = i;
			runs++;
		}
		else if (!low && previous_low)
		{
			end = i;
		}
	}
	if (runs != 1)
		fatalerror("%s: bit %d isn't low in a single run in the timing PROM at %03x\n", tag(), bit, base);
}


u8 gaelco_ms_mod21_device::green_red_r(offs_t offset)
{
	return m_green_red[offset & 0x3ff];
}

void gaelco_ms_mod21_device::green_red_w(offs_t offset, u8 data)
{
	m_green_red[offset & 0x3ff] = data;
	update_pen(offset & 0x3ff);
}

u8 gaelco_ms_mod21_device::blue_r(offs_t offset)
{
	return m_blue[offset & 0x3ff];
}

void gaelco_ms_mod21_device::blue_w(offs_t offset, u8 data)
{
	m_blue[offset & 0x3ff] = data;
	update_pen(offset & 0x3ff);
}

void gaelco_ms_mod21_device::update_pen(offs_t entry)
{
	// red in the low half of the green and red byte, green in the high half, blue in the low half of the blue byte
	set_pen_color(entry, pal4bit(m_green_red[entry]), pal4bit(m_green_red[entry] >> 4), pal4bit(m_blue[entry]));
}
