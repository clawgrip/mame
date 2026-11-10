// license:BSD-3-Clause
// copyright-holders:

/*
    Double Dragon (Modular System)

    As with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver.

    For this game the Modular System cage contains 6 main boards and 3 sub-boards.
    More information, dip switches and PCB pictures: https://www.recreativas.org/modular-system-double-dragon-4350-gaelco-sa

    MOD 3/4   - CPU board: 3 ROMs, RAM, 22.1184 MHz XTAL, Small sub-board with 2 x MC68B09EP a PROM and a PAL.
    MOD 1/3   - Sound board: Motorola SC84014P, 2 x YM2203C, 2 x Y3014, one ROM, one PROM, two 8 dip switches Banks, and a small sub-board with an OKI M5205.
    MOD 4/3   - Tilemap board: logic + 4 tilemap ROMs, long thin sub-board (C0437) with no chips, just routing along one edge.
    MOD 4/3-A - Tilemap board: logic + 4 tilemap ROMs, long thin sub-board (C0438) with no chips, just routing along one edge.
    MOD 51    - Sprite board: logic, a PROM, and 8 ROMs on a small sub-board.
    MOD 21/1  - Logic, 20 MHz XTAL, two PROMs.

    The decoder PROM on the CPU sub board (P0320) maps both 6809s from A11-A15, three bank bits and
    which CPU makes the access. The first CPU gets the fixed EPROM area at 0x8000-0xffff, a window at
    0x4000-0x7fff with the six banks of the original game, the program's own code at 0x2000-0x27ff and
    RAM at 0x0000-0x0fff, shared with the second CPU, which runs from 0x8000-0xffff. The bank bits are
    kept by the PAL on the sub board, which the program writes at 0x8534.

    The other PROM outputs reach the boards in the cage:
    0x43  interrupt acknowledge (first CPU, 0x2800-0x2fff), where the original game acknowledged its FIRQ
    0x61  palette (first CPU, 0x1000-0x17ff)
    0x65  MOD 51 sprite board RAM (second CPU, 0x4000-0x47ff)
    0x6d  text layer MOD 4 board: RAM at 0x1800-0x1fff (first CPU), scroll at 0x2000 (second CPU)
    0x71  background MOD 4 board: RAM at 0x3000-0x37ff, scroll at 0x2800 (second CPU)
    0x7d  MOD 1 board (both CPUs, 0x3800-0x380f), with the I/O of the original game
    The second CPU builds the sprite list from the one of the original game and sets the scroll.

    TODO:
    - CPU clocks
    - interrupts: one vblank interrupt for both CPUs, acknowledged by the first one, fits the programs
    - flip screen: the second CPU can mirror the sprites, unknown for the MOD 4 boards
*/


#include "emu.h"

#include "../gaelco/gaelco_ms_mod1.h"
#include "../gaelco/gaelco_ms_mod21.h"
#include "../gaelco/gaelco_ms_mod3.h"
#include "../gaelco/gaelco_ms_mod3_pal.h"
#include "../gaelco/gaelco_ms_mod4.h"
#include "../gaelco/gaelco_ms_mod51.h"

#include "cpu/m6809/m6809.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"


namespace {

class ddragon_ms_state : public driver_device
{
public:
	ddragon_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_subcpu(*this, "subcpu"),
		m_mod1(*this, "mod1"),
		m_mod3(*this, "mod3"),
		m_mod3_sub(*this, "mod3_sub"),
		m_pal(*this, "mod3_pal"),
		m_mod4_fg(*this, "mod4_fg"),
		m_mod4_bg(*this, "mod4_bg"),
		m_mod51(*this, "mod51"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette"),
		m_gfxdecode(*this, "gfxdecode"),
		m_rom(*this, "maincpu"),
		m_ram(*this, "ram", 0x2000, ENDIANNESS_BIG),
		m_spriteram(*this, "spriteram", 0x800, ENDIANNESS_BIG),
		m_paletteram(*this, "palette", 0x400, ENDIANNESS_BIG),
		m_paletteram_ext(*this, "palette_ext", 0x400, ENDIANNESS_BIG)
	{ }

	void ddragonm(machine_config &config) ATTR_COLD;

	void init_ddragonm() ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_subcpu;
	required_device<gaelco_ms_mod1_6809_adpcm_device> m_mod1;
	required_device<gaelco_ms_mod3_decoder_device> m_mod3;
	required_device<gaelco_ms_mod3_decoder_device> m_mod3_sub;
	required_device<gaelco_ms_mod3_pal_device> m_pal;
	required_device<gaelco_ms_mod4_device> m_mod4_fg;
	required_device<gaelco_ms_mod4_device> m_mod4_bg;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;
	required_device<gfxdecode_device> m_gfxdecode;
	required_region_ptr<u8> m_rom;
	memory_share_creator<u8> m_ram;
	memory_share_creator<u8> m_spriteram;
	memory_share_creator<u8> m_paletteram;
	memory_share_creator<u8> m_paletteram_ext;

	u8 m_fg_scroll[4]{};
	u8 m_bg_scroll[4]{};

	void mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch);
	void install_mod4(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, gaelco_ms_mod4_device &board, u8 *scroll);
	void vblank_w(int state);
	void irq_ack_w(u8 data);

