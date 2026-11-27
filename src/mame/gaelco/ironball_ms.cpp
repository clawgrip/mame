// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote
/*
    Iron Balls (prototype on Modular System)

    A Gaelco game in development, on a Modular System cage with a development CPU board like World
    Rally's. Its title screen says "VERSION: 1.1 BETA". The cage has 5 boards:

    MOD 6-ESP - Development CPU board: MC68000P10, 24 MHz XTAL, EPROMs IRON EVEN and IRON ODD, two
                CXK58256PM-10 RAMs, two empty sockets, GALs 06FL and 16FL, NMI and reset buttons, a RD/WR
                switch and a connector for the development system.
    MOD 7/4   - Tilemap board: two 8x8 layers and a 16x16 one. GALs 71FA, 7250-FL, 73FA, 74FL, 75FL and
                76FA.
    MOD 8/2   - ROM board of the MOD 7/4 board: four EPROMs for each layer, F (front), M (middle) and FON
                (background).
    MOD 51/3  - Sprite board: EPROMs BOLA 540-543, PROM 502, GAL 503.
    MOD 9/2   - "SYSTEM 2" sound, I/O and video timing board: Z84C0008PSC, YM3812 (relabelled 5A12), Y3014B,
                OKI M5205 with a 384 kHz resonator, 28 MHz and 16 MHz XTALs, EPROMs BO 906 and BO 911, GALs
                9150, 9250 and 9350, two banks of 8 DIP switches and optocouplers for the inputs.

    The program copies itself to RAM at 0x40400 and runs from there, as a program loaded by the
    development system would. Its ROM checksum screen sums only 0x5516 bytes of that copy: its loop
    count is 0x15516, but dbra takes 16 bits.

    The memory map is the one of the other games with a MOD 7/4 board: the front layer at 0x80000, the
    middle one at 0x90000, the background at 0xa0000 and their scroll registers at 0xc0000. The sprite
    list has 256 entries in two halves, at 0x000 and 0x400, with each colour byte 0x200 after its entry:
    the list of the other cages with A9 and A10 swapped.

    TODO:
    - the layer offsets and priorities, taken from Splash (Modular System), whose MOD 9/2 timing GAL has
      the same label; what scroll registers 4 and 5 and bit 15 of register 2 do
    - the program RAM size: the copy needs 0x40400-0x5f827
    - DIP switches 1, 2 and 8 of SW1 (debugging options of the beta?)
    - sprite attribute bit 4, set with colour bit 3 and on a few other sprites (priority?)
*/

#include "emu.h"

#include "gaelco_ms_mod51.h"
#include "gaelco_ms_mod9.h"

#include "cpu/m68000/m68000.h"

#include "screen.h"
#include "speaker.h"
#include "tilemap.h"


namespace {

class ironball_ms_state : public driver_device
{
public:
	ironball_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_gfxdecode(*this, "gfxdecode"),
		m_mod51(*this, "mod51"),
		m_mod9(*this, "mod9"),
		m_vram(*this, "vram_%u", 0U),
		m_scroll(*this, "scroll"),
		m_spriteram(*this, "spriteram")
	{ }

	void ironballm(machine_config &config) ATTR_COLD;

	void init_ironballm() ATTR_COLD;

protected:
	virtual void video_start() override ATTR_COLD;

private:
	enum : u8 { LAYER_FRONT, LAYER_MIDDLE, LAYER_BACK };

	required_device<cpu_device> m_maincpu;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<gaelco_ms_mod9_device> m_mod9;
	required_shared_ptr_array<u16, 3> m_vram;
	required_shared_ptr<u16> m_scroll;
	required_shared_ptr<u16> m_spriteram;

	tilemap_t *m_tilemap[3]{};

	template <u32 Layer> TILE_GET_INFO_MEMBER(get_tile_info);
	template <u32 Layer> void vram_w(offs_t offset, u16 data, u16 mem_mask = ~0);
	u8 sprite_byte(offs_t offset) const;
	void sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8);
	u32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void main_map(address_map &map) ATTR_COLD;
};


