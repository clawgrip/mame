// license:BSD-3-Clause
// copyright-holders:

/*
    Gryzor (Modular System)

    As with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver.

    For this game the Modular System cage contains 7 main boards and 2 sub-boards.
    More information, dip switches and PCB pictures: https://www.recreativas.org/modular-system-gryzor-4344-gaelco-sa

    MOD 3/2   - CPU board: 2 ROMs, RAM, 22.1184 MHz XTAL, Small sub-board with 2 x Motorola SC84014P a PROM and a PAL.
    MOD 1/3   - Sound board: Motorola SC84014P, 2 x YM2203C, 2 x Y3014, one ROM, one PROM, two 8 dip switches Banks.
    MOD 4/3   - Tilemap board: logic + 2 tilemap ROMs, long thin sub-board (C0431A) with no chips, just routing along one edge.
    MOD 4/3-A - Tilemap board: logic + 4 tilemap ROMs, long thin sub-board (C0435A) with no chips, just routing along one edge.
    MOD 4/3-B - Tilemap board: logic + 4 tilemap ROMs, long thin sub-board (C0436A) with no chips, just routing along one edge.
    MOD 51    - Sprite board: logic, a PROM, and 8 ROMs on a small sub-board.
    MOD 21/1  - Logic, 24 MHz XTAL, two PROMs.

    The program is the one of gryzor, its EPROMs wired like the other MOD 3 boards. The decoder PROM on
    the CPU sub board (P0318) maps both SC84014Ps from A12-A15, four bank bits and which CPU makes the
    access. The first CPU gets the fixed EPROM area at 0x8000-0xffff, the 8 KB banks of the original
    game at 0x6000-0x7fff (0-7 from IC13, 8-11 from IC14) and RAM at 0x0000-0x1fff, shared with the
    second CPU, which runs its own program from 0xf000-0xffff. The PAL on the sub board keeps three
    bank bits, written at 0xf800.

    Both CPUs see 16 KB of RAM at 0x2000-0x5fff, where the first one keeps the video RAM of the
    original game. Each MOD 4 board holds 4 KB of it, two pages of 1024 attribute and 1024 code bytes:
    0x2000  MOD 4/3-A  shows its first page, the first layer of the original game
    0x4000  MOD 4/3-B  shows its first page, the second layer
    0x5000  MOD 4/3    shows its second page in columns: the five columns of text of the original game,
                       which the second CPU copies from the second page of the MOD 4/3-A board
    The tile bank bits in the attributes are the ones the original game selects in the registers of its
    two 007121 video chips, plain RAM here. The MOD 4/3-A and MOD 4/3-B EPROMs hold tiles 0-0x1fff of
    each chip and the MOD 51 EPROMs the sprites of both, tiles 0x2000-0x3fff of the first and
    0x1800-0x37ff of the second, with the tiles of the first that the game also draws as sprites
    (0x0d00-0x0dff) in place of 256 unused ones.

    The second CPU does the rest of the work of those chips and the I/O of the original game. It copies
    the inputs and DIP switches from the MOD 1 board (0x9000) to where the program reads its I/O ports
    and passes it the sound commands, converts the palette (0x7000) through the colour lookup PROMs of
    the original game, which it has in its EPROM, builds the MOD 51 sprite list (0x8000) from the object
    tables of the game and writes the scroll in bytes of the video RAM that the game leaves alone: 0x2808
    and 0x280e for the first layer, 0x4ffc and 0x4ffe for the second and 0x3000-0x3003 for the text. At
    the end of each interrupt the two CPUs wait for each other, and the second one acknowledges the
    interrupt by reading 0x6000.

    TODO:
    - where the fourth bank bit comes from: the program writes each bank number twice to 0xf800,
      in bits 0-3 and then in bits 1-4, and the PAL only holds three bits
    - interrupts: one vblank interrupt for both CPUs, acknowledged by the second one, fits the programs
    - which board holds the RAM at 0x3000-0x3fff, and how the MOD 4 boards take their scroll from the
      bytes above
*/


#include "emu.h"

#include "../gaelco/gaelco_ms_mod1.h"
#include "../gaelco/gaelco_ms_mod21.h"
#include "../gaelco/gaelco_ms_mod3.h"
#include "../gaelco/gaelco_ms_mod3_pal.h"
#include "../gaelco/gaelco_ms_mod4.h"
#include "../gaelco/gaelco_ms_mod51.h"
#include "konamipt.h"