	TILEMAP_MAPPER_MEMBER(bg_scan);
	void fg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void bg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void main_map(address_map &map) ATTR_COLD;
	void sub_map(address_map &map) ATTR_COLD;
};


void ddragon_ms_state::machine_start()
{
	save_item(NAME(m_fg_scroll));
	save_item(NAME(m_bg_scroll));
}


void ddragon_ms_state::vblank_w(int state)
{
	// one interrupt for both CPUs, acknowledged by the first one
	if (state)
	{
		m_maincpu->set_input_line(M6809_IRQ_LINE, ASSERT_LINE);
		m_subcpu->set_input_line(M6809_IRQ_LINE, ASSERT_LINE);
	}
}

void ddragon_ms_state::irq_ack_w(u8 data)
{
	m_maincpu->set_input_line(M6809_IRQ_LINE, CLEAR_LINE);
	m_subcpu->set_input_line(M6809_IRQ_LINE, CLEAR_LINE);
}


TILEMAP_MAPPER_MEMBER(ddragon_ms_state::bg_scan)
{
	// four pages of 16 x 16 tiles, as in the original game
	return (col & 0x0f) | ((row & 0x0f) << 4) | ((col & 0x10) << 4) | ((row & 0x10) << 5);
}

void ddragon_ms_state::fg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// the original game's entries, the attribute byte first; the characters are in the upper half of the EPROMs
	code = 0x800 | (data >> 8) | ((data & 0x07) << 8);
	color = BIT(data, 5, 3);
}

void ddragon_ms_state::bg_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	code = (data >> 8) | ((data & 0x07) << 8);
	color = BIT(data, 3, 3);
	flags = TILE_FLIPYX(BIT(data, 6, 2));
}

void ddragon_ms_state::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// 4 bytes per sprite (code, y, x, attributes) and the colour 0x200 bytes later, built by the second
	// CPU from the sprite list of the original game: tall sprites split, flip X in attribute bit 5
	// and flip Y in bit 4, the colour in the upper half of the colour byte
	for (int offs = 0x200 - 4; offs >= 0; offs -= 4)
	{
		u8 const attr = m_spriteram[offs + 3];
		u32 const code = m_spriteram[offs] | ((attr & 0x0f) << 8);
		u32 const color = m_spriteram[offs + 0x200] >> 4;
		int const sx = (m_spriteram[offs + 2] | (BIT(attr, 7) << 8)) - 63;
		int const sy = 233 - m_spriteram[offs + 1];

		m_gfxdecode->gfx(0)->transmask(bitmap, cliprect, code, color, BIT(attr, 5), BIT(attr, 4), sx, sy, m_mod51->transmask());
	}
}

u32 ddragon_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the program writes the scroll of the original game minus 26 and 1 (27 for the text layer)
	m_mod4_bg->set_scrollx(26 + (m_bg_scroll[0] | (m_bg_scroll[1] << 8)));
	m_mod4_bg->set_scrolly(1 + (m_bg_scroll[2] | (m_bg_scroll[3] << 8)));
	m_mod4_fg->set_scrollx(27 + (m_fg_scroll[0] | (m_fg_scroll[1] << 8)));
	m_mod4_fg->set_scrolly(1 + (m_fg_scroll[2] | (m_fg_scroll[3] << 8)));

	m_mod4_bg->draw(screen, bitmap, cliprect, TILEMAP_DRAW_OPAQUE);
	draw_sprites(bitmap, cliprect);
	m_mod4_fg->draw(screen, bitmap, cliprect);

	return 0;
}


