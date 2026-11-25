// license:BSD-3-Clause
// copyright-holders:David Haywood
/*
    Euro League (Gaelco bootleg, Modular System)

    Modular System is a stack of boards in a cage, there are other games on this 'system'.
    For this game the cage contains 6 boards:

    MOD 9/2   - "SYSTEM 2" sound, I/O and video timing board: Z0840006PSC, 2 x YM2203C, 2 x Y3014B, OKI M5205
                with a 384 kHz resonator, 20 MHz and 16 MHz XTALs, 3 x 6116 RAM, three GALs (9138, 9238 and
                9338), two 8 DIP switches banks and optocouplers for the inputs.
    CPU board - With the COMP 3Z-1 sub board on top: 2 x Z0840008PSC, 4 ROMs, 2 x 6264 RAM, 16 MHz XTAL
                and two GALs (3138 and 3238). The board under it has another 6264.
    MOD 4/3   - Tilemap board for the text: logic + 4 ROMs, GAL 403, long thin sub-board C0473.
    MOD 4/3   - Tilemap board for the foreground: logic + 4 ROMs, PAL P0403, long thin sub-board C0474.
    MOD 4/3   - Tilemap board for the background: logic + 4 ROMs, PAL P0403, long thin sub-board C0475.
    MOD 51/1  - Sprite board: logic, PROM P0502, PAL P0503 and the 8 ROMs on a REF 1M sub-board.

    PCB pics, DIP switches and more info: https://www.recreativas.org/modular-system-euro-league-4219-gaelco-sa

    The programs are those of the Euro League hack of World Cup '90 in wc90b.cpp with the hardware
    accesses moved. The first CPU writes the tile maps of the original game straight to the MOD 4
    boards, and their scroll in bytes it keeps out of the tile maps: the last four code bytes of the
    foreground (0xaffc) and text (0xeffc) maps, and 0xdf00-0xdf03 on the background board. The second
    CPU builds the MOD 51 sprite list, with colours 0-7 that the sprites take from palettes 8-15, and
    writes the palette in the format of the original game.

    There is no MOD 21/1 board: the MOD 9/2 board generates the video timing and has the palette RAM.

    Gaelco requested the registry of the "Euro League" trademark on 1990, and it was a Gaelco
    protected trademark (in Spain) until 1999 (they paid a 5-year renew in 1994):
    https://www.patentes-y-marcas.com/marca/euro-league-m1546246

    TODO:
    - the scroll offsets of the MOD 4 boards, fitted: with the MOD 9/2 timing the position model of the
      MOD 4 device puts the layers 5 pixels left (see the MOD 9/2 TODO for the horizontal sync)
    - how the CPU board gives the second CPU its interrupt
*/

#include "emu.h"

#include "../gaelco/gaelco_ms_mod4.h"
#include "../gaelco/gaelco_ms_mod51.h"
#include "../gaelco/gaelco_ms_mod9.h"

#include "cpu/z80/z80.h"

#include "screen.h"
#include "speaker.h"


namespace {

class euroleag_ms_state : public driver_device
{
public:
	euroleag_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_subcpu(*this, "sub"),
		m_gfxdecode(*this, "gfxdecode"),
		m_mod4(*this, { "mod4_tx", "mod4_fg", "mod4_bg" }),
		m_mod51(*this, "mod51"),
		m_mod9(*this, "mod9"),
		m_spriteram(*this, "spriteram"),
		m_mainbank(*this, "mainbank"),
		m_subbank(*this, "subbank")
	{ }

	void euroleagm(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	enum : u8 { MOD4_TX, MOD4_FG, MOD4_BG };

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_subcpu;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device_array<gaelco_ms_mod4_device, 3> m_mod4;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<gaelco_ms_mod9_ym2203_device> m_mod9;
	required_shared_ptr<u8> m_spriteram;
	required_memory_bank m_mainbank;
	required_memory_bank m_subbank;

	u8 mod4_read(u32 which, offs_t offset) const;
	template <u32 Which> u8 mod4_r(offs_t offset) { return mod4_read(Which, offset); }
	template <u32 Which> void mod4_w(offs_t offset, u8 data);
	void tile_cb(u16 data, u32 &code, u32 &color, u8 &flags);
	void sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8);
	u32 sprite_priority(u8 attr, u8 color);
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void main_bank_w(u8 data);
	void sub_bank_w(u8 data);
	void sub_irq_ack_w(u8 data);

