// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD3_PAL_H
#define MAME_GAELCO_GAELCO_MS_MOD3_PAL_H

#pragma once


class gaelco_ms_mod3_pal_device : public device_t
{
public:
	gaelco_ms_mod3_pal_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// levels of the registered outputs, pins 17-20 in bits 0-3
	auto outputs_callback() { return m_outputs_cb.bind(); }

	// a CPU write to the bank latch area
	void latch_w(u8 data);

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;

private:
	bool term(unsigned row, u32 levels) const { return ((levels & m_high[row]) == m_high[row]) && ((~levels & m_low[row]) == m_low[row]); }
	u32 levels(u32 inputs) const;
	void clock(u32 inputs);

	required_region_ptr<u8> m_fuses;
	devcb_write8 m_outputs_cb;

	// the pins each product term needs high or low, one bit per pin
	std::array<u32, 40> m_high;
	std::array<u32, 40> m_low;

	u8 m_regs; // flip-flops of pins 17-20, the pins show them inverted
};


DECLARE_DEVICE_TYPE(GAELCO_MS_MOD3_PAL, gaelco_ms_mod3_pal_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD3_PAL_H