void ddragon_ms_state::install_mod4(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, gaelco_ms_mod4_device &board, u8 *scroll)
{
	// the RAM with A12 high, the scroll registers with A12 low
	for (offs_t area = start; area < end; area += 0x800)
	{
		if (BIT(area, 12))
		{
			if (write)
				view.install_write_handler(area, area + 0x7ff, write8sm_delegate(board, FUNC(gaelco_ms_mod4_device::vram8_w)));
			else
				view.install_read_handler(area, area + 0x7ff, read8sm_delegate(board, FUNC(gaelco_ms_mod4_device::vram8_r)));
		}
		else if (write)
		{
			view.install_write_handler(area, area + 0x003, 0, 0x7fc, 0, write8sm_delegate(*this, NAME([scroll] (offs_t offset, u8 data) { scroll[offset] = data; })));
		}
	}
}

void ddragon_ms_state::mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch)
{
	if (!BIT(code, 7))
	{
		// the boards in the cage
		switch (code)
		{
		case 0x43: // interrupt acknowledge, where the original game acknowledged its FIRQ
			if (write)
				view.install_write_handler(start, end, write8smo_delegate(*this, FUNC(ddragon_ms_state::irq_ack_w)));
			break;

		case 0x61: // palette, A10 selects the blue byte
			for (offs_t area = start; area < end; area += 0x400)
			{
				if (write)
				{
					view.install_write_handler(area, area + 0x3ff, BIT(area, 10) ?
							write8sm_delegate(*m_palette, FUNC(palette_device::write8_ext)) :
							write8sm_delegate(*m_palette, FUNC(palette_device::write8)));
				}
				else
				{
					view.install_rom(area, area + 0x3ff, BIT(area, 10) ? &m_paletteram_ext[0] : &m_paletteram[0]);
				}
			}
			break;

		case 0x65: // MOD 51 sprite board
			if (write)
				view.install_writeonly(start, end, &m_spriteram[0]);
			else
				view.install_rom(start, end, &m_spriteram[0]);
			break;

		case 0x6d: // text layer MOD 4 board
			install_mod4(view, start, end, write, *m_mod4_fg, m_fg_scroll);
			break;

		case 0x71: // background MOD 4 board
			install_mod4(view, start, end, write, *m_mod4_bg, m_bg_scroll);
			break;

		case 0x7d: // MOD 1 board, which keeps the I/O of the original game at 0x3800-0x380f
			for (offs_t area = start; area < end; area += 0x800)
			{
				if (write)
					view.install_write_handler(area, area + 0x00f, 0, 0x7f0, 0, write8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_6809_adpcm_device::host_w)));
				else
					view.install_read_handler(area, area + 0x00f, 0, 0x7f0, 0, read8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_6809_adpcm_device::host_r)));
			}
			break;
		}
		return;
	}

	// with output 0 low, writes clock the bank latch PAL
	if (write && !BIT(code, 0))
	{
		view.install_write_handler(start, end, write8smo_delegate(*m_pal, FUNC(gaelco_ms_mod3_pal_device::latch_w)));
		return;
	}

	// bits 5-6 pick the chip, IC12, IC13, IC14 or the RAM, and bits 2-4 drive A13-A15
	offs_t const offset = (BIT(code, 2, 3) << 13) | (start & 0x1fff);
	if (BIT(code, 5, 2) == 3)
	{
		if (write)
			view.install_writeonly(start, end, &m_ram[offset & 0x1fff]);
		else
			view.install_rom(start, end, &m_ram[offset & 0x1fff]);
	}
	else if (!write)
	{
		view.install_rom(start, end, &m_rom[(BIT(code, 5, 2) << 16) | offset]);
	}
}


void ddragon_ms_state::main_map(address_map &map)
{
	map(0x0000, 0xffff).m(m_mod3, FUNC(gaelco_ms_mod3_decoder_device::map));
}

