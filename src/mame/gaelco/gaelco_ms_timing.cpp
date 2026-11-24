// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System video timing

    The MOD 21/1 board generates the video timing in the earlier cages, from
    two PROMs, and a GAL20V8 sync generator in the later ones: IC18 on the
    MOD 9/2 ("SYSTEM 2") board, labelled 92 and the game code, and IC8 on
    Gals Panic's COMP MOD-A board, labelled a-247. The tile and sprite
    boards position their pictures from its sync and blanking signals.

    The GAL decodes counters clocked by a crystal divided by 4 into the
    blanking and the composite sync, and clears the counters at the end of
    each line and frame. The dumped GALs differ only in where they blank,
    sync and clear the counters. Deduced from them and from the games:
    - pins 2-6 take horizontal counter bits 8-4, pin 7 vertical bit 8,
      pins 8-11 vertical bits 7-4 and pin 14 vertical bit 3
    - pin 22 clears the horizontal counter and pin 20 the vertical one, as
      soon as they go high
    - pin 18 is the blanking and pin 19 the composite sync
    - pin 17 is low from line 256 to the end of the frame, and on Big
      Karnak's, Splash's and World Rally's GALs for lines 0-7 too; the tile
      and sprite boards count lines from its end, as they do from the end
      of the vertical sync pulse in the MOD 21/1 cages (the sprites fit the
      games within 2 lines with it, not with pin 19)
    The array sees the flip-flops of the registered outputs, which the pins
    show inverted. A product term with all its fuses blown is left out, as
    jedutil does: World Rally's pin 22 has one.

    TODO:
    - pin 21, H6 inverted here: it makes Big Karnak's lines 448 pixels long,
      what its sprites fit, and doesn't change the other games
    - pin 23, high here: Euro League's horizontal sync moves with it
    - what pins 15 and 16 do

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_timing.h"

#include "ioprocs.h"
#include "jedparse.h"

#include <array>
#include <vector>


namespace {

// GAL20V8 in registered mode: 64 product terms of 40 fuses, an intact fuse connects its column. The macrocells of pins
// 22 down to 15 have eight terms each; the product term disable fuses start at 2640, the XOR fuses at 2560 and the AC1
// fuses at 2632
struct gal_input { u8 pin, complement, level; };
constexpr gal_input GAL_INPUTS[] = {
		{  2,  1,  0 }, {  3,  5,  4 }, {  4,  9,  8 }, {  5, 13, 12 }, {  6, 17, 16 },
		{  7, 21, 20 }, {  8, 25, 24 }, {  9, 29, 28 }, { 10, 33, 32 }, { 11, 37, 36 },
		{ 14, 39, 38 }, { 15, 35, 34 }, { 16, 31, 30 }, { 17, 27, 26 }, { 18, 23, 22 },
		{ 19, 19, 18 }, { 20, 15, 14 }, { 21, 11, 10 }, { 22,  7,  6 }, { 23,  3,  2 } };

class gal20v8
{
public:
	gal20v8(u8 const *data, u32 length, char const *tag);

	// the pin levels for the levels of the input pins
	u32 pins(u32 inputs) const;
	void clock(u32 inputs);

private:
	bool term(u32 row, u32 levels) const
	{
		return m_enabled[row] && ((levels & m_high[row]) == m_high[row]) && !(levels & m_low[row]);
	}
	bool sum(u32 cell, u32 first, u32 levels) const;
	u32 array_levels(u32 inputs) const;

