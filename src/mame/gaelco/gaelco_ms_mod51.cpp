// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 51 sprite board

    Line buffer logic, a PAL (P0503), four EPROM sockets (or a MODULAR
    SYSTEM 2 MOD 5/1 ROM board on top) and an 82S129 PROM (IC10), 502 in
    every known set. The board inverts the EPROM pixels.

    The PROM decides which pixels are written into the line buffer: its
    outputs are all low for pen 15 (0 in the EPROMs) in all 16 rows.

    The sprite list has 4 bytes for each 16x16 sprite: code bits 0-7, Y,
    X bits 0-7 and attributes, and on most boards a colour byte for each
    sprite after the list: 0x200 bytes after a list of 128 sprites, or 0x400
    bytes after one of 256 on the Blood Bros., Power Spikes and Raiden
    boards. Cabal's list has 256 sprites and no colour bytes. Which
    attribute and colour bits give the other code bits, the
    flips, X bit 8 and the palette changes from game to game, also between
    games with the same board (flip X is bit 5 on Double Dragon and bit 4 on
    Gryzor), so it probably goes through each game's EPROM sub board. The
    drivers give it, deduced from the programs and the original games.

    Entry 0 isn't drawn: every program writes its X on purpose (at boot or
    every frame), and the line buffer starts from it. Fitted to the games,
    with no schematics: a sprite appears X - X0 - (pixels from the start of
    the horizontal blank to the first visible pixel) + 2 pixels right of the
    first visible pixel, and its top line is 241 - Y lines after the end of
    the vertical sync pulse, the lines the MOD 4 boards count from: 225 - Y
    lines below the first visible line with vertical PROM 202, 233 - Y with
    205. With horizontal PROMs 201, 206, 210 and 211 this matches the
    original games within a pixel horizontally and 2 lines vertically on
    Bad Lands, Black Tiger, Blood Bros., Cabal, Gryzor, Raiden, Secret
    Agent and Toki, and puts the sprites on the ground of Dragon Ninja and
    Fighting Fantasy. It doesn't fit the games with horizontal PROMs 204
    and 207 (Last Duel, Power Spikes and Twin Cobra), whose drivers set
    corrections. The lower entries are drawn over the higher ones.

    TODO:
    - what drives PROM address bits 4-7
    - which attribute and colour lines are fixed on the board
    - the position constants, measured on a board
    - priority against the tile layers, still done by the drivers that need it

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod51.h"


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD51, gaelco_ms_mod51_device, "gaelco_ms_mod51", "Gaelco Modular System MOD 51 sprite board")


gaelco_ms_mod51_device::gaelco_ms_mod51_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MOD51, tag, owner, clock),
	m_prom(*this, DEVICE_SELF),
	m_timing(*this, finder_base::DUMMY_TAG),
	m_list_r(*this, 0),
	m_sprite_cb(*this),
	m_transmask(0),
	m_entries(128),
	m_color_offset(0x200),
	m_x_bit8(true),
	m_correction_x(0),
	m_correction_y(0),
	m_x_delta(0),
	m_y_base(0),
	m_origin_x(0),
	m_origin_y(0)
{
}

void gaelco_ms_mod51_device::device_start()
{
	if (m_timing && !m_timing->started())
		throw device_missing_dependencies();

	// the pixel is on address bits 0-3
	m_transmask = 0;
	for (unsigned pen = 0; pen < 16; pen++)
	{
		if (!(m_prom[pen] & 0x0f))
			m_transmask |= 1 << pen;
	}

	m_sprite_cb.resolve();
	if (!m_sprite_cb.isnull())
	{
		if (!m_timing)
			fatalerror("%s: drawing the sprite list needs the MOD 21/1 timing\n", tag());

		m_x_delta = m_timing->hblank_to_visible() - 2;
		m_y_base = 241 - m_timing->vsync_to_visible();
		m_origin_x = m_timing->origin_x();
		m_origin_y = m_timing->origin_y();
	}
}


void gaelco_ms_mod51_device::entry(unsigned index, u32 &code, u32 &palette, bool &flipx, bool &flipy, u16 &x, u8 &y)
{
	offs_t const offset = index * 4;
	u8 const color = m_color_offset ? m_list_r(m_color_offset + offset) : 0;
	u8 const attr = m_list_r(offset + 3);
	bool bit8 = false;

	code = m_list_r(offset);
	y = m_list_r(offset + 1);
	palette = color & 0x0f;
	flipx = flipy = false;
	m_sprite_cb(attr, color, code, palette, flipx, flipy, bit8);
	x = m_list_r(offset + 2) | (u16(bit8) << 8);
}

void gaelco_ms_mod51_device::draw(bitmap_ind16 &bitmap, const rectangle &cliprect, gfx_element &gfx)
{
	u16 const xmask = m_x_bit8 ? 0x1ff : 0x0ff;
	u32 code, palette;
	bool flipx, flipy;
	u16 x;
	u8 y;

	// the line buffer address of the first visible pixel
	entry(0, code, palette, flipx, flipy, x, y);
	u16 const origin = (x + m_x_delta) & xmask;

	for (unsigned index = m_entries - 1; index > 0; index--)
	{
		entry(index, code, palette, flipx, flipy, x, y);
		int sx = (x - origin + m_correction_x) & xmask;
		int const sy = m_origin_y + m_y_base - y + m_correction_y;

		if (m_x_bit8)
		{
			// partly off the left edge
			if (sx >= 0x1f0)
				sx -= 0x200;
			gfx.transmask(bitmap, cliprect, code, palette, flipx, flipy, m_origin_x + sx, sy, m_transmask);
		}
		else
		{
			// X wraps around within 256 pixels
			for (int wrap = 0; wrap < 2; wrap++)
				gfx.transmask(bitmap, cliprect, code, palette, flipx, flipy, m_origin_x + sx - (wrap << 8), sy, m_transmask);
		}
	}
}