template <u32 Layer>
TILE_GET_INFO_MEMBER(ironball_ms_state::get_tile_info)
{
	// a code word and an attribute word for each tile: colour in bits 0-4, flip X in bit 6 and flip Y in bit 7
	u16 const code = m_vram[Layer][tile_index * 2];
	u16 const attr = m_vram[Layer][tile_index * 2 + 1];
	tileinfo.set(Layer, code, attr & 0x1f, TILE_FLIPYX(BIT(attr, 6, 2)));
}

template <u32 Layer>
void ironball_ms_state::vram_w(offs_t offset, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_vram[Layer][offset]);
	m_tilemap[Layer]->mark_tile_dirty(offset / 2);
}

u8 ironball_ms_state::sprite_byte(offs_t offset) const
{
	offs_t const address = bitswap<11>(offset, 9, 10, 8, 7, 6, 5, 4, 3, 2, 1, 0);
	return m_spriteram[address >> 1] >> (BIT(address, 0) ? 0 : 8);
}

void ironball_ms_state::sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8)
{
	// code bits 8-11 in attribute bits 0-3, X bit 8 in bit 5, flip X in bit 6 and flip Y in bit 7
	code |= (attr & 0x0f) << 8;
	x8 = BIT(attr, 5);
	flipx = BIT(attr, 6);
	flipy = BIT(attr, 7);
}

void ironball_ms_state::video_start()
{
	m_tilemap[LAYER_FRONT] = &machine().tilemap().create(*m_gfxdecode,
			tilemap_get_info_delegate(*this, FUNC(ironball_ms_state::get_tile_info<LAYER_FRONT>)), TILEMAP_SCAN_ROWS, 8, 8, 64, 32);
	m_tilemap[LAYER_MIDDLE] = &machine().tilemap().create(*m_gfxdecode,
			tilemap_get_info_delegate(*this, FUNC(ironball_ms_state::get_tile_info<LAYER_MIDDLE>)), TILEMAP_SCAN_ROWS, 8, 8, 64, 32);
	m_tilemap[LAYER_BACK] = &machine().tilemap().create(*m_gfxdecode,
			tilemap_get_info_delegate(*this, FUNC(ironball_ms_state::get_tile_info<LAYER_BACK>)), TILEMAP_SCAN_ROWS, 16, 16, 64, 32);

	// as on the other MOD 7/4 boards; the front layer covers the playfield with pen 0 between levels
	m_tilemap[LAYER_FRONT]->set_transparent_pen(15);
	m_tilemap[LAYER_MIDDLE]->set_transparent_pen(0);
}

u32 ironball_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the program writes 2 and 4 to the X registers of the middle and background layers for no scroll
	m_tilemap[LAYER_FRONT]->set_scrollx(0, 64 - m_scroll[2]);
	m_tilemap[LAYER_FRONT]->set_scrolly(0, -m_scroll[3]);
	m_tilemap[LAYER_MIDDLE]->set_scrollx(0, 64 - (m_scroll[0] - 2));
	m_tilemap[LAYER_MIDDLE]->set_scrolly(0, -m_scroll[1]);
	m_tilemap[LAYER_BACK]->set_scrollx(0, 64 - (m_scroll[6] - 4));
	m_tilemap[LAYER_BACK]->set_scrolly(0, -m_scroll[7]);

	m_tilemap[LAYER_BACK]->draw(screen, bitmap, cliprect, TILEMAP_DRAW_OPAQUE, 0);
	m_tilemap[LAYER_MIDDLE]->draw(screen, bitmap, cliprect, 0, 0);
	m_mod51->draw(bitmap, cliprect, *m_gfxdecode->gfx(3));
	m_tilemap[LAYER_FRONT]->draw(screen, bitmap, cliprect, 0, 0);

	return 0;
}


void ironball_ms_state::main_map(address_map &map)
{
	map(0x000000, 0x01ffff).rom();
	map(0x040000, 0x05ffff).ram(); // the program copies itself here
	map(0x080000, 0x081fff).ram().w(FUNC(ironball_ms_state::vram_w<LAYER_FRONT>)).share(m_vram[LAYER_FRONT]);
	map(0x090000, 0x091fff).ram().w(FUNC(ironball_ms_state::vram_w<LAYER_MIDDLE>)).share(m_vram[LAYER_MIDDLE]);
	map(0x0a0000, 0x0a1fff).ram().w(FUNC(ironball_ms_state::vram_w<LAYER_BACK>)).share(m_vram[LAYER_BACK]);
	map(0x0c0000, 0x0c000f).ram().share(m_scroll);
	map(0x100000, 0x1007ff).ram().share(m_spriteram);
	map(0x200000, 0x200fff).rw(m_mod9, FUNC(gaelco_ms_mod9_device_base::palette_r), FUNC(gaelco_ms_mod9_device_base::palette_w));
	map(0x400000, 0x40000f).rw(m_mod9, FUNC(gaelco_ms_mod9_device_base::io16_r), FUNC(gaelco_ms_mod9_device_base::io16_w));
	map(0xff0000, 0xffffff).ram();
}


