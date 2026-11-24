// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 9/2 board ("SYSTEM 2")

    Sound and I/O board of the later cages: a Z80 with YM2203 or YM3812
    sound and an OKI M5205, two banks of 8 dip switches, optocouplers for
    the inputs, and the video timing. One of its two crystals, divided by
    4, clocks counters that a GAL20V8 (IC18, labelled 92 and the game code)
    decodes into the blanking and the composite sync; the GAL also clears
    the counters at the end of each line and frame. This device has the
    video timing: it evaluates the GAL from its fuse map, laid out as
    jedutil does.

    The four dumped GALs (Big Karnak, Euro League, Splash and World Rally)
    differ only in where they blank and clear the counters. Deduced from
    them and from the games:
    - pins 2-6 take horizontal counter bits 8-4, pin 7 vertical bit 8,
      pins 8-11 vertical bits 7-4 and pin 14 vertical bit 3
    - pin 22 clears the horizontal counter and pin 20 the vertical one, as
      soon as they go high
    - pin 18 is the blanking and pin 19 the composite sync
    All four get 264 lines of 15.1 to 15.6 kHz, Euro League the 256 x 224
    picture of its original game and Big Karnak the 320 pixels wide one of
    its original game.

    The array sees the flip-flops of the registered outputs, which the pins
    show inverted. A product term with all its fuses blown is left out, as
    jedutil does: World Rally's pin 22 has one.

    TODO:
    - pin 21, H6 inverted here: it makes Big Karnak's lines 448 pixels long,
      what its sprites fit, and doesn't change the other games
    - pin 23, high here: Euro League's horizontal sync moves with it
    - what pins 15-17 do
    - sound and I/O, still in the drivers

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod9.h"

#include "ioprocs.h"
#include "jedparse.h"

#include <array>
#include <vector>


namespace {

// GAL20V8 in registered mode: 64 product terms of 40 fuses, an intact fuse connects its column. The macrocells of pins
// 22 down to 15 have eight terms each; the product term disable fuses start at 2640, the XOR fuses at 2560 and the AC1
// fuses at 2632
struct gal_input { u8 pin, complement, level; };
constexpr gal_input INPUTS[] = {
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
		for (auto const &input : INPUTS)
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
	gal20v8 gal(m_gal.target(), m_gal.bytes(), tag());

	// the counter bits on the GAL inputs, see the TODO for pins 21 and 23
	auto const inputs = [] (u16 h, u16 v) -> u32
	{
		return (u32(bitswap<5>(h, 4, 5, 6, 7, 8)) << 2) | (u32(bitswap<5>(v, 4, 5, 6, 7, 8)) << 7) | (u32(BIT(v, 3)) << 14) |
				(u32(BIT(~h, 6)) << 21) | (1U << 23);
	};

	// run the counters for three frames and keep the blanking (bit 0) and the sync (bit 1) of the last one,
	// by counter values
	std::vector<u8> frame(512 * 512, 0xff);
	u16 h = 0, v = 0;
	u32 frames = 0;
	for (u32 step = 0; (frames < 3) && (step < 4 * 512 * 512); step++)
	{
		if (frames == 2)
		{
			u32 const levels = gal.pins(inputs(h, v));
			frame[v * 512 + h] = BIT(levels, 18) | (BIT(levels, 19) << 1);
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

	// the line with the most visible pixels, both signals active low; World Rally's GAL clears the horizontal
	// counter a pixel late and leaves a pixel of the blanking unblanked
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
		if (visible > width)
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
		fatalerror("%s: the GAL doesn't blank or sync the lines\n", tag());

	// down the middle of the visible area
	u16 const middle = (hvisible + width / 2) % htotal;
	u16 vtotal = 0;
	while ((vtotal < 512) && (frame[vtotal * 512 + middle] != 0xff))
		vtotal++;
	visible.resize(vtotal);
	sync.resize(vtotal);
	for (u16 y = 0; y < vtotal; y++)
	{
		visible[y] = BIT(frame[y * 512 + middle], 0);
		sync[y] = !BIT(frame[y * 512 + middle], 1);
	}
	u16 vvisible, vblank, vsync, vsync_end;
	if (!longest_run(visible, vvisible, vblank) || !longest_run(sync, vsync, vsync_end))
		fatalerror("%s: the GAL doesn't blank or sync the frames\n", tag());

	set_timing(*m_screen, clock() / 4, htotal, hvisible, hblank, hsync_end, vtotal, vvisible, vblank, vsync_end);
}
