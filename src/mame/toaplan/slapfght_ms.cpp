// license:BSD-3-Clause
// copyright-holders:

/*
    Slap Fight (Modular System)

    As with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver.

    For this game the Modular System cage contains 6 main boards and 1 sub board.

    MOD. 3 - Z80 board (3 ROMs + RAM + 24MHz XTAL) + small sub board with Z0840006PSC + PROM + logic (decryption?).
    MOD 21/1 - 20 MHz XTAL + logic.
    MOD 1 - Sound board (Z08400BPS, 2 x YM2203C). 2 8-dips banks.
    MOD 51/1 - Sprite board, has logic + 4 ROMs.
    MOD 4/2 - Tilemap board, has logic + 2 ROMs.
    MOD 4/2 - Tilemap board, has logic + 4 ROMs, long thin sub-board (C0425) with no chips, just routing along one edge.

    The decoder PROM on the CPU sub board (P0312) maps the CPU address space in 256 byte areas from
    A8-A15 and bit 0 of the bank latch at 0xe900. With the bit set, the 16 KB EPROM replaces the
    second half of the other one at 0x8000-0xbfff and the text layer area at 0xf000-0xffff becomes
    EPROM, the MOD 51 sprite list (0xf400-0xf5ff), RAM, the palette (0xf800-0xfaff and 0xfc00-0xffff)
    and the interrupt acknowledge (0xfb00-0xfbff).

    The program keeps the sprite list and the scroll registers of the original game in the RAM at
    0xe000-0xe8ff. Its interrupt routine converts the list into the MOD 51 format every other frame,
    and its NMI routine copies the converted list to the MOD 51 board and the X scroll to 0xe820,
    which the playfield board takes with the Y scroll at 0xe822. The text layer board takes its
    scroll from 0xe820-0xe823 with the bank bit set.

    TODO:
    - the NMI source is unknown, once per frame with the vblank interrupt fits the program;
    - sound IRQs once testable;
    - derive from slapfght_state (slapfght.cpp) once all the common points and differences are verified.
*/


#include "emu.h"

#include "../gaelco/gaelco_ms_mod1.h"
#include "../gaelco/gaelco_ms_mod21.h"
#include "../gaelco/gaelco_ms_mod3.h"
#include "../gaelco/gaelco_ms_mod4.h"
#include "../gaelco/gaelco_ms_mod51.h"

#include "cpu/z80/z80.h"
#include "machine/74259.h"

#include "screen.h"
#include "speaker.h"

#include "multibyte.h"

#include <algorithm>


namespace {

class slapfght_ms_state : public driver_device
{
public:
	slapfght_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mod1(*this, "mod1"),
		m_mod3(*this, "mod3"),
		m_mod4_pf(*this, "mod4_pf"),
		m_mod4_fix(*this, "mod4_fix"),
		m_mod21(*this, "mod21"),
		m_mod51(*this, "mod51"),
		m_screen(*this, "screen"),
		m_gfxdecode(*this, "gfxdecode"),
		m_rom(*this, "maincpu"),
		m_ram(*this, "ram", 0x2000, ENDIANNESS_LITTLE),
		m_ram2(*this, "ram2", 0x900, ENDIANNESS_LITTLE),
		m_spriteram(*this, "spriteram", 0x200, ENDIANNESS_LITTLE)
	{ }

	void slapfighm(machine_config &config);

	void init_decryption();

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<gaelco_ms_mod1_device> m_mod1;
	required_device<gaelco_ms_mod3_decoder_device> m_mod3;
	required_device<gaelco_ms_mod4_device> m_mod4_pf;
	required_device<gaelco_ms_mod4_device> m_mod4_fix;
	required_device<gaelco_ms_mod21_device> m_mod21;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<screen_device> m_screen;
	required_device<gfxdecode_device> m_gfxdecode;

	required_region_ptr<u8> m_rom;
	memory_share_creator<u8> m_ram;
	memory_share_creator<u8> m_ram2;
	memory_share_creator<u8> m_spriteram;

	u8 m_fix_scroll[4] = { };

	void vblank_irq(int state);
	u8 irq_ack_r();
	void irq_ack_w(u8 data);
	void sound_reset_w(int state);

	void mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch) ATTR_COLD;
	void fix_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void main_io_map(address_map &map) ATTR_COLD;
	void main_prg_map(address_map &map) ATTR_COLD;
};


void slapfght_ms_state::fix_tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	code = data & 0x03ff;
	color = data >> 10;
}