void ddragon_ms_state::sub_map(address_map &map)
{
	map(0x0000, 0xffff).m(m_mod3_sub, FUNC(gaelco_ms_mod3_decoder_device::map));
}


void ddragon_ms_state::init_ddragonm()
{
	gaelco_ms_mod3_descramble(&m_rom[0], m_rom.bytes());
}


static INPUT_PORTS_START( ddragonm )
	PORT_INCLUDE( gaelco_ms_mod1 )

	PORT_START("SW1") // MOD 1 YM2203 #2 port B, the original game's DSW0 at 0x3803
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:4,5,6")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) ) PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A, the original game's DSW1 at 0x3804
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPUNUSED_DIPLOC( 0x08, IP_ACTIVE_LOW, "SW2:4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW2:5,6")
	PORT_DIPSETTING(    0x10, "20k" )
	PORT_DIPSETTING(    0x00, "40k" )
	PORT_DIPSETTING(    0x30, "30k and every 60k" )
	PORT_DIPSETTING(    0x20, "40k and every 80k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW2:7,8")
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x00, "Infinite (Cheat)")
INPUT_PORTS_END


static const gfx_layout sprite_layout =
{
	16, 16,
	RGN_FRAC(1, 1),
	4,
	{ 0, 8, 16, 24 },
	{ STEP8(0, 1), STEP8(16 * 32, 1) },
	{ STEP16(0, 32) },
	16 * 16 * 4
};

static GFXDECODE_START( gfx_ddragon_ms )
	GFXDECODE_ENTRY( "sprites", 0, sprite_layout, 0x200, 16 )
GFXDECODE_END


void ddragon_ms_state::ddragonm(machine_config &config)
{
	// basic machine hardware
	MC6809E(config, m_maincpu, 22.1184_MHz_XTAL / 12); // MC68B09EP, divisor unknown, the E clock of the Gryzor sub board
	m_maincpu->set_addrmap(AS_PROGRAM, &ddragon_ms_state::main_map);

	MC6809E(config, m_subcpu, 22.1184_MHz_XTAL / 12); // MC68B09EP, divisor unknown, the E clock of the Gryzor sub board
	m_subcpu->set_addrmap(AS_PROGRAM, &ddragon_ms_state::sub_map);

	// one PROM for both CPUs, the bank bits come from PAL pins 18-20
	using decoder = gaelco_ms_mod3_decoder_device;
	constexpr std::array<u8, 9> lines{ decoder::LATCH(3), decoder::A(11), decoder::A(12), decoder::A(13), decoder::A(14), decoder::A(15), decoder::LATCH(1), decoder::LATCH(2), decoder::SELECT };
	GAELCO_MS_MOD3_DECODER(config, m_mod3);
	m_mod3->set_lines(lines);
	m_mod3->set_latch_mask(0x0e);
	m_mod3->set_install_callback(FUNC(ddragon_ms_state::mod3_install));

	GAELCO_MS_MOD3_DECODER(config, m_mod3_sub);
	m_mod3_sub->set_lines(lines);
	m_mod3_sub->set_select(1);
	m_mod3_sub->set_prom("mod3");
	m_mod3_sub->set_install_callback(FUNC(ddragon_ms_state::mod3_install));

	GAELCO_MS_MOD3_PAL(config, m_pal);
	m_pal->outputs_callback().set(m_mod3, FUNC(gaelco_ms_mod3_decoder_device::latch_w));

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_screen_update(FUNC(ddragon_ms_state::screen_update));
	m_screen->set_palette(m_palette);
	m_screen->screen_vblank().set(FUNC(ddragon_ms_state::vblank_w));

	PALETTE(config, m_palette).set_format(palette_device::xBGR_444, 0x400); // on the MOD 21/1 board

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_ddragon_ms);

	gaelco_ms_mod21_device &mod21(GAELCO_MS_MOD21(config, "mod21", 20_MHz_XTAL));
	mod21.set_screen(m_screen);
	mod21.set_origin(0, 8);

	GAELCO_MS_MOD51(config, m_mod51);

	GAELCO_MS_MOD4(config, m_mod4_bg);
	m_mod4_bg->set_palette(m_palette);
	m_mod4_bg->set_color_base(0x100);
	m_mod4_bg->set_layout(FUNC(ddragon_ms_state::bg_scan), 32, 32);
	m_mod4_bg->set_tile_callback(FUNC(ddragon_ms_state::bg_tile_cb));

	GAELCO_MS_MOD4(config, m_mod4_fg);
	m_mod4_fg->set_palette(m_palette);
	m_mod4_fg->set_color_base(0x080);
	m_mod4_fg->set_tile_size(8);
	m_mod4_fg->set_layout(TILEMAP_SCAN_ROWS, 32, 32);
	m_mod4_fg->set_transparent_pen(15);
	m_mod4_fg->set_tile_callback(FUNC(ddragon_ms_state::fg_tile_cb));

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1_6809_ADPCM(config, m_mod1, 22.1184_MHz_XTAL / 5); // divisor unknown, no crystal on the board
	m_mod1->set_ym_clock(20_MHz_XTAL / 16); // divisor unknown, gives the pitch and about the tempo of the original game
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<2>().set_ioport("SW2");
	m_mod1->in_callback<3>().set_ioport("SW1");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 0.30);
}