static INPUT_PORTS_START( ironballm )
	PORT_INCLUDE( gaelco_ms_mod9 )

	// on the MOD 9/2 board
	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:1") // checks player 2 button 2 somewhere
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:2") // 2 or 4 somewhere
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Credits" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, "Common" )
	PORT_DIPSETTING(    0x00, "Per player" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW1:4,5") // added to the level for its parameters
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW1:6,7")
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x60, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:8") // steps a value 0-7 somewhere
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW2:1,2,3")
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW2:4,5,6")
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW2:7" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW2:8" )
INPUT_PORTS_END


static const gfx_layout tiles8x8x4_layout =
{
	8, 8,
	RGN_FRAC(1, 1),
	4,
	{ 0, 8, 16, 24 },
	{ STEP8(0, 1) },
	{ STEP8(0, 32) },
	8 * 32
};

static const gfx_layout tiles16x16x4_layout =
{
	16, 16,
	RGN_FRAC(1, 1),
	4,
	{ 0, 8, 16, 24 },
	{ STEP8(0, 1), STEP8(512, 1) },
	{ STEP16(0, 32) },
	16 * 16 * 4
};

static GFXDECODE_START( gfx_ironballm )
	GFXDECODE_ENTRY( "front",   0, tiles8x8x4_layout,   0x000, 32 )
	GFXDECODE_ENTRY( "middle",  0, tiles8x8x4_layout,   0x000, 32 )
	GFXDECODE_ENTRY( "back",    0, tiles16x16x4_layout, 0x000, 32 )
	GFXDECODE_ENTRY( "sprites", 0, tiles16x16x4_layout, 0x200, 16 )
GFXDECODE_END


void ironball_ms_state::init_ironballm()
{
	// the MOD 8/2 board keeps the four 8x8 quarters of each 16x16 tile 256 tiles apart
	u8 *const src = memregion("back")->base();
	u32 const len = memregion("back")->bytes();
	std::vector<u8> buffer(src, src + len);
	for (u32 i = 0; i < len; i++)
		src[bitswap<20>(i, 19, 18, 17, 16, 15, 12, 11, 10, 9, 8, 7, 6, 5, 14, 13, 4, 3, 2, 1, 0)] = buffer[i];
}

void ironball_ms_state::ironballm(machine_config &config)
{
	M68000(config, m_maincpu, 24_MHz_XTAL / 2); // divider not verified
	m_maincpu->set_addrmap(AS_PROGRAM, &ironball_ms_state::main_map);

	screen_device &screen(SCREEN(config, "screen"));
	screen.set_raw(28_MHz_XTAL / 4, 464, 0, 384, 264, 0, 240);
	screen.set_screen_update(FUNC(ironball_ms_state::screen_update));
	screen.set_palette(m_mod9);

	GFXDECODE(config, m_gfxdecode, m_mod9, gfx_ironballm);

	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD9(config, m_mod9, 28_MHz_XTAL);
	m_mod9->set_screen("screen");
	m_mod9->irq_callback().set_inputline(m_maincpu, 4);
	m_mod9->in_callback<0>().set_ioport("IN0");
	m_mod9->in_callback<1>().set_ioport("IN1");
	m_mod9->in_callback<2>().set_ioport("DSW1");
	m_mod9->in_callback<3>().set_ioport("DSW2");
	m_mod9->add_route(ALL_OUTPUTS, "mono", 1.0);

	GAELCO_MS_MOD51(config, m_mod51);
	m_mod51->set_timing(m_mod9);
	m_mod51->set_list(256, 0x400, true);
	m_mod51->list_r().set([this] (offs_t offset) { return sprite_byte(offset); });
	m_mod51->set_sprite_callback(FUNC(ironball_ms_state::sprite_cb));
}