void slapfght_ms_state::sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8)
{
	// the attribute byte of the original list: code bits 8-9 in bits 6-7 and the palette in bits 1-4; X bit 8
	// in bit 0 isn't wired, the program drops the sprites outside a 256 pixel window instead
	code |= BIT(attr, 6, 2) << 8;
	palette = BIT(attr, 1, 4);
}

uint32_t slapfght_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_mod4_pf->set_scrollx(get_u16le(&m_ram2[0x820]));
	m_mod4_pf->set_scrolly(m_ram2[0x822]);
	m_mod4_fix->set_scrollx(get_u16le(m_fix_scroll));
	m_mod4_fix->set_scrolly(m_fix_scroll[2]);

	m_mod4_pf->draw(screen, bitmap, cliprect);
	m_mod51->draw(bitmap, cliprect, *m_gfxdecode->gfx(0));
	m_mod4_fix->draw(screen, bitmap, cliprect);

	return 0;
}


void slapfght_ms_state::vblank_irq(int state)
{
	if (state)
	{
		m_maincpu->set_input_line(0, ASSERT_LINE);
		m_maincpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);
	}
}

u8 slapfght_ms_state::irq_ack_r()
{
	if (!machine().side_effects_disabled())
		m_maincpu->set_input_line(0, CLEAR_LINE);

	return 0xff;
}

void slapfght_ms_state::irq_ack_w(u8 data)
{
	m_maincpu->set_input_line(0, CLEAR_LINE);
}

void slapfght_ms_state::sound_reset_w(int state)
{
	m_mod1->reset_w(!state);
}

void slapfght_ms_state::machine_start()
{
	save_item(NAME(m_fix_scroll));
}


void slapfght_ms_state::mod3_install(memory_view::memory_view_entry &view, offs_t start, offs_t end, bool write, u8 code, u8 latch)
{
	if (BIT(code, 7))
	{
		u8 const socket = BIT(code, 4, 3);
		if (socket == 6)
		{
			// 8 KB RAM
			if (write)
				view.install_writeonly(start, end, &m_ram[start & 0x1fff]);
			else
				view.install_rom(start, end, &m_ram[start & 0x1fff]);
		}
		else if (!write)
		{
			// EPROMs: 303, 302 and the 16 KB 301
			switch (socket)
			{
			case 0: view.install_rom(start, end, &m_rom[0x00000 | (start & 0x7fff)]); break;
			case 2: view.install_rom(start, end, &m_rom[0x08000 | (start & 0x7fff)]); break;
			case 4: view.install_rom(start, end, &m_rom[0x10000 | (start & 0x3fff)]); break;
			}
		}
		else if ((socket == 4) && BIT(start, 14))
		{
			view.install_write_handler(start, end, write8smo_delegate(*m_mod3, FUNC(gaelco_ms_mod3_decoder_device::latch_w)));
		}
		return;
	}

	// the boards tell their areas apart with the lower address lines
	switch (code)
	{
	case 0x7d: // MOD 1 board
		for (offs_t page = start; page < end; page += 0x100)
		{
			if (write)
				view.install_write_handler(page, page + 0x0f, 0, 0xf0, 0, write8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_device_base::host_w)));
			else
				view.install_read_handler(page, page + 0x0f, 0, 0xf0, 0, read8sm_delegate(*m_mod1, FUNC(gaelco_ms_mod1_device_base::host_r)));
		}
		break;

	case 0x75: // MOD 4 boards for the playfield and the text layer, A11 selects the attributes
	case 0x6d:
		for (offs_t area = start; area < end; area += 0x800)
		{
			gaelco_ms_mod4_device &board = (code == 0x75) ? *m_mod4_pf : *m_mod4_fix;
			offs_t const last = std::min<offs_t>(end, area | 0x7ff);
			if (write)
			{
				view.install_write_handler(area, last, BIT(area, 11) ?
						write8sm_delegate(board, FUNC(gaelco_ms_mod4_device::attr_w)) :
						write8sm_delegate(board, FUNC(gaelco_ms_mod4_device::code_w)));
			}
			else
			{
				view.install_read_handler(area, last, BIT(area, 11) ?
						read8sm_delegate(board, FUNC(gaelco_ms_mod4_device::attr_r)) :
						read8sm_delegate(board, FUNC(gaelco_ms_mod4_device::code_r)));
			}
		}
		break;

	case 0x79: // RAM, sprite list and scroll registers in the original game
		if (write)
			view.install_writeonly(start, end, &m_ram2[start - 0xe000]);
		else
			view.install_rom(start, end, &m_ram2[start - 0xe000]);
		break;

	case 0x71: // text layer scroll registers
		if (write)
			view.install_write_handler(start, end, write8sm_delegate(*this, NAME([this] (offs_t offset, u8 data) { m_fix_scroll[offset & 3] = data; })));
		break;

	case 0x65: // MOD 51 sprite list
		if (write)
			view.install_writeonly(start, end, &m_spriteram[start & 0x1ff]);
		else
			view.install_rom(start, end, &m_spriteram[start & 0x1ff]);
		break;

	case 0x43: // interrupt acknowledge, the interrupt routine reads 0xfb00
		if (write)
			view.install_write_handler(start, end, write8smo_delegate(*this, FUNC(slapfght_ms_state::irq_ack_w)));
		else
			view.install_read_handler(start, end, read8smo_delegate(*this, FUNC(slapfght_ms_state::irq_ack_r)));
		break;

	case 0x61: // palette, A10 selects the blue byte
		for (offs_t area = start; area < end; area = (area | 0x3ff) + 1)
		{
			offs_t const last = std::min<offs_t>(end, area | 0x3ff);
			if (write)
			{
				view.install_write_handler(area, last, BIT(area, 10) ?
						write8sm_delegate(*m_mod21, FUNC(gaelco_ms_mod21_device::blue_w)) :
						write8sm_delegate(*m_mod21, FUNC(gaelco_ms_mod21_device::green_red_w)));
			}
			else
			{
				view.install_read_handler(area, last, BIT(area, 10) ?
						read8sm_delegate(*m_mod21, FUNC(gaelco_ms_mod21_device::blue_r)) :
						read8sm_delegate(*m_mod21, FUNC(gaelco_ms_mod21_device::green_red_r)));
			}
		}
		break;
	}
}


