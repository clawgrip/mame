// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 3 6809 sub board PAL

    The CPU sub board with two 6809s keeps the bank bits for its decoder
    PROM in the registers of a PAL20X4 (P36). This device evaluates the PAL
    from its fuse map, laid out as jedutil does.

    When pins 4, 6 and 9 are low at the clock, the register of pin 18 takes
    pin 2, the one of pin 19 takes pin 10 and the one of pin 20 takes the
    previous pin 19. The programs write each bank number twice, in bits 0-2
    and then shifted one bit up, so with D1 on pin 2 and D2 on pin 10 pins
    18, 19 and 20 end up with bank bits 0, 1 and 2. D3 on pin 2 and D1 on
    pin 10 would fit the programs as well and give the same banks.

    TODO:
    - what drives pins 1 and 3-11; here each write to the latch area clocks
      the PAL once with pins 4, 6 and 9 low
    - what the combinatorial outputs and pin 17 do (bus timing for the two
      CPUs?)

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod3_pal.h"

#include "ioprocs.h"
#include "jedparse.h"


namespace {

// PAL20X4 fuse map: 40 product terms of 40 fuses, an intact fuse connects its column
struct pal_input { u8 pin, complement, level; };
constexpr pal_input INPUTS[] = {
		{  2,  1,  0 }, {  3,  5,  4 }, {  4,  9,  8 }, {  5, 13, 12 }, {  6, 17, 16 },
		{  7, 21, 20 }, {  8, 25, 24 }, {  9, 29, 28 }, { 10, 33, 32 }, { 11, 37, 36 },
		{ 14, 39, 38 }, { 15, 35, 34 }, { 16, 31, 30 }, { 17, 27, 26 }, { 18, 23, 22 },
		{ 19, 19, 18 }, { 20, 15, 14 }, { 21, 11, 10 }, { 22,  7,  6 }, { 23,  3,  2 } };

// active low combinatorial outputs: an output enable term, then three terms
struct pal_output { u8 pin, row; };
constexpr pal_output COMBINATORIAL[] = { { 14, 36 }, { 15, 32 }, { 16, 28 }, { 21, 8 }, { 22, 4 }, { 23, 0 } };

// registered outputs: two pairs of terms, each pair ORed, XORed into the flip-flop
constexpr pal_output REGISTERED[] = { { 17, 24 }, { 18, 20 }, { 19, 16 }, { 20, 12 } };

constexpr u32 INPUT_PINS = 0x0ffc; // pins 2-11

} // anonymous namespace


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD3_PAL, gaelco_ms_mod3_pal_device, "gaelco_ms_mod3_pal", "Gaelco Modular System MOD 3 6809 sub board PAL")


gaelco_ms_mod3_pal_device::gaelco_ms_mod3_pal_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MOD3_PAL, tag, owner, clock),
	m_fuses(*this, DEVICE_SELF),
	m_outputs_cb(*this),
	m_high{ },
	m_low{ },
	m_regs(0)
{
}

void gaelco_ms_mod3_pal_device::device_start()
{
	jed_data jed;
	auto file = util::ram_read(m_fuses.target(), m_fuses.bytes());
	if (!file || (jedbin_parse(*file, &jed) != JEDERR_NONE) || (jed.numfuses != 40 * 40))
		throw emu_fatalerror("%s: bad PAL20X4 fuse map\n", tag());

	for (unsigned row = 0; row < 40; row++)
	{
		for (auto const &input : INPUTS)
		{
			if (!jed_get_fuse(&jed, row * 40 + input.level))
				m_high[row] |= 1U << input.pin;
			if (!jed_get_fuse(&jed, row * 40 + input.complement))
				m_low[row] |= 1U << input.pin;
		}
	}

	// the power-on state is unknown: start with the outputs low, as the decoder starts with bank 0
	m_regs = 0x0f;

	save_item(NAME(m_regs));
}


u32 gaelco_ms_mod3_pal_device::levels(u32 inputs) const
{
	// registered outputs feed back from the flip-flops
	u32 levels = inputs & INPUT_PINS;
	for (unsigned i = 0; i < std::size(REGISTERED); i++)
	{
		if (!BIT(m_regs, i))
			levels |= 1U << REGISTERED[i].pin;
	}

	// combinatorial outputs feed back from their pins, so let them settle
	for (auto const &output : COMBINATORIAL)
		levels |= 1U << output.pin;
	for (unsigned pass = 0; pass < std::size(COMBINATORIAL); pass++)
	{
		u32 const previous = levels;
		for (auto const &output : COMBINATORIAL)
		{
			if (term(output.row, levels))
			{
				if (term(output.row + 1, levels) || term(output.row + 2, levels) || term(output.row + 3, levels))
					levels &= ~(1U << output.pin);
				else
					levels |= 1U << output.pin;
			}
		}
		if (levels == previous)
			break;
	}

	return levels;
}

void gaelco_ms_mod3_pal_device::clock(u32 inputs)
{
	u32 const pins = levels(inputs);
	u8 regs = 0;
	for (unsigned i = 0; i < std::size(REGISTERED); i++)
	{
		unsigned const row = REGISTERED[i].row;
		bool const first = term(row, pins) || term(row + 1, pins);
		bool const second = term(row + 2, pins) || term(row + 3, pins);
		regs |= u8(first != second) << i;
	}
	m_regs = regs;
}


void gaelco_ms_mod3_pal_device::latch_w(u8 data)
{
	u32 inputs = INPUT_PINS & ~((1U << 2) | (1U << 4) | (1U << 6) | (1U << 9) | (1U << 10));
	inputs |= (u32(BIT(data, 1)) << 2) | (u32(BIT(data, 2)) << 10);
	clock(inputs);

	m_outputs_cb(~m_regs & 0x0f);
}
