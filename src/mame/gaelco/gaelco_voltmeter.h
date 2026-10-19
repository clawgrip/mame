// license:BSD-3-Clause
// copyright-holders: Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_VOLTMETER_H
#define MAME_GAELCO_GAELCO_VOLTMETER_H

#pragma once

#include "cpu/pic16c5x/pic16c5x.h"
#include "machine/74166.h"
#include "video/pwm.h"


class gaelco_voltmeter_device : public device_t
{
public:
	gaelco_voltmeter_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// two digit display, digit 0 is the tens
	auto digit_callback() { return m_digit_cb.bind(); }

	// pulsed low once per credit
	int credit_r() { return BIT(m_portb, 7); }

protected:
	// device_t implementation
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;

private:
	u8 porta_r() { return m_qh << 2; }
	void porta_w(u8 data);
	void portb_w(u8 data);

	u8 inputs_r();
	void qh_w(int state) { m_qh = state; }
	TIMER_CALLBACK_MEMBER(rtcc_fall);

	void update_display();
	void digit_w(offs_t offset, u64 data) { m_digit_cb(offset, data); }

	required_device<pic16c54_device> m_pic;
	required_device<ttl166_device> m_shifter;
	required_device<pwm_display_device> m_display;
	required_ioport m_in;
	required_ioport m_dsw;
	required_ioport m_sensor;

	devcb_write8 m_digit_cb;

	emu_timer *m_rtcc_timer;

	u8 m_porta;
	u8 m_portb;
	u8 m_qh;
};


DECLARE_DEVICE_TYPE(GAELCO_VOLTMETER, gaelco_voltmeter_device)

#endif // MAME_GAELCO_GAELCO_VOLTMETER_H