	std::array<u32, 64> m_high; // the pins each product term needs high
	std::array<u32, 64> m_low; // and low
	std::array<bool, 64> m_enabled;
	std::array<bool, 64> m_blown; // all fuses blown
	u8 m_registered; // macrocell n, pin 22 - n, has a flip-flop
	u8 m_active_high;
	u8 m_q;
};

gal20v8::gal20v8(u8 const *data, u32 length, char const *tag) :
	m_high{ },
	m_low{ },
	m_enabled{ },
	m_blown{ },
	m_registered(0),
	m_active_high(0),
	m_q(0)
{
	jed_data jed;
	auto file = util::ram_read(data, length);
	if (!file || (jedbin_parse(*file, &jed) != JEDERR_NONE) || (jed.numfuses != 2706) || jed_get_fuse(&jed, 2704))
		throw emu_fatalerror("%s: bad GAL20V8 fuse map, or not in registered mode\n", tag);

	for (u32 row = 0; row < 64; row++)
	{
		m_enabled[row] = jed_get_fuse(&jed, 2640 + row);
		for (auto const &input : GAL_INPUTS)
		{
			if (!jed_get_fuse(&jed, row * 40 + input.level))
				m_high[row] |= 1U << input.pin;
			if (!jed_get_fuse(&jed, row * 40 + input.complement))
				m_low[row] |= 1U << input.pin;
		}
		m_blown[row] = !m_high[row] && !m_low[row];
	}
	for (u32 cell = 0; cell < 8; cell++)
	{
		if (!jed_get_fuse(&jed, 2632 + cell))
			m_registered |= 1U << cell;
		if (jed_get_fuse(&jed, 2560 + cell))
			m_active_high |= 1U << cell;
	}
}

bool gal20v8::sum(u32 cell, u32 first, u32 levels) const
{
	for (u32 row = cell * 8 + first; row < (cell + 1) * 8; row++)
	{
		if (!m_blown[row] && term(row, levels))
			return true;
	}
	return false;
}

u32 gal20v8::array_levels(u32 inputs) const
{
	// the array sees the flip-flops and the pins of the combinatorial outputs, which can feed each other
	u32 levels = inputs;
	for (u32 cell = 0; cell < 8; cell++)
	{
		if (BIT(m_registered, cell))
			levels = (levels & ~(1U << (22 - cell))) | (u32(BIT(m_q, cell)) << (22 - cell));
	}
	for (u32 pass = 0; pass < 8; pass++)
	{
		u32 const previous = levels;
		for (u32 cell = 0; cell < 8; cell++)
		{
			// the first term enables the output
			if (!BIT(m_registered, cell) && term(cell * 8, levels))
				levels = (levels & ~(1U << (22 - cell))) | (u32(sum(cell, 1, levels) == BIT(m_active_high, cell)) << (22 - cell));
		}
		if (levels == previous)
			break;
	}
	return levels;
}

u32 gal20v8::pins(u32 inputs) const
{
	u32 levels = array_levels(inputs);
	for (u32 cell = 0; cell < 8; cell++)
	{
		if (BIT(m_registered, cell))
			levels ^= 1U << (22 - cell);
	}
	return levels;
}

void gal20v8::clock(u32 inputs)
{
	u32 const levels = array_levels(inputs);
	u8 q = 0;
	for (u32 cell = 0; cell < 8; cell++)
	{
		if (BIT(m_registered, cell) && (sum(cell, 0, levels) != BIT(m_active_high, cell)))
			q |= 1U << cell;
	}
	m_q = q;
}

// the longest run of set entries, which can wrap around the end: its first entry and the first one after it
bool longest_run(std::vector<bool> const &bits, u16 &start, u16 &end)
{
	u16 const size = bits.size();
	u16 longest = 0;
	start = end = 0;
	for (u16 i = 0; i < size; i++)
	{
		if (bits[i] && !bits[(i ? i : size) - 1])
		{
			u16 length = 0;
			while ((length < size) && bits[(i + length) % size])
				length++;
			if (length > longest)
			{
				longest = length;
				start = i;
				end = (i + length) % size;
			}
		}
	}
	return longest && (longest < size);
}

} // anonymous namespace


device_gaelco_ms_timing_interface::device_gaelco_ms_timing_interface(const machine_config &mconfig, device_t &device) :
	device_interface(device, "gaelco_ms_timing"),
	m_origin_x(0),
	m_origin_y(0),
	m_hsync_to_visible(0),
	m_hblank_to_visible(0),
	m_vsync_to_visible(0)
{
}


