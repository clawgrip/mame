// license:BSD-3-Clause
// copyright-holders: Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_CRED4X4_H
#define MAME_GAELCO_GAELCO_CRED4X4_H

#pragma once

#include "cpu/pic16c5x/pic16c5x.h"
#include "machine/74166.h"
#include "video/pwm.h"


class gaelco_cred4x4_device : public device_t
{
public:
	gaelco_cred4x4_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// coins or credits, depending on the program
	template <unsigned N> auto meter_callback() { return m_meter_cb[N].bind(); }

	// lamps that blink out the number of credits
	template <unsigned N> auto lamp_callback() { return m_lamp_cb[N].bind(); }

	// external credit display
	auto digit_callback() { return m_digit_cb.bind(); }

	// ULN2003A outputs to the game's MON1-MON4 and START1-START4 inputs
	template <unsigned N> int coin_r() { return BIT(~m_coin_latch, N + 2); }
	template <unsigned N> int start_r() { return BIT(~m_start_latch, N + 2); }

protected:
	// device_t implementation
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;

private:
	u8 porta_r();
	void porta_w(u8 data);
	void portb_w(u8 data);

	u8 switches_r();
	u8 inputs_r();
	void qh_w(int state) { m_qh = state; }

	void start_latch_w(u8 data);
	void coin_latch_w(u8 data);
	void update_display();
	void digit_w(offs_t offset, u64 data) { m_digit_cb(data); }

	required_device<pic16c54_device> m_pic;
	required_device_array<ttl166_device, 2> m_shifter;
	required_device<pwm_display_device> m_display;
	required_ioport m_in;
	required_ioport m_dsw;

	devcb_write_line::array<2> m_meter_cb;
	devcb_write_line::array<2> m_lamp_cb;
	devcb_write8 m_digit_cb;

	bool m_direct_display;

	u8 m_porta;
	u8 m_portb;
	u8 m_qh;
	u8 m_start_latch;
	u8 m_coin_latch;
	u8 m_display_shift;
};


DECLARE_DEVICE_TYPE(GAELCO_CRED4X4, gaelco_cred4x4_device)

#endif // MAME_GAELCO_GAELCO_CRED4X4_H
