// license:BSD-3-Clause
// copyright-holders: Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_CRED2X1_H
#define MAME_GAELCO_GAELCO_CRED2X1_H

#pragma once

#include "cpu/pic16c5x/pic16c5x.h"
#include "video/pwm.h"


class gaelco_cred2x1_base : public device_t
{
public:
	// CONT1 and CONT2
	template <unsigned N> auto meter_callback() { return m_meter_cb[N].bind(); }

	// LUZ, the lamps in both credit buttons
	auto lamp_callback() { return m_lamp_cb.bind(); }

	auto digit_callback() { return m_digit_cb.bind(); }

	// ULN2003 outputs to the games, CRED1 and CRED2
	template <unsigned N> int credit_r() { return BIT(~m_latch, 1 + 2 * N); }

protected:
	gaelco_cred2x1_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock);

	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

private:
	void porta_w(offs_t offset, u8 data, u8 mem_mask);
	u8 portb_r();
	void portb_w(u8 data);

	void latch_w(u8 data);
	void update_display();
	void digit_w(offs_t offset, u64 data) { m_digit_cb(data); }

	required_device<pic16c54_device> m_pic;
	required_device<pwm_display_device> m_display;
	required_ioport m_in;
	required_ioport m_dsw;
	required_ioport m_config;

	devcb_write_line::array<2> m_meter_cb;
	devcb_write_line m_lamp_cb;
	devcb_write8 m_digit_cb;

	u8 m_porta;
	u8 m_portb;
	u8 m_latch;
};


// the programs that follow the manual
class gaelco_cred2x1_device : public gaelco_cred2x1_base
{
public:
	gaelco_cred2x1_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

protected:
	// device_t implementation
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
};

DECLARE_DEVICE_TYPE(GAELCO_CRED2X1, gaelco_cred2x1_device)


class gaelco_cred2x1_covi_device : public gaelco_cred2x1_base
{
public:
	static auto parent_rom_device_type() { return &GAELCO_CRED2X1; }

	gaelco_cred2x1_covi_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

protected:
	// device_t implementation
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
};

DECLARE_DEVICE_TYPE(GAELCO_CRED2X1_COVI, gaelco_cred2x1_covi_device)

#endif // MAME_GAELCO_GAELCO_CRED2X1_H