#include "cpu/m6809/m6809.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"


namespace {

class gryzor_ms_state : public driver_device
{
public:
	gryzor_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_subcpu(*this, "subcpu"),
		m_mod1(*this, "mod1"),
		m_mod3(*this, "mod3"),
		m_mod3_sub(*this, "mod3_sub"),
		m_pal(*this, "mod3_pal"),
		m_mod4(*this, { "mod4_tx", "mod4_a", "mod4_b" }),
		m_mod51(*this, "mod51"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette"),
		m_gfxdecode(*this, "gfxdecode"),
		m_rom(*this, "maincpu"),
		m_ram(*this, "ram", 0x2000, ENDIANNESS_BIG),
		m_ram2(*this, "ram2", 0x1000, ENDIANNESS_BIG),
		m_spriteram(*this, "spriteram", 0x1000, ENDIANNESS_BIG),
		m_paletteram(*this, "palette", 0x400, ENDIANNESS_BIG),
		m_paletteram_ext(*this, "palette_ext", 0x400, ENDIANNESS_BIG)
	{ }

	void gryzorm(machine_config &config) ATTR_COLD;

	void init_gryzorm() ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	// the MOD 4/3, MOD 4/3-A and MOD 4/3-B boards
	enum : u8 { MOD4_TX, MOD4_A, MOD4_B };

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_subcpu;
	required_device<gaelco_ms_mod1_6809_device> m_mod1;
	required_device<gaelco_ms_mod3_decoder_device> m_mod3;
	required_device<gaelco_ms_mod3_decoder_device> m_mod3_sub;
	required_device<gaelco_ms_mod3_pal_device> m_pal;
	required_device_array<gaelco_ms_mod4_device, 3> m_mod4;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;
	required_device<gfxdecode_device> m_gfxdecode;
	required_region_ptr<u8> m_rom;
	memory_share_creator<u8> m_ram;
	memory_share_creator<u8> m_ram2;
	memory_share_creator<u8> m_spriteram;
	memory_share_creator<u8> m_paletteram;
	memory_share_creator<u8> m_paletteram_ext;

	u8 m_bank3 = 0;

	void mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch) ATTR_COLD;
	template <unsigned Which> void install_mod4(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write) ATTR_COLD;
	void bank_w(u8 data);
	void vblank_w(int state);
	u8 irq_ack_r();
	template <unsigned Which> u8 mod4_r(offs_t offset);
	template <unsigned Which> void mod4_w(offs_t offset, u8 data);

	void tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8);
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void main_map(address_map &map) ATTR_COLD;
	void sub_map(address_map &map) ATTR_COLD;
};


template <unsigned Which>
u8 gryzor_ms_state::mod4_r(offs_t offset)
{
	// two pages of 1024 attribute and 1024 code bytes, as in the video chips of the original game
	offs_t const entry = (offset & 0x3ff) | (BIT(offset, 11) << 10);
	return BIT(offset, 10) ? m_mod4[Which]->code_r(entry) : m_mod4[Which]->attr_r(entry);
}

template <unsigned Which>
void gryzor_ms_state::mod4_w(offs_t offset, u8 data)
{
	offs_t const entry = (offset & 0x3ff) | (BIT(offset, 11) << 10);
	if (BIT(offset, 10))
		m_mod4[Which]->code_w(entry, data);
	else
		m_mod4[Which]->attr_w(entry, data);
}


void gryzor_ms_state::tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// the tile bank bits the original game selects in its video chip registers
	code = (data & 0x00ff) | (BIT(data, 15) << 8) | (BIT(data, 11, 4) << 9);
	color = BIT(data, 8, 3);
}

void gryzor_ms_state::tx_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// the tiles are in the upper half of the EPROMs, and with EPROMs for the two lower bitplanes only they take
	// the last four pens of their colour
	code = 0x800 | (data & 0x00ff);
	color = (color << 2) | 3;
}

void gryzor_ms_state::sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8)
{
	// built by the second CPU from the objects of the game: X bit 8 in attribute bit 0, code bits 8-9 in bits
	// 1-2, 10 in bit 6 and 11 in bit 7, flip X in bit 4 and flip Y in bit 5
	code |= (BIT(attr, 1, 2) << 8) | (BIT(attr, 6, 2) << 10);
	flipx = BIT(attr, 4);
	flipy = BIT(attr, 5);
	x8 = BIT(attr, 0);
}