void device_gaelco_ms_timing_interface::set_timing(screen_device &screen, u32 pixel_clock, u16 htotal, u16 hvisible,
		u16 hblank, u16 hsync_end, u16 vtotal, u16 vvisible, u16 vblank, u16 vsync_end)
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

void device_gaelco_ms_timing_interface::set_timing_from_gal(screen_device &screen, u32 pixel_clock, u8 const *fuses,
		u32 length)
{
	gal20v8 gal(fuses, length, device().tag());

	// the counter bits on the GAL inputs, see the TODO for pins 21 and 23
	auto const inputs = [] (u16 h, u16 v) -> u32
	{
		return (u32(bitswap<5>(h, 4, 5, 6, 7, 8)) << 2) | (u32(bitswap<5>(v, 4, 5, 6, 7, 8)) << 7) | (u32(BIT(v, 3)) << 14) |
				(u32(BIT(~h, 6)) << 21) | (1U << 23);
	};

	// run the counters for three frames and keep pins 18 (bit 0), 19 (bit 1) and 17 (bit 2) of the last one, by
	// counter values
	std::vector<u8> frame(512 * 512, 0xff);
	u16 h = 0, v = 0;
	u32 frames = 0;
	for (u32 step = 0; (frames < 3) && (step < 4 * 512 * 512); step++)
	{
		if (frames == 2)
		{
			u32 const levels = gal.pins(inputs(h, v));
			frame[v * 512 + h] = BIT(levels, 18) | (BIT(levels, 19) << 1) | (BIT(levels, 17) << 2);
		}
		gal.clock(inputs(h, v));
		h = (h + 1) & 0x1ff;
		if (BIT(gal.pins(inputs(h, v)), 22))
			h = 0;
		if (!h)
			v = (v + 1) & 0x1ff;
		if (v && BIT(gal.pins(inputs(h, v)), 20))
		{
			v = 0;
			frames++;
		}
	}

	// a whole line with the most visible pixels, the signals active low: a registered clear of the vertical counter
	// leaves line 0 a pixel short, and World Rally's GAL, which also clears the horizontal counter a pixel late,
	// leaves a pixel of the blanking unblanked
	u16 line = 0, width = 0, htotal = 0;
	for (u16 y = 0; y < 512; y++)
	{
		u16 visible = 0, total = 0;
		for (u16 x = 0; x < 512; x++)
		{
			u8 const signals = frame[y * 512 + x];
			if (signals != 0xff)
			{
				total++;
				visible += BIT(signals, 0);
			}
		}
		if ((visible > width) || ((visible == width) && (total > htotal)))
		{
			line = y;
			width = visible;
			htotal = total;
		}
	}
	std::vector<bool> visible(htotal), sync(htotal);
	for (u16 x = 0; x < htotal; x++)
	{
		visible[x] = BIT(frame[line * 512 + x], 0);
		sync[x] = !BIT(frame[line * 512 + x], 1);
	}
	u16 hvisible, hblank, hsync, hsync_end;
	if (!width || !longest_run(visible, hvisible, hblank) || !longest_run(sync, hsync, hsync_end))
		fatalerror("%s: the GAL doesn't blank or sync the lines\n", device().tag());

	// down the middle of the visible area, with the vertical sync of the tile and sprite boards
	u16 const middle = (hvisible + width / 2) % htotal;
	u16 vtotal = 0;
	while ((vtotal < 512) && (frame[vtotal * 512 + middle] != 0xff))
		vtotal++;
	visible.resize(vtotal);
	sync.resize(vtotal);
	for (u16 y = 0; y < vtotal; y++)
	{
		visible[y] = BIT(frame[y * 512 + middle], 0);
		sync[y] = !BIT(frame[y * 512 + middle], 2);
	}
	u16 vvisible, vblank, vsync, vsync_end;
	if (!longest_run(visible, vvisible, vblank) || !longest_run(sync, vsync, vsync_end))
		fatalerror("%s: the GAL doesn't blank or sync the frames\n", device().tag());

	set_timing(screen, pixel_clock, htotal, hvisible, hblank, hsync_end, vtotal, vvisible, vblank, vsync_end);
}
