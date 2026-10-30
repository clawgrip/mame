// license:BSD-3-Clause
// copyright-holders:Ernesto Corvi
#ifndef MAME_TECMO_WC90B_H
#define MAME_TECMO_WC90B_H

#pragma once

#include "sound/msm5205.h"

#include "emupal.h"
#include "screen.h"
#include "tilemap.h"


class wc90b_state : public driver_device
{
public:
	wc90b_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_subcpu(*this, "sub"),
		m_audiocpu(*this, "audiocpu"),
		m_msm(*this, "msm"),
		m_palette(*this, "palette"),
		m_gfxdecode(*this, "gfxdecode"),
		m_fgvideoram(*this, "fgvideoram"),
		m_bgvideoram(*this, "bgvideoram"),
		m_txvideoram(*this, "txvideoram"),
		m_spriteram(*this, "spriteram"),
		m_mainbank(*this, "mainbank"),
		m_subbank(*this, "subbank"),
		m_audiobank(*this, "audiobank"),
		m_scrollx(*this, "scrollx%u", 1U),
		m_scrolly(*this, "scrolly%u", 1U),
		m_scroll_x_lo(*this, "scroll_x_lo")
	{ }

	void wc90b(machine_config &config) ATTR_COLD;

	void init_wc90b() ATTR_COLD;

protected:
	DECLARE_GFXDECODE_MEMBER(gfx_wc90b);

	virtual void machine_start() override ATTR_COLD;
	virtual void video_start() override ATTR_COLD;

	tilemap_t *m_tx_tilemap;
	tilemap_t *m_fg_tilemap;
	tilemap_t *m_bg_tilemap;

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_subcpu;
	required_device<cpu_device> m_audiocpu;
	required_device<msm5205_device> m_msm;
	required_device<palette_device> m_palette;
	required_device<gfxdecode_device> m_gfxdecode;
	required_shared_ptr<uint8_t> m_fgvideoram;
	required_shared_ptr<uint8_t> m_bgvideoram;
	required_shared_ptr<uint8_t> m_txvideoram;
	required_shared_ptr<uint8_t> m_spriteram;
	required_memory_bank m_mainbank;
	required_memory_bank m_subbank;
	required_memory_bank m_audiobank;

	void bgvideoram_w(offs_t offset, uint8_t data);
	void fgvideoram_w(offs_t offset, uint8_t data);
	void txvideoram_w(offs_t offset, uint8_t data);
	void bankswitch_w(uint8_t data);
	void adpcm_int(int state);

	void sound_map(address_map &map) ATTR_COLD;
	void sub_map(address_map &map) ATTR_COLD;

	virtual uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	virtual void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect, int priority);

private:
	optional_shared_ptr_array<uint8_t, 2> m_scrollx;
	optional_shared_ptr_array<uint8_t, 2> m_scrolly;
	optional_shared_ptr<uint8_t> m_scroll_x_lo;

	void main_map(address_map &map) ATTR_COLD;

	uint8_t m_msm5205next;
	uint8_t m_toggle;

	void sub_bankswitch_w(uint8_t data);
	void adpcm_data_w(uint8_t data);
	void adpcm_control_w(uint8_t data);
	uint8_t master_irq_ack_r();
	void slave_irq_ack_w(uint8_t data);

	TILE_GET_INFO_MEMBER(get_bg_tile_info);
	TILE_GET_INFO_MEMBER(get_fg_tile_info);
	TILE_GET_INFO_MEMBER(get_tx_tile_info);
};


INPUT_PORTS_EXTERN( wc90b );

#endif // MAME_TECMO_WC90B_H