u32 gryzor_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the second CPU writes the X scroll of the original game plus 0xa8 and its Y scroll, and 0x00ff for the
	// text, which puts the five columns at the left, one pixel off the original game
	m_mod4[MOD4_A]->set_scrollx(mod4_r<MOD4_A>(0x808));
	m_mod4[MOD4_A]->set_scrolly(mod4_r<MOD4_A>(0x80e));
	m_mod4[MOD4_B]->set_scrollx(mod4_r<MOD4_B>(0xffc));
	m_mod4[MOD4_B]->set_scrolly(mod4_r<MOD4_B>(0xffe));
	m_mod4[MOD4_TX]->set_scrollx(m_ram2[0] | (m_ram2[1] << 8));
	m_mod4[MOD4_TX]->set_scrolly(m_ram2[2] | (m_ram2[3] << 8));

	m_mod4[MOD4_B]->draw(screen, bitmap, cliprect, TILEMAP_DRAW_OPAQUE);
	m_mod4[MOD4_A]->draw(screen, bitmap, cliprect);
	m_mod51->draw(bitmap, cliprect, *m_gfxdecode->gfx(0));
	m_mod4[MOD4_TX]->draw(screen, bitmap, cliprect);

	return 0;
}


void gryzor_ms_state::machine_start()
{
	save_item(NAME(m_bank3));
}


void gryzor_ms_state::bank_w(u8 data)
{
	// the PAL doesn't hold the fourth bank bit, the program has it in bit 4 of its second write
	m_bank3 = BIT(data, 4);
	m_pal->latch_w(data);
	m_mod3->latch_w(m_pal->outputs() | (m_bank3 << 4));
}


void gryzor_ms_state::vblank_w(int state)
{
	// one interrupt for both CPUs, acknowledged by the second one
	if (state)
	{
		m_maincpu->set_input_line(M6809_IRQ_LINE, ASSERT_LINE);
		m_subcpu->set_input_line(M6809_IRQ_LINE, ASSERT_LINE);
	}
}

u8 gryzor_ms_state::irq_ack_r()
{
	if (!machine().side_effects_disabled())
	{
		m_maincpu->set_input_line(M6809_IRQ_LINE, CLEAR_LINE);
		m_subcpu->set_input_line(M6809_IRQ_LINE, CLEAR_LINE);
	}
	return 0xff;
}


template <unsigned Which>
void gryzor_ms_state::install_mod4(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write)
{
	if (write)
		view.install_write_handler(start, end, write8sm_delegate(*this, FUNC(gryzor_ms_state::mod4_w<Which>)));
	else
		view.install_read_handler(start, end, read8sm_delegate(*this, FUNC(gryzor_ms_state::mod4_r<Which>)));
}