	void main_map(address_map &map) ATTR_COLD;
	void sub_map(address_map &map) ATTR_COLD;
};


u8 euroleag_ms_state::mod4_read(u32 which, offs_t offset) const
{
	// each 4 KB holds 2048 attribute bytes and then 2048 code bytes, as in the video RAM of the original game
	offs_t const entry = (offset & 0x7ff) | (BIT(offset, 12) << 11);
	return BIT(offset, 11) ? m_mod4[which]->code_r(entry) : m_mod4[which]->attr_r(entry);
}

template <u32 Which>
void euroleag_ms_state::mod4_w(offs_t offset, u8 data)
{
	offs_t const entry = (offset & 0x7ff) | (BIT(offset, 12) << 11);
	if (BIT(offset, 11))
		m_mod4[Which]->code_w(entry, data);
	else
		m_mod4[Which]->attr_w(entry, data);
}

void euroleag_ms_state::tile_cb(u16 data, u32 &code, u32 &color, u8 &flags)
{
	// tile code bit 10 in attribute bit 3, as in the original game, and attribute bit 2 unused
	code = (data & 0x03ff) | (BIT(data, 11) << 10);
}

void euroleag_ms_state::sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8)
{
	// code bits 8-11 in attribute bits 0-3, X bit 8 in bit 5, flip X in bit 6 and flip Y in bit 7; the program writes
	// colours 0-7 and the sprites take palettes 8-15
	code |= (attr & 0x0f) << 8;
	palette = 8 | (color & 0x07);
	flipx = BIT(attr, 6);
	flipy = BIT(attr, 7);
	x8 = BIT(attr, 5);
}

u32 euroleag_ms_state::sprite_priority(u8 attr, u8 color)
{
	// attribute bit 4 puts the sprite behind the foreground and the text
	return BIT(attr, 4) ? GFX_PMASK_1 : 0;
}

u32 euroleag_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the offsets are fitted, see the TODO
	auto const scroll = [this] (u32 which, offs_t offset)
	{
		m_mod4[which]->set_scrollx(33 + (mod4_read(which, offset) | (mod4_read(which, offset + 1) << 8)));
		m_mod4[which]->set_scrolly(1 + (mod4_read(which, offset + 2) | (mod4_read(which, offset + 3) << 8)));
	};
	scroll(MOD4_TX, 0x0ffc);
	scroll(MOD4_FG, 0x0ffc);
	scroll(MOD4_BG, 0x1f00);

	screen.priority().fill(0, cliprect);
	m_mod4[MOD4_BG]->draw(screen, bitmap, cliprect);
	m_mod4[MOD4_FG]->draw(screen, bitmap, cliprect, 0, 1);
	m_mod4[MOD4_TX]->draw(screen, bitmap, cliprect, 0, 1);
	m_mod51->draw(bitmap, cliprect, *m_gfxdecode->gfx(0), screen.priority());

	return 0;
}


void euroleag_ms_state::main_bank_w(u8 data)
{
	m_mainbank->set_entry(data >> 3);
}

void euroleag_ms_state::sub_bank_w(u8 data)
{
	m_subbank->set_entry(data >> 3);
}

void euroleag_ms_state::sub_irq_ack_w(u8 data)
{
	m_subcpu->set_input_line(0, CLEAR_LINE);
}