void slapfght_ms_state::main_prg_map(address_map &map)
{
	map(0x0000, 0xffff).m(m_mod3, FUNC(gaelco_ms_mod3_decoder_device::map));
}

void slapfght_ms_state::main_io_map(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0x0f).w("mainlatch", FUNC(ls259_device::write_a0));
}

static INPUT_PORTS_START( slapfighm )
	PORT_INCLUDE( gaelco_ms_mod1 )

	// the sound program does the coinage and passes SW2 at 0xc809 and SW1 at 0xc80a, inverted, where the original
	// game's sound CPU leaves its first and second DIP switch banks; the program's DIP switch display numbers the
	// switches as on the original board and shows coin B as unused
	PORT_START("SW1") // MOD 1 YM2203 #2 port B
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW1:8,7")
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW1:6,5")
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x08, "50000 200000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW1:4,3")
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_SERVICE_DIPLOC( 0x40, IP_ACTIVE_LOW, "SW1:2" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:1" )

	PORT_START("SW2") // MOD 1 YM2203 #2 port A
	PORT_DIPUNUSED_DIPLOC( 0x01, 0x01, "SW2:8" ) // cabinet type in the original game, the sound program sets upright
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Screen Test" ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW2:4,3")
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW2:2,1")
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
INPUT_PORTS_END


static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7, 512,513,514,515,516,517,518,519 },
	{ STEP16(0,32) },
	16 * 16 * 4
};

static GFXDECODE_START( gfx_slapfighm)
	GFXDECODE_ENTRY( "sprites", 0, spritelayout, 0x200, 16 )
GFXDECODE_END