void gryzor_ms_state::mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch)
{
	if (!BIT(code, 7))
	{
		// the boards in the cage
		switch (code)
		{
		case 0x5f: // interrupt acknowledge
			if (!write)
				view.install_read_handler(start, end, read8smo_delegate(*this, FUNC(gryzor_ms_state::irq_ack_r)));
			break;

		case 0x61: // palette, A10 selects the blue byte
			for (offs_t area = start; area < end; area += 0x800)
			{
				for (unsigned half = 0; half < 2; half++)
				{
					offs_t const first = area + (half << 10);
					if (write)
					{
						view.install_write_handler(first, first + 0x3ff, half ?
								write8sm_delegate(*m_palette, FUNC(palette_device::write8_ext)) :
								write8sm_delegate(*m_palette, FUNC(palette_device::write8)));
					}
					else
					{
						view.install_rom(first, first + 0x3ff, half ? &m_paletteram_ext[0] : &m_paletteram[0]);
					}
				}
			}
			break;

		case 0x65: // MOD 51 sprite board
			if (write)
				view.install_writeonly(start, end, &m_spriteram[0]);
			else
				view.install_rom(start, end, &m_spriteram[0]);
			break;

		case 0x6d: // MOD 4/3 board
			install_mod4<MOD4_TX>(view, start, end, write);
			break;

		case 0x71: // MOD 4/3-A board
			install_mod4<MOD4_A>(view, start, end, write);
			break;

		case 0x75: // MOD 4/3-B board
			install_mod4<MOD4_B>(view, start, end, write);
			break;

		case 0x79: // RAM, sprite RAM of the first video chip and variables in the original game
			if (write)
				view.install_writeonly(start, end, &m_ram2[0]);
			else
				view.install_rom(start, end, &m_ram2[0]);
			break;

		case 0x7d: // MOD 1 board
			if (write)
				view.install_write_handler(start, start + 0x00f, 0, (end - start) & ~0xf, 0, write8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_device_base::host_w)));
			else
				view.install_read_handler(start, start + 0x00f, 0, (end - start) & ~0xf, 0, read8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_device_base::host_r)));
			break;
		}
		return;
	}

	// with output 0 low, writes clock the bank latch
	if (write && !BIT(code, 0))
	{
		view.install_write_handler(start, end, write8smo_delegate(*this, FUNC(gryzor_ms_state::bank_w)));
		return;
	}

	// bits 5-6 pick the chip, IC13, IC14 or the RAM, and bits 2-4 drive A13-A15
	offs_t const offset = (BIT(code, 2, 3) << 13) | (start & 0x1fff);
	switch (BIT(code, 5, 2))
	{
	case 1: // IC13, gz302
		if (!write)
			view.install_rom(start, end, &m_rom[0x10000 | offset]);
		break;
	case 2: // IC14, gz301
		if (!write)
			view.install_rom(start, end, &m_rom[0x00000 | offset]);
		break;
	case 3:
		if (write)
			view.install_writeonly(start, end, &m_ram[offset & 0x1fff]);
		else
			view.install_rom(start, end, &m_ram[offset & 0x1fff]);
		break;
	}
}


void gryzor_ms_state::main_map(address_map &map)
{
	map(0x0000, 0xffff).m(m_mod3, FUNC(gaelco_ms_mod3_decoder_device::map));
}

void gryzor_ms_state::sub_map(address_map &map)
{
	map(0x0000, 0xffff).m(m_mod3_sub, FUNC(gaelco_ms_mod3_decoder_device::map));
}


void gryzor_ms_state::init_gryzorm()
{
	gaelco_ms_mod3_descramble(&m_rom[0], m_rom.bytes());
}


