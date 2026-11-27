// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote
#ifndef MAME_GAELCO_GAELCO_MS_GAL_H
#define MAME_GAELCO_GAELCO_MS_GAL_H

#pragma once

#include <array>
#include <string_view>


// the fuse map of a GAL16V8 in complex mode or a GAL20V8 in registered mode, laid out as jedutil does, evaluated for the
// Modular System boards that take their decoding or video timing from one
class gaelco_ms_gal
{
public:
	enum class type : u8 { GAL16V8_COMPLEX, GAL20V8_REGISTERED };

	// outputs: the macrocell pins the board takes as outputs, the others stay inputs
	gaelco_ms_gal(type kind, u8 const *data, u32 length, u32 outputs, std::string_view tag);

	// the pin levels for the levels of the input pins
	u32 pins(u32 inputs) const;
	// clocks the flip-flops of the registered outputs
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
	u32 m_first_pin; // macrocell n drives pin m_first_pin - n
	u8 m_outputs;
	u8 m_registered;
	u8 m_active_high;
	u8 m_q;
};

#endif // MAME_GAELCO_GAELCO_MS_GAL_H
