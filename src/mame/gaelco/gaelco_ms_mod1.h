// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD1_H
#define MAME_GAELCO_GAELCO_MS_MOD1_H

#pragma once

#include "sound/msm5205.h"
#include "sound/ymopn.h"


class gaelco_ms_mod1_device_base : public device_t, public device_mixer_interface
{
public:
	template <typename T> void set_ym_clock(T &&clock)
	{
		m_ym[0].lookup()->set_clock(clock);
		m_ym[1].lookup()->set_clock(clock);
	}
	void set_int_period(const attotime &period) { m_int_period = period; }
	void set_host_nmi(bool enable) { m_host_nmi = enable; }

	// 0-1: YM2203 #1 ports A and B (connector inputs), 2-3: YM2203 #2 ports A and B (DIP switches SW2 and SW1)
	template <unsigned N> auto in_callback() { return m_in_cb[N].bind(); }

	void reset_w(int state) { m_cpu->set_input_line(INPUT_LINE_RESET, state ? ASSERT_LINE : CLEAR_LINE); }

	// the top 16 bytes of the RAM, shared with the CPU board (16-bit boards see even bytes on D8-D15)
	u8 host_r(offs_t offset);
	void host_w(offs_t offset, u8 data);
	u16 host16_r(offs_t offset);
	void host16_w(offs_t offset, u16 data, u16 mem_mask = ~0);

protected:
	gaelco_ms_mod1_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock, int int_line, u8 int_ack, u8 nmi_ack);

	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_post_load() override ATTR_COLD;

	void add_adpcm(machine_config &config) ATTR_COLD;
	void program_map(address_map &map) ATTR_COLD;

	required_device<cpu_device> m_cpu;
	optional_device<msm5205_device> m_msm;

private:
	u8 ym_r(offs_t offset);
	void ym_w(offs_t offset, u8 data);
	void latch_w(u8 data);
	void set_bank(u8 bank);
	void int_w(int state);
	void int_ack();
	void nmi_ack();

	TIMER_CALLBACK_MEMBER(int_tick);

	required_device_array<ym2203_device, 2> m_ym;
	required_region_ptr<u8> m_rom;
	required_region_ptr<u8> m_prom;
	memory_share_creator<u8> m_ram;
	memory_view m_view;
	devcb_read8::array<4> m_in_cb;

	int const m_int_line;
	u8 const m_int_ack;
	u8 const m_nmi_ack;

	emu_timer *m_int_timer;

	attotime m_int_period;
	bool m_host_nmi;
	u8 m_bank;
};


class gaelco_ms_mod1_device : public gaelco_ms_mod1_device_base
{
public:
	gaelco_ms_mod1_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	gaelco_ms_mod1_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock);

	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
};


class gaelco_ms_mod1_adpcm_device : public gaelco_ms_mod1_device
{
public:
	gaelco_ms_mod1_adpcm_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	void set_adpcm_prescaler(int select) { m_msm.lookup()->set_prescaler_selector(select); }

protected:
	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
};


class gaelco_ms_mod1_6809_device : public gaelco_ms_mod1_device_base
{
public:
	gaelco_ms_mod1_6809_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	gaelco_ms_mod1_6809_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock);

	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
};


class gaelco_ms_mod1_6809_adpcm_device : public gaelco_ms_mod1_6809_device
{
public:
	gaelco_ms_mod1_6809_adpcm_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	void set_adpcm_prescaler(int select) { m_msm.lookup()->set_prescaler_selector(select); }

protected:
	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
};


INPUT_PORTS_EXTERN( gaelco_ms_mod1 );

DECLARE_DEVICE_TYPE(GAELCO_MS_MOD1, gaelco_ms_mod1_device)
DECLARE_DEVICE_TYPE(GAELCO_MS_MOD1_ADPCM, gaelco_ms_mod1_adpcm_device)
DECLARE_DEVICE_TYPE(GAELCO_MS_MOD1_6809, gaelco_ms_mod1_6809_device)
DECLARE_DEVICE_TYPE(GAELCO_MS_MOD1_6809_ADPCM, gaelco_ms_mod1_6809_adpcm_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD1_H
