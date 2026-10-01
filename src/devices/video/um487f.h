// license:BSD-3-Clause
// copyright-holders:ClawGrip
/***************************************************************************

    UMC UM487F HCGA (Hercules + CGA) single-chip video controller

****************************************************************************

    Pinout (100-pin QFP):

      1  CLOCK       26  SQ0         51  N.C.        76  D1
      2  N.C.        27  SQ1         52  VOUT        77  D2
      3  RESET       28  SQ2         53  HOUT        78  D3
      4  CRA0        29  SQ3         54  CD0         79  N.C.
      5  CRA1        30  N.C.        55  CD1         80  N.C.
      6  CRA2        31  SQ4         56  CD2         81  D4
      7  CRA3        32  SQ5         57  CD3         82  D5
      8  A0          33  SQ6         58  CD4         83  D6
      9  A1          34  SQ7         59  CD5         84  D7
     10  A2          35  WEB         60  CD6         85  GND
     11  A3          36  GND         61  CD7         86  IOWB
     12  A4          37  MEMWB       62  N.C.        87  82C11CSB
     13  A5          38  IORDY       63  GND         88  SWS
     14  A6          39  GND         64  MOSC        89  SWR
     15  A7          40  VCC         65  OSC         90  JMPO
     16  VCC         41  CGLAT       66  VCC         91  GND
     17  GND         42  RASB        67  MD0         92  VCC
     18  A8          43  CASB        68  MD1         93  GND
     19  A9          44  GND         69  MD2         94  PSW
     20  A10         45  IOUT        70  MD3         95  DATAGATEB
     21  A11         46  BOUT        71  MD4         96  MEMSELB
     22  A12         47  VIDOT       72  MD5         97  DIR
     23  A13         48  ROUT        73  MD6         98  AEN
     24  A14         49  GOUT        74  MD7         99  IORB
     25  A15         50  N.C.        75  D0         100  MEMRB

***************************************************************************/

#ifndef MAME_VIDEO_UM487F_H
#define MAME_VIDEO_UM487F_H

#pragma once

#include "video/mc6845.h"