ROM_START( ddragonm )
	ROM_REGION( 0x30000, "maincpu", 0 ) // on MOD 3/4 board
	ROM_LOAD( "mod_3-4_dr303_27c512.ic12", 0x00000, 0x10000, CRC(3aa464b0) SHA1(cee14d02eca3a8dd8d8fd29fadd77a44bf60b72f) )
	ROM_LOAD( "mod_3-4_dr302_27c512.ic13", 0x10000, 0x10000, CRC(67321538) SHA1(9dca2ea8bdcf6c0d3657d0d81c8aa02bb34ce4bd) )
	ROM_LOAD( "mod_3-4_dr301_27c512.ic14", 0x20000, 0x10000, CRC(de8f6f06) SHA1(68de0f757b4a8652ed13ad9802b022a36339d6e1) ) // 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x10000, "mod1", 0 ) // on MOD 1/3 board
	ROM_LOAD( "mod_1-3_dra101_27c512.ic12", 0x00000, 0x10000, CRC(e75de5a1) SHA1(e59a67b05d8dc723edfd88c52fe75301d345c38c) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "mod_1-3_p0111_82s123.ic20", 0x00, 0x20, CRC(b3baf409) SHA1(74e378bdb1f265a7067d59ff9514b50263231fa2) )

	ROM_REGION( 0x20000, "mod4_fg", ROMREGION_INVERT ) // on MOD 4/3 board
	ROM_LOAD32_BYTE( "mod_4-3_dr401_27256.ic17", 0x00003, 0x08000, CRC(86dc72eb) SHA1(dc23e884a03e21353ec25e24bc7073dd756408f6) ) //  01xxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "mod_4-3_dr402_27256.ic16", 0x00002, 0x08000, CRC(88abf9a3) SHA1(8ca97e2c0ecb8cf438a718bb197785d0c848613e) ) //  01xxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "mod_4-3_dr403_27256.ic15", 0x00001, 0x08000, CRC(b1d94294) SHA1(42e85f02eb2c1cf4b78e13be1c082ca4057aee57) ) //  01xxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "mod_4-3_dr404_27256.ic14", 0x00000, 0x08000, CRC(667682b2) SHA1(09108673d0f04a9249be04c6fa41a2cacff61e72) ) //  1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x40000, "mod4_bg", ROMREGION_INVERT ) // on MOD 4/3-A board
	ROM_LOAD32_BYTE( "mod_4-3_a_dr4a01_27512.ic17", 0x00003, 0x10000, CRC(5f6a6d6f) SHA1(7d546a226cda81c28e7ccfb4c5daebc65072198d) )
	ROM_LOAD32_BYTE( "mod_4-3_a_dr4a02_27512.ic16", 0x00002, 0x10000, CRC(4171b70d) SHA1(dc300c9bca6481417e97ad03c973e47389f261c1) )
	ROM_LOAD32_BYTE( "mod_4-3_a_dr4a03_27512.ic15", 0x00001, 0x10000, CRC(a670d088) SHA1(27e7b49645753dd039f104c3e0a7e6513a98710d) )
	ROM_LOAD32_BYTE( "mod_4-3_a_dr4a04_27512.ic14", 0x00000, 0x10000, CRC(736eff0f) SHA1(ae2ec2d5c8ab1db579a08256d874426dc5d889c6) )

	ROM_REGION( 0x80000, "sprites", ROMREGION_INVERT ) // on MOD 51 board
	ROM_LOAD32_BYTE( "mod_51_sub_dr501_27512.bin",  0x00003, 0x10000, CRC(bfa4da27) SHA1(68a649aec43e18dc79b4690c1dff2e2a6fc0065a) )
	ROM_LOAD32_BYTE( "mod_51_sub_dr502_27512.bin",  0x00002, 0x10000, CRC(a73bc8f5) SHA1(286cd04dfc8b6bfc26faf8ec246ffe9e7bfe87a4) )
	ROM_LOAD32_BYTE( "mod_51_sub_dr503_27512.bin",  0x00001, 0x10000, CRC(4b10defd) SHA1(fb43eba7c8a7f77f0fdd6253d51b40b0e64598f5) )
	ROM_LOAD32_BYTE( "mod_51_sub_dr504_27512.bin",  0x00000, 0x10000, CRC(7300b785) SHA1(6d3b72bd7208e2bd790517a753c9d5192c88d20f) )
	ROM_LOAD32_BYTE( "mod_51_sub_dr5a01_27512.bin", 0x40003, 0x10000, CRC(f80e81a6) SHA1(e74768d47985f59b9eb45013127f26ea8e0ddc28) )
	ROM_LOAD32_BYTE( "mod_51_sub_dr5a02_27512.bin", 0x40002, 0x10000, CRC(15b86326) SHA1(5da97dc3ac500a4ee38ee625a14e9efc1ca6bb5b) )
	ROM_LOAD32_BYTE( "mod_51_sub_dr5a03_27512.bin", 0x40001, 0x10000, CRC(6248bcc4) SHA1(27d160ad489ea32f4bda652f7f8f5e32257f34a1) )
	ROM_LOAD32_BYTE( "mod_51_sub_dr5a04-27512.bin", 0x40000, 0x10000, CRC(a4baa5ba) SHA1(25a7434a0a3ea33faa0881e6a372b1ee50b19cdd) )

	ROM_REGION( 0x200, "mod21", 0 ) // on MOD 21/1 board
	ROM_LOAD( "mod_21-1_p0201_82s129.ic4",    0x000, 0x100, CRC(2697da58) SHA1(e62516b886ff6e204b718e5f0c6ce2712e4b7fc5) )
	ROM_LOAD( "mod_21-1_p0205_82s129.ic12",   0x100, 0x100, CRC(204a7aee) SHA1(322164134aa65c37a9389024f921364a81d13e88) )

	ROM_REGION( 0x100, "mod51", 0 ) // on MOD 51 board
	ROM_LOAD( "mod_51_p0502_82s129.ic10",     0x000, 0x100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )

	ROM_REGION( 0x200, "mod3", 0 ) // on the CPU sub board, address decoder
	ROM_LOAD( "mod_3-4_sub_p0320_82s147.bin", 0x000, 0x200, CRC(9bc0d387) SHA1(bac95901cacf883fcde0662b444c443364227def) )

	ROM_REGION( 0xcc, "mod3_pal", 0 ) // on the CPU sub board, bank latch
	ROM_LOAD( "mod_3-4_sub_p36_pal20x4.bin",   0x000, 0x0cc, CRC(896e285d) SHA1(5a49890c396fb9f3625d421ff4a8823d3b76ccb9) )

	ROM_REGION( 0x200, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "mod_51_p0503-pal16r6a.ic46",    0x000, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
	ROM_LOAD( "mod_4-3_403_gal16v8.ic29",      0x000, 0x117, CRC(c136de93) SHA1(116f6d3b456d20621ab07a005c1421f57569915c) )
	ROM_LOAD( "mod_4-3_a_p0403_pal16r8a.ic29", 0x000, 0x104, CRC(16379b0d) SHA1(5379560b0ec7c67cbe131a581a347b86395f34ac) )
ROM_END

} // anonymous namespace


GAME( 1987, ddragonm, ddragon, ddragonm, ddragonm, ddragon_ms_state, init_ddragonm, ROT0, "bootleg (Gaelco / Ervisa)", "Double Dragon (Modular System)", MACHINE_NOT_WORKING )