void slapfght_ms_state::slapfighm(machine_config &config)
{
	// basic machine hardware
	Z80(config, m_maincpu, 24_MHz_XTAL / 4); // divisor unknown
	m_maincpu->set_addrmap(AS_PROGRAM, &slapfght_ms_state::main_prg_map);
	m_maincpu->set_addrmap(AS_IO, &slapfght_ms_state::main_io_map);

	using decoder = gaelco_ms_mod3_decoder_device;
	GAELCO_MS_MOD3_DECODER(config, m_mod3);
	m_mod3->set_lines({ decoder::addr(8), decoder::addr(9), decoder::addr(10), decoder::addr(11), decoder::addr(12), decoder::addr(13), decoder::addr(14), decoder::addr(15), decoder::latch_bit(0) });
	m_mod3->set_latch_mask(0x01);
	m_mod3->set_install_callback(FUNC(slapfght_ms_state::mod3_install));

	ls259_device &mainlatch(LS259(config, "mainlatch"));
	mainlatch.q_out_cb<0>().set(FUNC(slapfght_ms_state::sound_reset_w));
	//mainlatch.q_out_cb<1>().set(FUNC(slapfght_ms_state::flipscreen_w));

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_raw(20_MHz_XTAL / 4, 320, 0, 256, 266, 16, 240); // MOD 21/1 PROMs 201 and 203, the vertical position is a guess
	m_screen->set_screen_update(FUNC(slapfght_ms_state::screen_update));
	m_screen->screen_vblank().set(FUNC(slapfght_ms_state::vblank_irq));
	m_screen->set_palette(m_mod21);

	GFXDECODE(config, m_gfxdecode, m_mod21, gfx_slapfighm);

	GAELCO_MS_MOD21(config, m_mod21, 20_MHz_XTAL);
	m_mod21->set_screen(m_screen);

	GAELCO_MS_MOD51(config, m_mod51);
	m_mod51->set_timing(m_mod21);
	m_mod51->set_list(128, 0, false); // no colour bytes
	m_mod51->list_r().set([this] (offs_t offset) { return m_spriteram[offset]; });
	m_mod51->set_sprite_callback(FUNC(slapfght_ms_state::sprite_cb));

	GAELCO_MS_MOD4(config, m_mod4_pf);
	m_mod4_pf->set_palette(m_mod21);
	m_mod4_pf->set_timing(m_mod21);
	m_mod4_pf->set_color_base(0x100); // the program writes the same colours at 0x100 and 0x200
	m_mod4_pf->set_tile_size(8);
	m_mod4_pf->set_layout(TILEMAP_SCAN_ROWS, 64, 32);

	GAELCO_MS_MOD4(config, m_mod4_fix);
	m_mod4_fix->set_palette(m_mod21);
	m_mod4_fix->set_timing(m_mod21);
	m_mod4_fix->set_tile_size(8);
	m_mod4_fix->set_planes(2);
	m_mod4_fix->set_layout(TILEMAP_SCAN_ROWS, 64, 32);
	m_mod4_fix->set_transparent_pen(3);
	m_mod4_fix->set_tile_callback(FUNC(slapfght_ms_state::fix_tile_cb));

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD1(config, m_mod1, 4'000'000); // unknown, no crystal on the board
	m_mod1->set_ym_clock(1'500'000); // unknown, gives the pitch of the original game
	m_mod1->set_int_period(attotime::from_hz(360)); // unknown source, gives the tempo of the original game (music every other interrupt)
	m_mod1->in_callback<0>().set_ioport("IN0");
	m_mod1->in_callback<1>().set_ioport("IN1");
	m_mod1->in_callback<2>().set_ioport("SW2");
	m_mod1->in_callback<3>().set_ioport("SW1");
	m_mod1->add_route(ALL_OUTPUTS, "mono", 0.30);
}

