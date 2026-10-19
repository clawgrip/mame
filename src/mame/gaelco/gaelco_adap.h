// license:BSD-3-Clause
// copyright-holders: Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_ADAP_H
#define MAME_GAELCO_GAELCO_ADAP_H

#pragma once

#include "cpu/pic16c5x/pic16c5x.h"
#include "machine/74166.h"
#include "video/pwm.h"


class gaelco_adap_base : public device_t
{
public:
	// two digit display, digit 0 is the tens
	auto digit_callback() { return m_digit_cb.bind(); }

protected:
	gaelco_adap_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock);

	// device_t implementation
	virtual void device_start() override ATTR_COLD;

	void common(machine_config &config) ATTR_COLD;
	void common_porta_w(u8 data);

	required_device<pic16c5x_device> m_pic;
	required_device<ttl166_device> m_shifter;
	required_ioport m_in;
	required_ioport m_dsw;

	u8 m_porta;
	u8 m_portb;

private:
	u8 portb_r() { return m_qh; }
	void portb_w(u8 data);
	void qh_w(int state) { m_qh = state; }
	void update_display();
	void digit_w(offs_t offset, u64 data) { m_digit_cb(offset, data); }

	required_device<pwm_display_device> m_display;
	required_ioport m_sensor;

	devcb_write8 m_digit_cb;

	u8 m_qh;
};


// ADAP-TE Portugal
class gaelco_adap_device : public gaelco_adap_base
{
public:
	gaelco_adap_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// pulsed high once per unit of the coin value, and once per credit
	int value_r() { return BIT(m_porta, 3); }
	int credit_r() { return BIT(m_porta, 2); }

protected:
	gaelco_adap_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock);

	// device_t implementation
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;

private:
	u8 acceptor_r();
};

DECLARE_DEVICE_TYPE(GAELCO_ADAP, gaelco_adap_device)


class gaelco_adap_te_device : public gaelco_adap_device
{
public:
	static auto parent_rom_device_type() { return &GAELCO_ADAP; }

	gaelco_adap_te_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

protected:
	// device_t implementation
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
};

DECLARE_DEVICE_TYPE(GAELCO_ADAP_TE, gaelco_adap_te_device)


class gaelco_adap_por_device : public gaelco_adap_device
{
public:
	static auto parent_rom_device_type() { return &GAELCO_ADAP; }

	gaelco_adap_por_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

protected:
	// device_t implementation
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
};

DECLARE_DEVICE_TYPE(GAELCO_ADAP_POR, gaelco_adap_por_device)


class gaelco_adap_ate_device : public gaelco_adap_device
{
public:
	static auto parent_rom_device_type() { return &GAELCO_ADAP; }

	gaelco_adap_ate_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

protected:
	// device_t implementation
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
};

DECLARE_DEVICE_TYPE(GAELCO_ADAP_ATE, gaelco_adap_ate_device)


class gaelco_adap_2x1_device : public gaelco_adap_base
{
public:
	static auto parent_rom_device_type() { return &GAELCO_ADAP; }

	gaelco_adap_2x1_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// CONT1 and CONT2
	template <unsigned N> auto meter_callback() { return m_meter_cb[N].bind(); }

	auto credit_digit_callback() { return m_credit_digit_cb.bind(); }

	// CRED1 and CRED2, pulsed high once per credit
	template <unsigned N> int credit_r() { return BIT(m_latch, 1 + N); }

protected:
	// device_t implementation
	virtual const tiny_rom_entry *device_rom_region() const override ATTR_COLD;
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual ioport_constructor device_input_ports() const override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;

private:
	u8 inputs_r();
	void porta_w(u8 data);
	void latch_w(u8 data);
	void credit_digit_w(offs_t offset, u64 data) { m_credit_digit_cb(data); }

	required_device<pwm_display_device> m_credit_display;

	devcb_write_line::array<2> m_meter_cb;
	devcb_write8 m_credit_digit_cb;

	u8 m_latch;
	u8 m_display_shift;
};

DECLARE_DEVICE_TYPE(GAELCO_ADAP_2X1, gaelco_adap_2x1_device)


class gaelco_adap_vc_device : public device_t
{
public:
	static auto parent_rom_device_type() { return &GAELCO_ADAP; }

	gaelco_adap_vc_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// two digit display, digit 0 is the tens, bit 7 the decimal point
	auto digit_callback() { return m_digit_cb.bind(); }

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
	void update_display();
	void digit_w(offs_t offset, u64 data) { m_digit_cb(offset, data); }

	required_device<pic16c54_device> m_pic;
	required_device<pwm_display_device> m_display;
	required_ioport m_sensor;

	devcb_write8 m_digit_cb;

	u8 m_porta;
	u8 m_portb;
};

DECLARE_DEVICE_TYPE(GAELCO_ADAP_VC, gaelco_adap_vc_device)

#endif // MAME_GAELCO_GAELCO_ADAP_H
