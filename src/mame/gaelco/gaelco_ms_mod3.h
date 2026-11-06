// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD3_H
#define MAME_GAELCO_GAELCO_MS_MOD3_H

#pragma once


class gaelco_ms_mod3_decoder_device : public device_t
{
public:
	// what drives each PROM address line
	static constexpr u8 A(unsigned line) { return line; }
	static constexpr u8 NOT_A(unsigned line) { return 0x10 | line; }
	static constexpr u8 LATCH(unsigned bit) { return 0x20 | bit; }
	static constexpr u8 WRITE = 0x30;
	static constexpr u8 SELECT = 0x31; // which of the two CPUs of the 6809 sub board
	static constexpr u8 HIGH = 0x32;

	// installs the handlers for one area with the same PROM output
	using install_delegate = device_delegate<void (memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch)>;

	gaelco_ms_mod3_decoder_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// sources of PROM address lines 0-8
	void set_lines(const std::array<u8, 9> &lines) { m_lines = lines; }
	// bank latch bits that change the map, including any wired around the PROM
	void set_latch_mask(u8 mask) { m_latch_mask = mask; }
	void set_select(u8 select) { m_select = select; }
	template <typename T> void set_prom(T &&tag) { m_prom.set_tag(std::forward<T>(tag)); }
	template <typename... T> void set_install_callback(T &&... args) { m_install_cb.set(std::forward<T>(args)...); }

	void map(address_map &map) ATTR_COLD;
	void latch_w(u8 data);
	u8 latch() const { return m_latch; }

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_post_load() override;

private:
	u8 decode(offs_t address, bool write, u8 latch) const;
	unsigned views() const;
	unsigned view_index(u8 latch) const;
	u8 view_latch(unsigned index) const;

	required_region_ptr<u8> m_prom;
	memory_view m_view;
	install_delegate m_install_cb;

	std::array<u8, 9> m_lines;
	u8 m_latch_mask;
	u8 m_select;
	u8 m_latch;
};


DECLARE_DEVICE_TYPE(GAELCO_MS_MOD3_DECODER, gaelco_ms_mod3_decoder_device)


// undoes the data and address line swaps between the CPU and the EPROMs
void gaelco_ms_mod3_descramble(u8 *rom, offs_t length);

#endif // MAME_GAELCO_GAELCO_MS_MOD3_H