ROM_START( ironballm )
	ROM_REGION( 0x20000, "maincpu", 0 ) // on the MOD 6-ESP board
	ROM_LOAD16_BYTE( "mod_6_esp_iron_even_27c512.bin", 0x00000, 0x10000, CRC(99342bf4) SHA1(76f10fb76cb0e22a9a99a3a050f27c4f098e79d9) )
	ROM_LOAD16_BYTE( "mod_6_esp_iron_odd_27c512.bin",  0x00001, 0x10000, CRC(40869935) SHA1(6006af6cddd40d7d1348894332cadd884caa15de) )

	ROM_REGION( 0x40000, "front", 0 ) // on the MOD 8/2 board
	ROM_LOAD32_BYTE( "mod_8-2_f1_27c512.ic37", 0x00000, 0x10000, CRC(c49f59a9) SHA1(429606db9867be58d81c0337ad7958631153f221) )
	ROM_LOAD32_BYTE( "mod_8-2_f2_27c512.ic30", 0x00001, 0x10000, CRC(16140816) SHA1(b5b63ec697a60c38adfd223845c0d1c373191c4b) )
	ROM_LOAD32_BYTE( "mod_8-2_f3_27c512.ic22", 0x00002, 0x10000, CRC(4ad6a8bd) SHA1(72338eb0be8c30b04f944583306a40583049bf5c) )
	ROM_LOAD32_BYTE( "mod_8-2_f4_27c512.ic15", 0x00003, 0x10000, CRC(ff39b705) SHA1(fc15258057ea4d37625eac6351628cd43220db96) )

	ROM_REGION( 0x40000, "middle", 0 ) // on the MOD 8/2 board
	ROM_LOAD32_BYTE( "mod_8-2_m1_27c512.ic33", 0x00000, 0x10000, CRC(7a4960c9) SHA1(f069c8bc7af7298dd4b76b9e4be8993a0149c3cc) )
	ROM_LOAD32_BYTE( "mod_8-2_m2_27c512.ic16", 0x00001, 0x10000, CRC(f4dc2281) SHA1(11c42cc3a0db4d61defe56f7381cd340cbe807c2) )
	ROM_LOAD32_BYTE( "mod_8-2_m3_27c512.ic18", 0x00002, 0x10000, CRC(232223c4) SHA1(3a0568ef407101bfe569c272828d1a0e0a786401) )
	ROM_LOAD32_BYTE( "mod_8-2_m4_27c512.ic11", 0x00003, 0x10000, CRC(d55d9325) SHA1(ce95b277344daf7c6431f251a40c560a914a2b1f) )

	ROM_REGION( 0x40000, "back", 0 ) // on the MOD 8/2 board
	ROM_LOAD32_BYTE( "mod_8-2_fon1_27c512.ic35", 0x00000, 0x10000, CRC(8e81ea49) SHA1(c863362c5fd08a1886c401cc047e0d21517cf1ff) )
	ROM_LOAD32_BYTE( "mod_8-2_fon2_27c512.ic28", 0x00001, 0x10000, CRC(b5384fe1) SHA1(8b4a5a08edc11a4af078e09d2a7d70072a233f96) )
	ROM_LOAD32_BYTE( "mod_8-2_fon3_27c512.ic20", 0x00002, 0x10000, CRC(aae53541) SHA1(a1c71bbf4f70e72af0e354dd1c8e3e18cc2994b6) )
	ROM_LOAD32_BYTE( "mod_8-2_fon4_27c512.ic13", 0x00003, 0x10000, CRC(78ff1148) SHA1(a38a8bea2ada8edf3aa23944efc0c290480410e9) )

	ROM_REGION( 0x80000, "sprites", ROMREGION_INVERT ) // on the MOD 51/3 board
	ROM_LOAD32_BYTE( "mod_51-3_bola_543_27c010.ic40", 0x00000, 0x20000, CRC(fb2e2199) SHA1(b1f2ba4f6e717b57b6f86feadfc0ba09efb8178c) )
	ROM_LOAD32_BYTE( "mod_51-3_bola_542_27c010.ic41", 0x00001, 0x20000, CRC(d4539e9f) SHA1(723939ee30cccc3f232dfd2471bc645e39ae2fd4) )
	ROM_LOAD32_BYTE( "mod_51-3_bola_541_27c010.ic42", 0x00002, 0x20000, CRC(8302cded) SHA1(c910d07033a817d2c3fb1ea44c34bdebbaf13dbb) )
	ROM_LOAD32_BYTE( "mod_51-3_bola_540_27c010.ic43", 0x00003, 0x20000, CRC(e35d2903) SHA1(efbe14591088faee2f2da7d5593b634cac68d766) )

	ROM_REGION( 0x50000, "mod9", 0 ) // on the MOD 9/2 board, sound program and samples
	ROM_LOAD( "mod_9-2_system_2_bo_906_27c512.ic6",  0x00000, 0x10000, CRC(c4ec82ba) SHA1(9b4cf0825e04e506ce04b1e58e9221452c6afa56) )
	ROM_LOAD( "mod_9-2_system_2_bo_911_27c020.ic11", 0x10000, 0x40000, CRC(3b4df4c9) SHA1(a7546fc64db2652d523970ab5d0b3271ccc4db0e) ) // the board decodes 128 KB, the data is in the first 32 KB

	ROM_REGION( 0x157, "mod9:timing", 0 ) // on the MOD 9/2 board
	ROM_LOAD( "mod_9-2_system_2_9250_gal20v8.ic18", 0x000, 0x157, BAD_DUMP CRC(d123dec1) SHA1(e68b91467924d3f94aa0495feeeeeee273a8b443) ) // not dumped, assumed to be the same as Splash's 9250

	// the decoder GAL of the MOD 9/2 board (IC10, 9350) isn't dumped, assumed to be the same as on the other boards

	ROM_REGION( 0x157, "plds", 0 )
	ROM_LOAD( "mod_6_esp_06fl_gal16v8.bin",         0x000, 0x117, NO_DUMP )
	ROM_LOAD( "mod_6_esp_16fl_gal16v8.bin",         0x000, 0x117, BAD_DUMP CRC(fe9294d1) SHA1(e64539a5e661acc21fe040a1a478a63ea9bf7ab8) ) // not dumped, assumed to be the same as World Rally's 16FL
	ROM_LOAD( "mod_7-4_71fa_gal20v8.ic7",           0x000, 0x157, NO_DUMP )
	ROM_LOAD( "mod_7-4_7250-fl_gal20v8.ic54",       0x000, 0x157, BAD_DUMP CRC(e9ec1e9e) SHA1(90d74aa6e15a3bba93c8264baffc72545be1474f) ) // not dumped, assumed to be the same as Splash's 7250 and World Rally's 72FL
	ROM_LOAD( "mod_7-4_73fa_gal16v8.ic55",          0x000, 0x117, NO_DUMP )
	ROM_LOAD( "mod_7-4_74fl_gal16v8.ic9",           0x000, 0x117, BAD_DUMP CRC(2634cce1) SHA1(e7ac65e24bae2973eb1ac914797e3b3140c267eb) ) // not dumped, assumed to be the same as World Rally's 74FL
	ROM_LOAD( "mod_7-4_75fl_gal16v8.ic59",          0x000, 0x117, BAD_DUMP CRC(b3e2f5c5) SHA1(d03fbcb28818c8a29ed42369ce4a6734f2a87b4f) ) // not dumped, assumed to be the same as World Rally's 75FL
	ROM_LOAD( "mod_7-4_76fa_gal20v8.ic44",          0x000, 0x157, NO_DUMP )
	ROM_LOAD( "mod_51-3_503_gal16v8.ic46",          0x000, 0x117, BAD_DUMP CRC(11470ea1) SHA1(cfcafbcc7e55be717348f895df61e144fdd0cc9b) ) // not dumped, assumed to be the same as the other boards' 503
	ROM_LOAD( "mod_9-2_system_2_9150_gal16v8.ic42", 0x000, 0x117, NO_DUMP )
ROM_END

} // anonymous namespace


GAME( 199?, ironballm, 0, ironballm, ironballm, ironball_ms_state, init_ironballm, ROT0, "Gaelco", "Iron Balls (prototype on Modular System, version 1.1 beta)", MACHINE_SUPPORTS_SAVE )