void euroleag_ms_state::main_map(address_map &map)
{
	map(0x0000, 0x7fff).rom();
	map(0x8000, 0x9fff).ram();
	map(0xa000, 0xafff).rw(FUNC(euroleag_ms_state::mod4_r<MOD4_FG>), FUNC(euroleag_ms_state::mod4_w<MOD4_FG>));
	map(0xc000, 0xdfff).rw(FUNC(euroleag_ms_state::mod4_r<MOD4_BG>), FUNC(euroleag_ms_state::mod4_w<MOD4_BG>));
	map(0xe000, 0xefff).rw(FUNC(euroleag_ms_state::mod4_r<MOD4_TX>), FUNC(euroleag_ms_state::mod4_w<MOD4_TX>));
	map(0xf000, 0xf7ff).bankr(m_mainbank);
	map(0xf800, 0xfbff).ram().share("main_sub");
	map(0xfc00, 0xfc00).w(FUNC(euroleag_ms_state::main_bank_w));
	map(0xfd00, 0xfd0f).rw(m_mod9, FUNC(gaelco_ms_mod9_device_base::io_r), FUNC(gaelco_ms_mod9_device_base::io_w));
}

void euroleag_ms_state::sub_map(address_map &map)
{
	map(0x0000, 0xbfff).rom();
	map(0xc000, 0xcfff).ram();
	map(0xd000, 0xd7ff).ram().share(m_spriteram);
	map(0xd800, 0xdfff).ram();
	map(0xe000, 0xe7ff).rw(m_mod9, FUNC(gaelco_ms_mod9_device_base::palette8_r), FUNC(gaelco_ms_mod9_device_base::palette8_w));
	map(0xe800, 0xefff).rom();
	map(0xf000, 0xf7ff).bankr(m_subbank);
	map(0xf800, 0xfbff).ram().share("main_sub");
	map(0xfc00, 0xfc00).w(FUNC(euroleag_ms_state::sub_bank_w));
	map(0xfd0c, 0xfd0c).w(FUNC(euroleag_ms_state::sub_irq_ack_w));
}


