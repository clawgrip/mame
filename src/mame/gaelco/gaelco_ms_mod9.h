// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD9_H
#define MAME_GAELCO_GAELCO_MS_MOD9_H

#pragma once

#include "gaelco_ms_timing.h"

#include "machine/gen_latch.h"
#include "sound/msm5205.h"
#include "sound/ymopl.h"
#include "sound/ymopn.h"

#include "screen.h"


class gaelco_ms_mod9_device_base : public device_t, public device_mixer_interface, public device_palette_interface,
		public device_gaelco_ms_timing_interface
{
public:
	// the screen, configured with the timing of the GAL
	template <typename T> void set_screen(T &&tag) { m_screen.set_tag(std::forward<T>(tag)); }

	// the CPU board interrupt, from the start of the vertical blank until the program acknowledges it
	auto irq_callback() { return m_irq_cb.bind(); }
	// 0-1: connector inputs of players 1 and 2, 2-3: DIP switches SW1 and SW2
	template <unsigned N> auto in_callback() { return m_in_cb[N].bind(); }

	// the registers the CPU boards see, selected by A1-A3; 16-bit boards see each byte on both halves of the data bus
	u8 io_r(offs_t offset);
	void io_w(offs_t offset, u8 data);
	u16 io16_r(offs_t offset);
	void io16_w(offs_t offset, u16 data, u16 mem_mask = ~0);

	// the palette RAM, 2048 entries; 8-bit boards see the high byte at the even address
	u16 palette_r(offs_t offset);
	void palette_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u8 palette8_r(offs_t offset);
	void palette8_w(offs_t offset, u8 data);

protected:
	gaelco_ms_mod9_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock);

	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_post_load() override ATTR_COLD;

	// device_palette_interface implementation
	virtual u32 palette_entries() const noexcept override { return 0x800; }

	optional_device<ym3812_device> m_opl;
	optional_device_array<ym2203_device, 2> m_opn;
	required_device<msm5205_device> m_msm;

private:
	void program_map(address_map &map) ATTR_COLD;
	void build_decoder() ATTR_COLD;

	u8 ym_r(u32 which, offs_t offset);
	void ym_w(u32 which, offs_t offset, u8 data);
	void control_w(u8 data);
	void adpcm_vck(int state);
	void screen_vblank(screen_device &screen, bool vblank_state);
	void update_pen(offs_t entry);

	required_device<cpu_device> m_cpu;
	required_device<generic_latch_8_device> m_soundlatch;
	required_device<screen_device> m_screen;
	required_region_ptr<u8> m_rom;
	required_region_ptr<u8> m_timing;
	required_region_ptr<u8> m_decoder;
	memory_share_creator<u8> m_ram;
	memory_view m_view;
	devcb_write_line m_irq_cb;
	devcb_read8::array<4> m_in_cb;

	u16 m_palette_ram[0x800];
	u8 m_control;
	u8 m_adpcm_data;
	bool m_adpcm_high;
};


class gaelco_ms_mod9_device : public gaelco_ms_mod9_device_base
{
public:
	gaelco_ms_mod9_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
};


class gaelco_ms_mod9_ym2203_device : public gaelco_ms_mod9_device_base
{
public:
	gaelco_ms_mod9_ym2203_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
};


INPUT_PORTS_EXTERN( gaelco_ms_mod9 );

DECLARE_DEVICE_TYPE(GAELCO_MS_MOD9, gaelco_ms_mod9_device)
DECLARE_DEVICE_TYPE(GAELCO_MS_MOD9_YM2203, gaelco_ms_mod9_ym2203_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD9_H
