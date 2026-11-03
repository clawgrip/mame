// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
#ifndef MAME_GAELCO_GAELCO_MS_MOD51_H
#define MAME_GAELCO_GAELCO_MS_MOD51_H

#pragma once


class gaelco_ms_mod51_device : public device_t
{
public:
	gaelco_ms_mod51_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);

	// pens kept out of the line buffer, for sprites decoded from inverted EPROM data
	u32 transmask() const { return m_transmask; }

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;

private:
	required_region_ptr<u8> m_prom;

	u32 m_transmask;
};


DECLARE_DEVICE_TYPE(GAELCO_MS_MOD51, gaelco_ms_mod51_device)

#endif // MAME_GAELCO_GAELCO_MS_MOD51_H