ROM_START( slapfighm )
	ROM_REGION( 0x14000, "maincpu", 0 ) // on MOD. 3 board
	ROM_LOAD( "3_cpu_fi_303.ic12-512", 0x0000,  0x8000, CRC(639b0817) SHA1(0999d447d1cc182c5d1fb1be91e47f1e64675a78) ) // 27256
	ROM_LOAD( "3_cpu_fi_302.ic13-512", 0x8000,  0x8000, CRC(97403de3) SHA1(b96acd3b4bbe6e6dc060255aaa1b237100c41847) ) // 27256
	ROM_LOAD( "3_cpu_fi_301.ic14-513", 0x10000, 0x4000, CRC(c63b02fe) SHA1(8d1991780b7aed718f60b6a8f5de7571682fb713) ) // 27128, identical to the original once decrypted

	ROM_REGION( 0x4000, "mod1", 0 ) // on MOD 1 board
	ROM_LOAD( "1_snd_fi_0101.ic12", 0x0000, 0x4000, CRC(4e3ae13e) SHA1(4ab29064f3a4cb4fd70a9eb8ddbffb4f5ee74057) ) // 1xxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x20, "mod1:prom", 0 )
	ROM_LOAD( "1_snd_p0102_82s123.ic20", 0x00, 0x20, CRC(6228a653) SHA1(a8cfceba0546875940e9124c5ca2a6c625232856) )

	// only the upper half of the EPROMs is used
	ROM_REGION( 0x8000, "mod4_fix", ROMREGION_INVERT ) // on one MOD 4/2 board, contains the same data as the original among the weirdness
	ROM_LOAD16_BYTE( "4-2_fi_402.ic16", 0x0000, 0x4000, CRC(6e9ce9ea) SHA1(9b80360050022fb7b938a874d5c9adb344907bab) ) // BADADDR  x-xxxxxxxxxxxxx, 4-2_fi_402.ic16  [3/4] a77_04.6f IDENTICAL
	ROM_CONTINUE(                       0x0000, 0x4000 )
	ROM_LOAD16_BYTE( "4-2_fi_401.ic17", 0x0001, 0x4000, CRC(fbdc3ded) SHA1(5c1db5fe0ce32d996f40acf51b9ea09f7709a032) ) // BADADDR  --xxxxxxxxxxxxx, 4-2_fi_401.ic17  [3/4] a77_03.6g IDENTICAL
	ROM_CONTINUE(                       0x0001, 0x4000 )

	ROM_REGION( 0x20000, "mod4_pf", ROMREGION_INVERT ) // on a second MOD 4/2 board, identical to the original
	ROM_LOAD32_BYTE( "4-2-b_fi_4a04.ic14", 0x00000, 0x8000, CRC(b6358305) SHA1(c7bb4236a75ec6b88f011bc30f8fb9a718e2ca3e) )
	ROM_LOAD32_BYTE( "4-2-b_fi_4a03.ic15", 0x00001, 0x8000, CRC(e92d9d60) SHA1(2554617e0e6615ca8c85a49299a4a0e762478339) )
	ROM_LOAD32_BYTE( "4-2-b_fi_4a02.ic16", 0x00002, 0x8000, CRC(5faeeea3) SHA1(696fba24bcf1f3a7e914a4403854da5eededaf7f) )
	ROM_LOAD32_BYTE( "4-2-b_fi_4a01.ic17", 0x00003, 0x8000, CRC(974e2ea9) SHA1(3840550fc3a833828dad8f3e300d2ea583d69ce7) )

	ROM_REGION( 0x20000, "sprites", ROMREGION_INVERT ) // on MOD 51/1 board, different format from the original
	ROM_LOAD32_BYTE( "51-1_fi_504.ic40", 0x0000, 0x8000, CRC(b80afe0a) SHA1(02efc3cb0c968d24ae0da4d16515c244f59d1d93) )
	ROM_LOAD32_BYTE( "51-1_fi_503.ic41", 0x0001, 0x8000, CRC(b03543cf) SHA1(2b8f0e8c60dbccc40e82f4af799d151efc259d52) )
	ROM_LOAD32_BYTE( "51-1_fi_502.ic42", 0x0002, 0x8000, CRC(9bf4f6a4) SHA1(a4b1287258d1635108b349805f15ae25224e2ab6) )
	ROM_LOAD32_BYTE( "51-1_fi_501.ic43", 0x0003, 0x8000, CRC(fa901876) SHA1(e27a8f9014419343bf08a7536b4b37bded611ec0) )

	ROM_REGION( 0x200, "mod21", 0 ) // on MOD 21/1 board
	ROM_LOAD( "21-1_p0201_82s129.ic5",  0x000, 0x100, CRC(2697da58) SHA1(e62516b886ff6e204b718e5f0c6ce2712e4b7fc5) )
	ROM_LOAD( "21-1_p0203_82s129.ic12", 0x100, 0x100, CRC(d56e29b4) SHA1(d94157a9cd75d1f6d305f3d48291d0ae4c41e006) )

	ROM_REGION( 0x200, "mod3", 0 ) // on the CPU sub board, address decoder
	ROM_LOAD( "3-sub_cpu_p0312_82s147.bin", 0x000, 0x200, CRC(28d906a7) SHA1(da11a515aa798c0fb687d8617461b0372364b332) )

	ROM_REGION( 0x104, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "4-2_0400_pal16r6acn.ic29", 0x000, 0x104, CRC(cfbed5c6) SHA1(5ec184b973ed4b82a49678a84ebaf9e8e33ccd90) )
	ROM_LOAD( "4-2-b_pal16r6_p0400.ic29", 0x000, 0x104, CRC(19ac48ea) SHA1(bb79d2e1522f144609cdfe32a01defd19753668d) )
	ROM_LOAD( "51-1_p0503_pal16r6.ic46",  0x000, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
ROM_END

void slapfght_ms_state::init_decryption()
{
	gaelco_ms_mod3_descramble(&m_rom[0], m_rom.bytes());
}

} // anonymous namespace


GAME( 199?, slapfighm, alcon, slapfighm, slapfighm, slapfght_ms_state, init_decryption, ROT270, "bootleg (Gaelco / Ervisa)", "Slap Fight (Modular System)", MACHINE_NOT_WORKING )