static INPUT_PORTS_START( gryzorm )
	// the MOD 1 board merges both joysticks into the first player's port of the original game, and the second
	// CPU fixes its third DIP switch bank at 0x0f: one set of controls, used by both players in turn
	PORT_INCLUDE( gaelco_ms_mod1 )

	PORT_START("SW1") // MOD 1 YM2203 #2 port B, DSW1 of the original game
	KONAMI_COINAGE_LOC(DEF_STR(Free_Play), "No Coin B", SW1)

	PORT_START("SW2") // MOD 1 YM2203 #2 port A, DSW2 of the original game
	PORT_DIPNAME( 0x03, 0x02, DEF_STR(Lives) )         PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPUNUSED_DIPLOC( 0x04, 0x04, "SW2:3" ) // cabinet type in the original game, the second CPU clears it
	PORT_DIPNAME( 0x18, 0x18, DEF_STR(Bonus_Life) )    PORT_DIPLOCATION("SW2:4,5")
	PORT_DIPSETTING(    0x18, "30000 70000" )
	PORT_DIPSETTING(    0x10, "40000 80000" )
	PORT_DIPSETTING(    0x08, "40000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR(Difficulty) )    PORT_DIPLOCATION("SW2:6,7")
	PORT_DIPSETTING(    0x60, DEF_STR(Easy) )
	PORT_DIPSETTING(    0x40, DEF_STR(Normal) )
	PORT_DIPSETTING(    0x20, DEF_STR(Hard) )
	PORT_DIPSETTING(    0x00, DEF_STR(Hardest) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR(Demo_Sounds) )   PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
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

static GFXDECODE_START( gfx_gryzor_ms )
	GFXDECODE_ENTRY( "sprites", 0, sprite_layout, 0x100, 16 )
GFXDECODE_END


void gryzor_ms_state::gryzorm(machine_config &config)
{
	// basic machine hardware
	// Motorola SC84014P, the part Double Dragon's sub board has as MC68B09EP
	MC6809E(config, m_maincpu, 22.1184_MHz_XTAL / 12); // E clock, divisor unknown
	m_maincpu->set_addrmap(AS_PROGRAM, &gryzor_ms_state::main_map);

	MC6809E(config, m_subcpu, 22.1184_MHz_XTAL / 12); // E clock, divisor unknown
	m_subcpu->set_addrmap(AS_PROGRAM, &gryzor_ms_state::sub_map);

	// the two CPUs of the sub board share its RAM, and the second one polls flags the first one sets only briefly
	config.set_perfect_quantum(m_maincpu);

	// one PROM for both CPUs, bank bits 0-2 come from PAL pins 18-20 and the input that takes A11 on
	// ddragonm is the fourth bank bit here
	using decoder = gaelco_ms_mod3_decoder_device;
	constexpr std::array<u8, 9> lines{ decoder::latch_bit(3), decoder::latch_bit(4), decoder::addr(12), decoder::addr(13), decoder::addr(14), decoder::addr(15), decoder::latch_bit(1), decoder::latch_bit(2), decoder::SELECT };
	GAELCO_MS_MOD3_DECODER(config, m_mod3);
	m_mod3->set_lines(lines);
	m_mod3->set_latch_mask(0x1e);
	m_mod3->set_latch_reset(false);
	m_mod3->set_install_callback(FUNC(gryzor_ms_state::mod3_install));

	GAELCO_MS_MOD3_DECODER(config, m_mod3_sub);
	m_mod3_sub->set_lines(lines);
	m_mod3_sub->set_select(1);
	m_mod3_sub->set_prom("mod3");
	m_mod3_sub->set_install_callback(FUNC(gryzor_ms_state::mod3_install));

	GAELCO_MS_MOD3_PAL(config, m_pal);

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_raw(24_MHz_XTAL / 4, 384, 0, 280, 266, 16, 240); // MOD 21/1 PROMs 206 and 202
	m_screen->set_screen_update(FUNC(gryzor_ms_state::screen_update));
	m_screen->screen_vblank().set(FUNC(gryzor_ms_state::vblank_w));
	m_screen->set_palette(m_palette);

	PALETTE(config, m_palette).set_format(palette_device::xBGR_444, 0x400); // on the MOD 21/1 board

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_gryzor_ms);

	gaelco_ms_mod21_device &mod21(GAELCO_MS_MOD21(config, "mod21", 24_MHz_XTAL));
	mod21.set_screen(m_screen);

	GAELCO_MS_MOD51(config, m_mod51);
	m_mod51->set_timing(mod21);
	m_mod51->list_r().set([this] (offs_t offset) { return m_spriteram[offset]; });
	m_mod51->set_sprite_callback(FUNC(gryzor_ms_state::sprite_cb));

	GAELCO_MS_MOD4(config, m_mod4[MOD4_B]);
	m_mod4[MOD4_B]->set_timing(mod21);
	m_mod4[MOD4_B]->set_palette(m_palette);
	m_mod4[MOD4_B]->set_color_base(0x300);
	m_mod4[MOD4_B]->set_tile_size(8);
	m_mod4[MOD4_B]->set_layout(TILEMAP_SCAN_ROWS, 32, 32);
	m_mod4[MOD4_B]->set_tile_callback(FUNC(gryzor_ms_state::tile_cb));

	GAELCO_MS_MOD4(config, m_mod4[MOD4_A]);
	m_mod4[MOD4_A]->set_timing(mod21);
	m_mod4[MOD4_A]->set_palette(m_palette);
	m_mod4[MOD4_A]->set_color_base(0x200);
	m_mod4[MOD4_A]->set_tile_size(8);
	m_mod4[MOD4_A]->set_layout(TILEMAP_SCAN_ROWS, 32, 32);
	m_mod4[MOD4_A]->set_transparent_pen(15);
	m_mod4[MOD4_A]->set_tile_callback(FUNC(gryzor_ms_state::tile_cb));

	GAELCO_MS_MOD4(config, m_mod4[MOD4_TX]);
	m_mod4[MOD4_TX]->set_timing(mod21);
	m_mod4[MOD4_TX]->set_palette(m_palette);
	m_mod4[MOD4_TX]->set_tile_size(8);
	m_mod4[MOD4_TX]->set_planes(2);
	m_mod4[MOD4_TX]->set_layout(TILEMAP_SCAN_COLS, 64, 32);
	m_mod4[MOD4_TX]->set_transparent_pen(3);
	m_mod4[MOD4_TX]->set_tile_callback(FUNC(gryzor_ms_state::tx_tile_cb));

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1_6809(config, m_mod1, 4'423'680); // unknown, no crystal on the board
	m_mod1->set_ym_clock(1'322'000); // unknown, gives the pitch of the original game
	m_mod1->set_int_period(attotime::from_hz(1000)); // unknown source, about 1 kHz gives the tempo of the original game
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<2>().set_ioport("SW2");
	m_mod1->in_callback<3>().set_ioport("SW1");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 0.30);
}

