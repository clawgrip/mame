// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System GAL evaluation

    The MOD 9/2 and COMP MOD-A boards take their video timing from a
    GAL20V8 in registered mode, and the MOD 9/2 its sound CPU decoding from
    a GAL16V8 in complex mode. This evaluates their fuse maps: each input
    has a column pair in the array, an intact fuse connects a column to its
    product term, and each macrocell sums eight terms. In complex mode and
    in the combinatorial macrocells of registered mode the first term
    enables the output; a disabled output leaves its pin to the board. The
    array sees the flip-flops of the registered macrocells, which the pins
    show inverted. A product term with all its fuses blown is left out of
    the sums, as jedutil does.

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_gal.h"

#include "ioprocs.h"
#include "jedparse.h"

#include <iterator>


namespace {

// a column pair of the array: its pin and the fuse columns of the complement and of the level
struct gal_column { u8 pin, complement, level; };

constexpr gal_column GAL16V8_COMPLEX_COLUMNS[] = {
		{  1,  3,  2 }, {  2,  1,  0 }, {  3,  5,  4 }, {  4,  9,  8 }, {  5, 13, 12 }, {  6, 17, 16 }, {  7, 21, 20 },
		{  8, 25, 24 }, {  9, 29, 28 }, { 11, 31, 30 }, { 13, 27, 26 }, { 14, 23, 22 }, { 15, 19, 18 }, { 16, 15, 14 },
		{ 17, 11, 10 }, { 18,  7,  6 } };

constexpr gal_column GAL20V8_REGISTERED_COLUMNS[] = {
		{  2,  1,  0 }, {  3,  5,  4 }, {  4,  9,  8 }, {  5, 13, 12 }, {  6, 17, 16 }, {  7, 21, 20 }, {  8, 25, 24 },
		{  9, 29, 28 }, { 10, 33, 32 }, { 11, 37, 36 }, { 14, 39, 38 }, { 15, 35, 34 }, { 16, 31, 30 }, { 17, 27, 26 },
		{ 18, 23, 22 }, { 19, 19, 18 }, { 20, 15, 14 }, { 21, 11, 10 }, { 22,  7,  6 }, { 23,  3,  2 } };

// 64 product terms, then the XOR, AC1 and product term disable fuses of each macrocell, and the mode fuses SYN and AC0
struct gal_layout
{
	char const *name;
	u32 fuses;
	u32 term_fuses;
	gal_column const *columns;
	u32 column_count;
	u32 xor_fuse;
	u32 ac1_fuse;
	u32 ptd_fuse;
	u32 syn_fuse;
	bool syn;
	u32 first_pin;
};

constexpr gal_layout GAL16V8_COMPLEX{ "GAL16V8 in complex mode", 2194, 32, GAL16V8_COMPLEX_COLUMNS,
		std::size(GAL16V8_COMPLEX_COLUMNS), 2048, 2120, 2128, 2192, true, 19 };
constexpr gal_layout GAL20V8_REGISTERED{ "GAL20V8 in registered mode", 2706, 40, GAL20V8_REGISTERED_COLUMNS,
		std::size(GAL20V8_REGISTERED_COLUMNS), 2560, 2632, 2640, 2704, false, 22 };

} // anonymous namespace


gaelco_ms_gal::gaelco_ms_gal(type kind, u8 const *data, u32 length, u32 outputs, char const *tag) :
	m_high{ },
	m_low{ },
	m_enabled{ },
	m_blown{ },
	m_first_pin(0),
	m_outputs(0),
	m_registered(0),
	m_active_high(0),
	m_q(0)
{
	gal_layout const &layout = (kind == type::GAL16V8_COMPLEX) ? GAL16V8_COMPLEX : GAL20V8_REGISTERED;

	// AC0, after SYN, is set in both modes
	jed_data jed;
	auto file = util::ram_read(data, length);
	if (!file || (jedbin_parse(*file, &jed) != JEDERR_NONE) || (jed.numfuses != layout.fuses) ||
			(bool(jed_get_fuse(&jed, layout.syn_fuse)) != layout.syn) || !jed_get_fuse(&jed, layout.syn_fuse + 1))
		throw emu_fatalerror("%s: not the fuse map of a %s\n", tag, layout.name);

	m_first_pin = layout.first_pin;
	for (u32 row = 0; row < 64; row++)
	{
		m_enabled[row] = jed_get_fuse(&jed, layout.ptd_fuse + row);
		for (u32 i = 0; i < layout.column_count; i++)
		{
			gal_column const &column = layout.columns[i];
			if (!jed_get_fuse(&jed, row * layout.term_fuses + column.level))
				m_high[row] |= 1U << column.pin;
			if (!jed_get_fuse(&jed, row * layout.term_fuses + column.complement))
				m_low[row] |= 1U << column.pin;
		}
		m_blown[row] = !m_high[row] && !m_low[row];
	}
	for (u32 cell = 0; cell < 8; cell++)
	{
		if (BIT(outputs, m_first_pin - cell))
			m_outputs |= 1U << cell;
		if (!layout.syn && !jed_get_fuse(&jed, layout.ac1_fuse + cell))
			m_registered |= 1U << cell;
		if (jed_get_fuse(&jed, layout.xor_fuse + cell))
			m_active_high |= 1U << cell;
	}
}

bool gaelco_ms_gal::sum(u32 cell, u32 first, u32 levels) const
{
	for (u32 row = cell * 8 + first; row < (cell + 1) * 8; row++)
	{
		if (!m_blown[row] && term(row, levels))
			return true;
	}
	return false;
}

u32 gaelco_ms_gal::array_levels(u32 inputs) const
{
	// the combinatorial outputs can feed each other
	u32 levels = inputs;
	for (u32 cell = 0; cell < 8; cell++)
	{
		if (BIT(m_outputs & m_registered, cell))
			levels = (levels & ~(1U << (m_first_pin - cell))) | (u32(BIT(m_q, cell)) << (m_first_pin - cell));
	}
	for (u32 pass = 0; pass < 8; pass++)
	{
		u32 const previous = levels;
		for (u32 cell = 0; cell < 8; cell++)
		{
			if (BIT(m_outputs & ~m_registered, cell))
			{
				u32 const pin = m_first_pin - cell;
				bool const level = term(cell * 8, levels) ? (sum(cell, 1, levels) == BIT(m_active_high, cell)) : BIT(inputs, pin);
				levels = (levels & ~(1U << pin)) | (u32(level) << pin);
			}
		}
		if (levels == previous)
			break;
	}
	return levels;
}

u32 gaelco_ms_gal::pins(u32 inputs) const
{
	u32 levels = array_levels(inputs);
	for (u32 cell = 0; cell < 8; cell++)
	{
		if (BIT(m_outputs & m_registered, cell))
			levels ^= 1U << (m_first_pin - cell);
	}
	return levels;
}

void gaelco_ms_gal::clock(u32 inputs)
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