class um487f_device : public device_t,
					  public device_video_interface,
					  public device_palette_interface
{
public:
	// construction/destruction
	// clock is the CGA base clock fed to the OSC pin (14.31818 MHz)
	um487f_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// configuration
	// MGA base clock fed to the MOSC pin (16.257 MHz), 0 if the pin is not connected
	void set_mga_clock(uint32_t clock) { m_mga_clock = clock; }
	void set_mga_clock(const XTAL &clock) { m_mga_clock = clock.value(); }
	// state of the SWS/SWR mode straps at power on (true = MGA mode)
	void set_mga_mode(bool mga) { m_strap_mga = mga; }
	// SW3: allow switching between CGA and MGA modes by software
	void set_mode_change_enable(bool enable) { m_strap_change_enable = enable; }
	// external character generator ROM (8 KiB, 2764 style) fed through CD0-CD7
	template <typename T> void set_chargen(T &&tag) { m_chargen.set_tag(std::forward<T>(tag)); }

	auto hsync_callback() { return m_hsync_cb.bind(); }
	auto vsync_callback() { return m_vsync_cb.bind(); }

	// CPU interface
	// I/O space 0x3b0-0x3df (offset 0x00-0x2f), A0-A9 are decoded and AEN must be active
	uint8_t io_r(offs_t offset);
	void io_w(offs_t offset, uint8_t data);
	// video memory window 0xb0000-0xbffff (A0-A15, while MEMSELB is active)
	uint8_t mem_r(offs_t offset);
	void mem_w(offs_t offset, uint8_t data);

	uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

protected:
	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_post_load() override;

	// device_palette_interface implementation
	virtual uint32_t palette_entries() const noexcept override { return 16 + 3; }

private:
	enum : uint8_t
	{
		// mode control register (0x3d8 / 0x3b8)
		MODE_HIRES       = 0x01, // CGA: 80x25 text (high clock)
		MODE_GRAPHICS    = 0x02,
		MODE_BW          = 0x04, // CGA: black/white (alternate palette)
		MODE_VIDEO       = 0x08, // video enable
		MODE_640         = 0x10, // CGA: 640x200 graphics
		MODE_BLINK       = 0x20, // blink enable
		MODE_CHANGE      = 0x40, // enable mode change
		MODE_PAGE1       = 0x80, // MGA: display page 1

		// configuration register (0x3bf)
		CONFIG_MGA_GFX   = 0x01, // enable MGA graphics
		CONFIG_MGA_PAGE1 = 0x02, // enable MGA page 1
		CONFIG_CGA       = 0x40, // select CGA (1) or MGA (0) mode

		// status register (0x3da / 0x3ba)
		STATUS_CGA_NODISP = 0x01, // CGA: non display period
		STATUS_MGA_HSYNC  = 0x01, // MGA: horizontal sync period
		STATUS_LPEN_TRIG  = 0x02, // light pen latched
		STATUS_LPEN_SW    = 0x04, // light pen switch on
		STATUS_CGA_VSYNC  = 0x08, // CGA: vertical sync period
		STATUS_MGA_VIDEO  = 0x08, // MGA: video dot
		STATUS_NOVSYNC    = 0x80  // vertical sync period when clear
	};

	enum : uint8_t
	{
		PEN_MGA_BLACK = 16,
		PEN_MGA_NORMAL,
		PEN_MGA_BRIGHT
	};

	MC6845_UPDATE_ROW(crtc_update_row);
	MC6845_RECONFIGURE(crtc_reconfigure);
	void crtc_hsync_w(int state);
	void crtc_vsync_w(int state);

	void cga_text_row(bitmap_rgb32 &bitmap, uint16_t ma, uint8_t ra, uint16_t y, uint8_t x_count, int8_t cursor_x);
	void cga_gfx_2bpp_row(bitmap_rgb32 &bitmap, uint16_t ma, uint8_t ra, uint16_t y, uint8_t x_count);
	void cga_gfx_1bpp_row(bitmap_rgb32 &bitmap, uint16_t ma, uint8_t ra, uint16_t y, uint8_t x_count);
	void mga_text_row(bitmap_rgb32 &bitmap, uint16_t ma, uint8_t ra, uint16_t y, uint8_t x_count, int8_t cursor_x);
	void mga_gfx_row(bitmap_rgb32 &bitmap, uint16_t ma, uint8_t ra, uint16_t y, uint8_t x_count);

	uint8_t chargen_r(uint8_t chr, uint8_t ra) const;
	uint8_t status_r();
	void mode_w(uint8_t data);
	void color_w(uint8_t data);
	void config_w(uint8_t data);
	void update_crtc_clock();
	void update_palette_lut();
	bool mga_graphics() const { return m_mga && (m_mode & MODE_GRAPHICS) && (m_config & CONFIG_MGA_GFX); }
	bool mga_page1() const { return m_mga && (m_mode & MODE_PAGE1) && (m_config & CONFIG_MGA_PAGE1); }

	required_device<mc6845_device> m_crtc;
	optional_region_ptr<uint8_t> m_chargen;

	devcb_write_line m_hsync_cb;
	devcb_write_line m_vsync_cb;

	// configuration
	uint32_t m_mga_clock;
	bool m_strap_mga;
	bool m_strap_change_enable;

	// internal state
	std::unique_ptr<uint8_t[]> m_vram;
	bool m_mga;             // current display mode (false = CGA, true = MGA)
	uint8_t m_mode;         // mode control register
	uint8_t m_color;        // color select register
	uint8_t m_config;       // configuration register
	bool m_lpen_latched;    // light pen latch
	uint8_t m_video_dot;    // fake MGA video dot stream
	uint8_t m_framecnt;     // vertical sync counter for cursor and character blinking
	int m_vsync_on_pos;     // raster line where the vertical sync pulse begins
	uint8_t m_palette_lut_2bpp[4];
};

DECLARE_DEVICE_TYPE(UM487F, um487f_device)

#endif // MAME_VIDEO_UM487F_H