static INPUT_PORTS_START( euroleagm )
	PORT_INCLUDE( gaelco_ms_mod9 )

	// on the MOD 9/2 board, locations from the DIP switch sheet of the cage
	PORT_START("DSW1")
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )      PORT_DIPLOCATION("SW1:1,2,3,4")
	PORT_DIPSETTING(    0x00, DEF_STR( 10C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )   PORT_DIPLOCATION("SW1:5,6")
	PORT_DIPSETTING(    0x30, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x40, 0x40, "Countdown Speed" )       PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )       // 60/60
	PORT_DIPSETTING(    0x00, "Fast" )                  // 56/60
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )  PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x03, 0x03, "1 Player Game Time" )    PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x01, "1:00" )
	PORT_DIPSETTING(    0x02, "1:30" )
	PORT_DIPSETTING(    0x03, "2:00" )
	PORT_DIPSETTING(    0x00, "2:30" )
	PORT_DIPNAME( 0x1c, 0x1c, "2 Player Game Time" )    PORT_DIPLOCATION("SW2:3,4,5")
	PORT_DIPSETTING(    0x0c, "1:00" )
	PORT_DIPSETTING(    0x14, "1:30" )
	PORT_DIPSETTING(    0x04, "2:00" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_DIPSETTING(    0x1c, "3:00" )
	PORT_DIPSETTING(    0x08, "3:30" )
	PORT_DIPSETTING(    0x10, "4:00" )
	PORT_DIPSETTING(    0x00, "5:00" )
	// "don't change" on the sheet
	PORT_DIPUNKNOWN_DIPLOC( 0x20, 0x20, "SW2:6" )
	PORT_DIPUNKNOWN_DIPLOC( 0x40, 0x40, "SW2:7" )
	PORT_DIPUNKNOWN_DIPLOC( 0x80, 0x00, "SW2:8" )
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

static GFXDECODE_START( gfx_euroleag_ms )
	GFXDECODE_ENTRY( "sprites", 0, sprite_layout, 0x000, 16 )
GFXDECODE_END


void euroleag_ms_state::machine_start()
{
	m_mainbank->configure_entries(0, 32, memregion("maincpu")->base() + 0x10000, 0x800);
	m_subbank->configure_entries(0, 32, memregion("sub")->base() + 0x10000, 0x800);
}


void euroleag_ms_state::euroleagm(machine_config &config)
{
	Z80(config, m_maincpu, 16_MHz_XTAL / 2); // divider guessed
	m_maincpu->set_addrmap(AS_PROGRAM, &euroleag_ms_state::main_map);

	Z80(config, m_subcpu, 16_MHz_XTAL / 2); // divider guessed
	m_subcpu->set_addrmap(AS_PROGRAM, &euroleag_ms_state::sub_map);
	m_subcpu->set_vblank_int("screen", FUNC(euroleag_ms_state::irq0_line_assert));

	screen_device &screen(SCREEN(config, "screen"));
	screen.set_raw(20_MHz_XTAL / 4, 320, 0, 256, 264, 16, 240);
	screen.set_screen_update(FUNC(euroleag_ms_state::screen_update));
	screen.set_palette(m_mod9);

	GFXDECODE(config, m_gfxdecode, m_mod9, gfx_euroleag_ms);

	GAELCO_MS_MOD4(config, m_mod4[MOD4_TX]);
	m_mod4[MOD4_TX]->set_palette(m_mod9);
	m_mod4[MOD4_TX]->set_color_base(0x100);
	m_mod4[MOD4_TX]->set_tile_size(8);
	m_mod4[MOD4_TX]->set_layout(TILEMAP_SCAN_ROWS, 64, 32);
	m_mod4[MOD4_TX]->set_transparent_pen(15);

	GAELCO_MS_MOD4(config, m_mod4[MOD4_FG]);
	m_mod4[MOD4_FG]->set_palette(m_mod9);
	m_mod4[MOD4_FG]->set_color_base(0x200);
	m_mod4[MOD4_FG]->set_layout(TILEMAP_SCAN_ROWS, 64, 32);
	m_mod4[MOD4_FG]->set_transparent_pen(15);
	m_mod4[MOD4_FG]->set_tile_callback(FUNC(euroleag_ms_state::tile_cb));

	GAELCO_MS_MOD4(config, m_mod4[MOD4_BG]);
	m_mod4[MOD4_BG]->set_palette(m_mod9);
	m_mod4[MOD4_BG]->set_color_base(0x300);
	m_mod4[MOD4_BG]->set_layout(TILEMAP_SCAN_ROWS, 64, 32);
	m_mod4[MOD4_BG]->set_tile_callback(FUNC(euroleag_ms_state::tile_cb));

	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD9_YM2203(config, m_mod9, 20_MHz_XTAL);
	m_mod9->set_screen(screen);
	m_mod9->irq_callback().set_inputline(m_maincpu, 0);
	m_mod9->in_callback<0>().set_ioport("IN0");
	m_mod9->in_callback<1>().set_ioport("IN1");
	m_mod9->in_callback<2>().set_ioport("DSW1");
	m_mod9->in_callback<3>().set_ioport("DSW2");
	m_mod9->add_route(ALL_OUTPUTS, "mono", 1.0);

	GAELCO_MS_MOD51(config, m_mod51);
	m_mod51->set_timing(m_mod9);
	m_mod51->list_r().set([this] (offs_t offset) { return m_spriteram[offset]; });
	m_mod51->set_sprite_callback(FUNC(euroleag_ms_state::sprite_cb));
	m_mod51->set_priority_callback(FUNC(euroleag_ms_state::sprite_priority));
}


ROM_START( euroleagm )
	ROM_REGION( 0x20000, "maincpu", 0 ) // on the COMP 3Z-1 sub board
	ROM_LOAD( "3z-1_fu301.ic17", 0x00000, 0x10000, CRC(74acc161) SHA1(d8660dd6d05164df4a66125c68627e955b35bef3) ) // 7000-ffff empty
	ROM_LOAD( "3z-1_fu302.ic15", 0x10000, 0x10000, CRC(f54ff17a) SHA1(a19850fc28a5a0da20795a5cc6b56d9c16554bce) ) // banked at f000-f7ff

	ROM_REGION( 0x20000, "sub", 0 ) // on the COMP 3Z-1 sub board
	ROM_LOAD( "3z-1_fu303.ic19", 0x00000, 0x10000, CRC(348195fa) SHA1(41c59e38ec4ba4f3c2185dd32dbf4ea0318ab375) ) // c000-dfff and f000-ffff empty
	ROM_LOAD( "3z-1_fu304.ic16", 0x10000, 0x10000, CRC(9e421c4b) SHA1(e23a1f1d5d1e960696f45df653869712eb889839) ) // banked at f000-f7ff

	ROM_REGION( 0x10000, "mod9", 0 ) // on the MOD 9/2 board, sound program and samples
	ROM_LOAD( "system2_fu101.ic6", 0x00000, 0x10000, CRC(712566ca) SHA1(9e46f9d449ff549b7a6d82283d8f903189b058e7) )

	ROM_REGION( 0x20000, "mod4_tx", ROMREGION_INVERT ) // on the MOD 4/3 board with sub-board C0473
	ROM_LOAD32_BYTE( "r1_fu401.ic17",  0x00003, 0x08000, CRC(b93201e0) SHA1(edc8dbab15d901aab678244811d8e08d4cb974f2) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_LOAD32_BYTE( "r1_fu402.ic16",  0x00002, 0x08000, CRC(da7f3ce3) SHA1(80ae95208a807e3866560ad5f192951d33f1da34) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_LOAD32_BYTE( "r1_fu403.ic15",  0x00001, 0x08000, CRC(f1effd53) SHA1(0299678e1716a92cd6ecc67d28dd95422f84d15d) ) // 1ST AND 2ND HALF IDENTICAL
	ROM_LOAD32_BYTE( "r1_fu404.ic14",  0x00000, 0x08000, CRC(1659529a) SHA1(ad7a2bab43922871ce7081d46efdeefa9db5e45f) ) // 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x40000, "mod4_fg", ROMREGION_INVERT ) // on the MOD 4/3 board with sub-board C0474
	ROM_LOAD32_BYTE( "r2_fu4a01.ic17", 0x00003, 0x10000, CRC(1bcdd99d) SHA1(e37a8308eff79b04ce3da75fec2fffe4021c70d3) )
	ROM_LOAD32_BYTE( "r2_fu4a02.ic16", 0x00002, 0x10000, CRC(e625de0c) SHA1(ca5a00ea27a5107d2957a1637b260d5aa1ff5563) )
	ROM_LOAD32_BYTE( "r2_fu4a03.ic15", 0x00001, 0x10000, CRC(995df90d) SHA1(121874c807001bf65a9cec90dd30212f1889a398) )
	ROM_LOAD32_BYTE( "r2_fu4a04.ic14", 0x00000, 0x10000, CRC(44031f34) SHA1(0b417c6cc1ed029394bcdce80e81262d776105b7) )

	ROM_REGION( 0x40000, "mod4_bg", ROMREGION_INVERT ) // on the MOD 4/3 board with sub-board C0475
	ROM_LOAD32_BYTE( "r3_fu4b01.ic17", 0x00003, 0x10000, CRC(adc7feda) SHA1(ff82e973599f13c76ecaf3c027a2967468cd9e72) )
	ROM_LOAD32_BYTE( "r3_fu4b02.ic16", 0x00002, 0x10000, CRC(1e5c0fda) SHA1(74163098810ff079467bad77796898426acbfac5) )
	ROM_LOAD32_BYTE( "r3_fu4b03.ic15", 0x00001, 0x10000, CRC(cc9dab0b) SHA1(7e5735e57b45de1b344455fa5a047ae17933de27) )
	ROM_LOAD32_BYTE( "r3_fu4b04.ic14", 0x00000, 0x10000, CRC(d54704d2) SHA1(7d4ec0120c3516a88abbe687e8916b44bffb7bcd) )

	ROM_REGION( 0x80000, "sprites", ROMREGION_INVERT ) // on the REF 1M sub-board of the MOD 51/1 board
	ROM_LOAD32_BYTE( "r4_fu501.bin",   0x00003, 0x10000, CRC(d5a60096) SHA1(a8e351a4b020b4fc2b2cb7d3f0fdfb43fc44d7d9) )
	ROM_LOAD32_BYTE( "r4_fu502.bin",   0x00002, 0x10000, CRC(26371c18) SHA1(0887041d86dc9f19dad264ae27dc56fb89ac3265) )
	ROM_LOAD32_BYTE( "r4_fu503.bin",   0x00001, 0x10000, CRC(0da825f9) SHA1(cfba0c85fc767726c1d63f87468335d1c2f1eed8) )
	ROM_LOAD32_BYTE( "r4_fu504.bin",   0x00000, 0x10000, CRC(516b6c09) SHA1(9d02514dece864b087f67886009ce54bd51b5575) )
	ROM_LOAD32_BYTE( "r4_fu5a01.bin",  0x40003, 0x10000, CRC(36bbf467) SHA1(627b5847ffb098c92edfd58c25391799f3b209e0) )
	ROM_LOAD32_BYTE( "r4_fu5a02.bin",  0x40002, 0x10000, CRC(75aa9b86) SHA1(0c221bd2e8a5472bb0e515f27fb72b0c8e8c0ca4) )
	ROM_LOAD32_BYTE( "r4_fu5a03.bin",  0x40001, 0x10000, CRC(228429d8) SHA1(3b2dbea53807929c24d593c469a83172f7747f66) )
	ROM_LOAD32_BYTE( "r4_fu5a04.bin",  0x40000, 0x10000, CRC(f36390a9) SHA1(e5ea36e91b3ced068281524ee79d0432f489715c) )

	ROM_REGION( 0x100, "mod51", 0 ) // on the MOD 51/1 board
	ROM_LOAD( "r4_p0502_82s129.ic10",      0x000, 0x100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )

	ROM_REGION( 0x157, "mod9:timing", 0 ) // on the MOD 9/2 board
	ROM_LOAD( "system2_9238_gal20v8.ic18", 0x000, 0x157, CRC(dd571a59) SHA1(ef2c7b33922dd79513a4fbe6f04a2f2c2c795ada) )

	ROM_REGION( 0x117, "mod9:decoder", 0 ) // on the MOD 9/2 board, sound CPU decoder
	ROM_LOAD( "system2_9338_gal16v8.ic10", 0x000, 0x117, CRC(54b4160f) SHA1(0156a2eda97a9d8e0adb3a3795f6ed547c6e06fc) )

	ROM_REGION( 0x157, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "r2_p0403_pal16r8a.ic29",    0x000, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "r3_p0403_pal16r8a.ic29",    0x000, 0x104, CRC(d8c6ac25) SHA1(d6184e491313ff8da5b1ce60ffe8ef517716807c) )
	ROM_LOAD( "r4_p0503_pal16r6.ic46",     0x000, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
	ROM_LOAD( "3z-1_3138_gal16v8.ic22",    0x000, 0x117, CRC(909dab7b) SHA1(e9f4bb239fa7843743e85e236ae0c744784a3b3f) ) // Same as Gaelco Goldart ?
	ROM_LOAD( "3z-1_3238_gal16v8.ic24",    0x000, 0x117, CRC(e9e538d9) SHA1(9ea73a903a06111843fe64ae55cb29ee88803334) ) // Same as Gaelco Goldart ?
	ROM_LOAD( "r1_403_gal16v8.ic29",       0x000, 0x117, CRC(c136de93) SHA1(116f6d3b456d20621ab07a005c1421f57569915c) )
	ROM_LOAD( "system2_9138_gal16v8.ic42", 0x000, 0x117, CRC(bd9ad8c3) SHA1(50e00b0cf7d075f9daed0338bc336a74caa3b66b) )
ROM_END

} // anonymous namespace


GAME( 1989, euroleagm, twcup90, euroleagm, euroleagm, euroleag_ms_state, empty_init, ROT0, "bootleg (Gaelco / Ervisa)", "Euro League (Gaelco bootleg, Modular System)", MACHINE_IMPERFECT_SOUND | MACHINE_SUPPORTS_SAVE )