ROM_START( gryzorm )
	ROM_REGION( 0x20000, "maincpu", 0 ) // on MOD 3 board
	ROM_LOAD( "mod_3-2_gz301_27512.ic14", 0x00000, 0x10000, CRC(b25cfc4a) SHA1(c1334b45a38fba8d304a4f96e241dd46222037df) )
	ROM_LOAD( "mod_3-2_gz302_27512.ic13", 0x10000, 0x10000, CRC(207e5fd0) SHA1(c2846a1d8eb3ad22b841807ececdb511ac1d4d1a) )

	ROM_REGION( 0x8000, "mod1", 0 ) // on MOD 1/3 board
	ROM_LOAD( "mod_1-3_gz0101_27256.ic12", 0x0000, 0x8000, CRC(b6106777) SHA1(891a622924f93c58ef2aca4be2cc5d7f3dee815b) )

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "mod_1-3_p0109_mmi6331.ic20", 0x00, 0x20, CRC(0c58dcfc) SHA1(a89170754745659d3fad6d25b9f54a4a48d4f26f) )

	ROM_REGION( 0x10000, "mod4_tx", ROMREGION_INVERT ) // on MOD 4/3 board
	ROM_LOAD16_BYTE( "mod_4-3_gz401_27256.ic18", 0x00001, 0x08000, CRC(7dedabc6) SHA1(ce4c6b17844dc54012ce981a3a518e32f87606f5) ) //  0111xxxxxxxxxxx = 0xFF
	ROM_LOAD16_BYTE( "mod_4-3_gz402_27256.ic17", 0x00000, 0x08000, CRC(02131f92) SHA1(51a77da46ed5b6bcdca94ad0a96f9ba2d43dfdde) ) //  0111xxxxxxxxxxx = 0xFF

	ROM_REGION( 0x40000, "mod4_a", ROMREGION_INVERT ) // on MOD 4/3-A board
	ROM_LOAD32_BYTE( "mod_4-3_a_gz4a01_27512.ic17", 0x00003, 0x10000, CRC(bfd1780a) SHA1(059276c9b9b138d43645dd59415467a7b6bdd2eb) )
	ROM_LOAD32_BYTE( "mod_4-3_a_gz4a02_27512.ic16", 0x00002, 0x10000, CRC(7c115140) SHA1(5f26fab8ca0e8df6fe79824b4985302b1e0dfd08) )
	ROM_LOAD32_BYTE( "mod_4-3_a_gz4a03_27512.ic15", 0x00001, 0x10000, CRC(de42af04) SHA1(1d79162889e965166ac34d462c20d6db15f36db8) )
	ROM_LOAD32_BYTE( "mod_4-3_a_gz4a04_27512.ic14", 0x00000, 0x10000, CRC(2a536bd5) SHA1(b523a7da90d69eefa0c4b528f89dc1fbc4e5252a) )

	ROM_REGION( 0x40000, "mod4_b", ROMREGION_INVERT ) // on MOD 4/3-B board
	ROM_LOAD32_BYTE( "mod_4-3_b_gz4b01_27512.ic17", 0x00003, 0x10000, CRC(51e247d1) SHA1(2176a39625ab21b88e25d2f3f5e03d7093041b52) )
	ROM_LOAD32_BYTE( "mod_4-3_b_gz4b02_27512.ic16", 0x00002, 0x10000, CRC(f1fba529) SHA1(8f6baaf8360bdb6c805e379b0dbb2079baa51364) )
	ROM_LOAD32_BYTE( "mod_4-3_b_gz4b03_27512.ic15", 0x00001, 0x10000, CRC(4aa2b6b9) SHA1(547653b220d5981160d1b31a68f0001a0d406c03) )
	ROM_LOAD32_BYTE( "mod_4-3_b_gz4b04_27512.ic14", 0x00000, 0x10000, CRC(8f77c11b) SHA1(32ba5870576ce419a4991091607de99d4f98583a) )

	ROM_REGION( 0x80000, "sprites", ROMREGION_INVERT ) // on MOD 51 board
	ROM_LOAD32_BYTE( "mod_51_sub_gz501_27512.bin",  0x00003, 0x10000, CRC(a42793fd) SHA1(89856a72301c9fa5e59b8b3304df953cf358cefb) )
	ROM_LOAD32_BYTE( "mod_51_sub_gz502_27512.bin",  0x00002, 0x10000, CRC(6ccfb733) SHA1(657161e725cd48cbc807c75ecd0644de8f7d0270) )
	ROM_LOAD32_BYTE( "mod_51_sub_gz503_27512.bin",  0x00001, 0x10000, CRC(0a17cc17) SHA1(9bed123b6754d9b8f7cfba2176536f3e453d94f0) )
	ROM_LOAD32_BYTE( "mod_51_sub_gz504_27512.bin",  0x00000, 0x10000, CRC(7afc287e) SHA1(bc2168194845ff2fde35df450bc8fa755b29df3f) )
	ROM_LOAD32_BYTE( "mod_51_sub_gz5a01_27512.bin", 0x40003, 0x10000, CRC(54cf2e96) SHA1(28ad483fdf645bf5e19e1e71ed1da9d32dfe95ca) )
	ROM_LOAD32_BYTE( "mod_51_sub_gz5a02_27512.bin", 0x40002, 0x10000, CRC(4c32e827) SHA1(7670803d46de7333df021b232759ea346f748e37) )
	ROM_LOAD32_BYTE( "mod_51_sub_gz5a03_27512.bin", 0x40001, 0x10000, CRC(abd7b5b7) SHA1(94326d7ff8bba1299a5be193d06c1e019d38ce5c) )
	ROM_LOAD32_BYTE( "mod_51_sub_gz5a04_27512.bin", 0x40000, 0x10000, CRC(e0b11c30) SHA1(a35c94a2dd48f10eec623356655f25bd9e533eaf) )

	ROM_REGION( 0x200, "mod21", 0 ) // on MOD 21/1 board
	ROM_LOAD( "mod_21-1_p0206_82s129.ic4",       0x000, 0x100, CRC(a47680d5) SHA1(6475cad53b91f6ef1043e7330b79f58695fd7ec9) )
	ROM_LOAD( "mod_21-1_p0202_82s129.ic12",      0x100, 0x100, CRC(e434128a) SHA1(ef0f6d8daef8b25211095577a182cdf120a272c1) )

	ROM_REGION( 0x100, "mod51", 0 ) // on MOD 51 board
	ROM_LOAD( "mod_51_p0502_mmi63s141.ic10",     0x000, 0x100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )

	ROM_REGION( 0x200, "mod3", 0 ) // on the CPU sub board, address decoder
	ROM_LOAD( "mod_3-4_sub_p0318_mmi63s481.bin", 0x000, 0x200, CRC(67a4bbc7) SHA1(441be3a810225813bf2667ddf9b2fdc7de35196e) )

	ROM_REGION( 0xcc, "mod3_pal", 0 ) // on the CPU sub board, bank latch
	ROM_LOAD( "mod_3-4_sub_p36_pal20x4.bin",  0x000, 0x0cc, CRC(896e285d) SHA1(5a49890c396fb9f3625d421ff4a8823d3b76ccb9) )

	ROM_REGION( 0x200, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "mod_4-3_p0403_pal16r8.ic29",   0x000, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) ) // Same on all MOD 4/3 PCBs
	ROM_LOAD( "mod_4-3_a_p0403_pal16r8.ic29", 0x000, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) ) // Same on all MOD 4/3 PCBs
	ROM_LOAD( "mod_4-3_b_p0403_pal16r8.ic29", 0x000, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) ) // Same on all MOD 4/3 PCBs
	ROM_LOAD( "mod_51_p0503_pal16r6a.ic46",   0x000, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
ROM_END

} // anonymous namespace


GAME( 1987, gryzorm, contra, gryzorm, gryzorm, gryzor_ms_state, init_gryzorm, ROT90, "bootleg (Gaelco / Ervisa)", "Gryzor (Modular System)", MACHINE_NOT_WORKING )
