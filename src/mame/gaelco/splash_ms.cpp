// license:BSD-3-Clause
// copyright-holders: David Haywood

/*
    Splash (Modular System)
*/


#include "emu.h"
#include "gaelco_ms_mod51.h"
#include "gaelco_ms_mod9.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"
#include "machine/bankdev.h"


class splashms_state : public driver_device
{
public:
	splashms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_subcpu(*this, "subcpu"),
		m_screen(*this, "screen"),
		m_gfxdecode(*this, "gfxdecode"),
		m_mod51(*this, "mod51"),
		m_mod9(*this, "mod9"),
		m_videoram(*this, "videoram"),
		m_videoram2(*this, "videoram2"),
		m_bitmapram(*this, "bitmapram"),
		m_scrollregs(*this, "scrollregs"),
		m_spriteram(*this, "spriteram"),
		m_bgdata(*this, "subcpu"),
		m_subram(*this, "subrambank"),
		m_subrom(*this, "subrombank")
	{ }

	void splashm(machine_config &config) ATTR_COLD;
	void init_splashms() ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_subcpu;

	required_device<screen_device> m_screen;

	required_device<gfxdecode_device> m_gfxdecode;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<gaelco_ms_mod9_device> m_mod9;
	required_shared_ptr<uint16_t> m_videoram;
	required_shared_ptr<uint16_t> m_videoram2;
	required_shared_ptr<uint8_t> m_bitmapram;
	required_shared_ptr<uint16_t> m_scrollregs;
	required_shared_ptr<uint16_t> m_spriteram;
	required_region_ptr<uint8_t> m_bgdata;
	required_device<address_map_bank_device> m_subram;
	required_device<address_map_bank_device> m_subrom;

	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;
	virtual void video_start() override ATTR_COLD;

	u8 sprite_byte(offs_t offset) const { return m_spriteram[offset >> 1] >> (BIT(offset, 0) ? 0 : 8); }
	void sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void splashms_map(address_map &map) ATTR_COLD;
	void sub_map(address_map &map) ATTR_COLD;
	void sub_portmap(address_map &map) ATTR_COLD;

	uint8_t frommain_command_r();
	void vram_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	uint16_t vram2_r(offs_t offset, uint16_t mem_mask);
	void vram2_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	TILE_GET_INFO_MEMBER(get_tile_info_tilemap1);
	TILE_GET_INFO_MEMBER(get_tile_info_tilemap2);
	tilemap_t *m_bg_tilemap = nullptr;
	tilemap_t *m_bg_tilemap2 = nullptr;

	void to_subcpu_0x400004_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	uint8_t m_subcmd = 0;

	void sub_rambankselect_w(uint8_t data);
	void sub_rombankselect_w(uint8_t data);

	void subrambank_map(address_map &map) ATTR_COLD;
	void subrombank_map(address_map &map) ATTR_COLD;

	void descramble_16x16tiles(uint8_t* src, int len);
};


void splashms_state::sub_rambankselect_w(uint8_t data)
{
//  logerror("sub_rambankselect_w %02x\n", data);
	m_subram->set_bank(data&0x7);
}

void splashms_state::sub_rombankselect_w(uint8_t data)
{
//  logerror("sub_rombankselect_w %02x\n", data);
	m_subrom->set_bank(data & 0x7f);
}


uint8_t splashms_state::frommain_command_r()
{
	return m_subcmd;
}

void splashms_state::to_subcpu_0x400004_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	// no IRQ, subcpu just polls this
	//popmessage("to_subcpu_0x400004_w %04x\n", data);
	m_subcmd = data;
}
void splashms_state::vram_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_videoram[offset]);
	m_bg_tilemap->mark_tile_dirty(offset/2);
}

uint16_t splashms_state::vram2_r(offs_t offset, uint16_t mem_mask)
{
	return m_videoram2[offset];
}

void splashms_state::vram2_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_videoram2[offset]);
	m_bg_tilemap2->mark_tile_dirty(offset/2);
}

TILE_GET_INFO_MEMBER(splashms_state::get_tile_info_tilemap1)
{
	int tile = m_videoram[tile_index*2];
	int attr = m_videoram[(tile_index*2)+1] & 0x0f;
	int fx = (m_videoram[(tile_index*2)+1] & 0xc0)>>6;

	tileinfo.set(1,tile,attr,TILE_FLIPYX(fx));
}

TILE_GET_INFO_MEMBER(splashms_state::get_tile_info_tilemap2)
{
	int tile = m_videoram2[tile_index*2];

	tile &= 0x1ff;

	int attr = m_videoram2[(tile_index*2)+1] & 0x0f;
	int fx = (m_videoram2[(tile_index*2)+1] & 0xc0)>>6;

	tileinfo.set(2,tile,attr,TILE_FLIPYX(fx));
}

void splashms_state::sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8)
{
	// code bits 8-13 in attribute bits 0-5, flip X in bit 6, flip Y in bit 7 and X bit 8 in colour byte bit 7
	code |= (attr & 0x3f) << 8;
	flipx = BIT(attr, 6);
	flipy = BIT(attr, 7);
	x8 = BIT(color, 7);
}

uint32_t splashms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
//  bitmap.fill(0, cliprect);

	for (int y = cliprect.top(); y <= cliprect.bottom(); y++)
	{
		uint16_t *const dst = &bitmap.pix(y);

		for (int x = cliprect.left(); x <= cliprect.right(); x++)
		{
			uint8_t const pix = m_bitmapram[(y * 512) + x];
			//if (pix)
			dst[x] = pix + 0x100;
		}
	}

	m_bg_tilemap2->set_scrollx(0, 64-(m_scrollregs[0]-0x2));
	m_bg_tilemap2->set_scrolly(0, -m_scrollregs[1]);

	m_bg_tilemap->set_scrollx(0, 64-(m_scrollregs[2]));
	m_bg_tilemap->set_scrolly(0, -m_scrollregs[3]);

	m_bg_tilemap2->draw(screen, bitmap, cliprect, 0, 0);

	m_mod51->draw(bitmap, cliprect, *m_gfxdecode->gfx(0));

	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 0);

	return 0;
}

void splashms_state::video_start()
{
	m_bg_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(splashms_state::get_tile_info_tilemap1)), TILEMAP_SCAN_ROWS,  8,  8, 64, 32);
	m_bg_tilemap2 = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(splashms_state::get_tile_info_tilemap2)), TILEMAP_SCAN_ROWS,  16,  16, 64, 32);

	m_bg_tilemap->set_transparent_pen(15);
	m_bg_tilemap2->set_transparent_pen(0);
}


void splashms_state::splashms_map(address_map &map)
{
	map(0x000000, 0x03ffff).rom(); // writes to 0x030000 on startup

	map(0x080000, 0x081fff).ram().w(FUNC(splashms_state::vram_w)).share("videoram");

	map(0x08e000, 0x08ffff).rw(FUNC(splashms_state::vram2_r), FUNC(splashms_state::vram2_w)); // mirror needed for paint dripping in attract
	map(0x090000, 0x091fff).ram().rw(FUNC(splashms_state::vram2_r), FUNC(splashms_state::vram2_w)).share("videoram2");

	map(0x0a0000, 0x0a1fff).ram(); // writes unused data from vram2 area here, but isn't a mirror (eg. has large paint can from attract during gameplay) unused?

	map(0x0c0000, 0x0c000f).ram().share("scrollregs"); // scroll vals

	map(0x100000, 0x1007ff).ram().share("spriteram");

	map(0x200000, 0x200fff).rw(m_mod9, FUNC(gaelco_ms_mod9_device::palette_r), FUNC(gaelco_ms_mod9_device::palette_w));

	map(0x400000, 0x40000f).rw(m_mod9, FUNC(gaelco_ms_mod9_device::io16_r), FUNC(gaelco_ms_mod9_device::io16_w));
	map(0x400004, 0x400005).w(FUNC(splashms_state::to_subcpu_0x400004_w));

	map(0xff0000, 0xffffff).ram();
}

void splashms_state::sub_portmap(address_map &map)
{
	map.global_mask(0xff);
	map(0x01, 0x01).w(FUNC(splashms_state::sub_rambankselect_w)); // banking for 0x4000-0x7fff RAM
	map(0x02, 0x02).w(FUNC(splashms_state::sub_rombankselect_w)); // banking for 0x8000-0xffff ROM

	map(0x03, 0x03).r(FUNC(splashms_state::frommain_command_r));
}

void splashms_state::sub_map(address_map &map)
{
	map(0x0000, 0x0fff).rom();
	map(0x4000, 0x7fff).m(m_subram, FUNC(address_map_bank_device::amap8));
	map(0x8000, 0xffff).m(m_subrom, FUNC(address_map_bank_device::amap8));
}


void splashms_state::machine_start()
{
}

void splashms_state::machine_reset()
{
	m_subcmd = 0;
	m_subram->set_bank(0);
	m_subrom->set_bank(0);
}


static INPUT_PORTS_START( splashm )
	PORT_INCLUDE( gaelco_ms_mod9 )

	// on the MOD 9/2 board
	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x14, "5" )
	PORT_DIPSETTING(    0x18, "6" )
	PORT_DIPSETTING(    0x1c, "7" )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x60, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0xa0, "5" )
	PORT_DIPSETTING(    0xc0, "6" )
	PORT_DIPSETTING(    0xe0, "7" )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
INPUT_PORTS_END

static const gfx_layout tiles16x16x4_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7, 512,513,514,515,516,517,518,519 },
	{ STEP16(0,32) },
	16 * 16 * 4
};

static const gfx_layout tiles8x8x4_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7 },
	{ STEP8(0,32) },
	16 * 16
};


static GFXDECODE_START( gfx_splashms )
	GFXDECODE_ENTRY( "sprites", 0, tiles16x16x4_layout, 0x200, 16 )
	GFXDECODE_ENTRY( "fgtile", 0, tiles8x8x4_layout, 0, 16 )
	GFXDECODE_ENTRY( "bgtile", 0, tiles16x16x4_layout, 0, 16 )
GFXDECODE_END

void splashms_state::subrambank_map(address_map &map)
{
	map(0x00000, 0x1ffff).ram().share("bitmapram");
}

void splashms_state::subrombank_map(address_map &map)
{
	map(0x000000, 0x3fffff).rom().region("subcpu", 0x000000);
}


void splashms_state::splashm(machine_config &config)
{
	// basic machine hardware
	M68000(config, m_maincpu, 20_MHz_XTAL/2);
	m_maincpu->set_addrmap(AS_PROGRAM, &splashms_state::splashms_map);

	Z80(config, m_subcpu, 12_MHz_XTAL/2);
	m_subcpu->set_addrmap(AS_PROGRAM, &splashms_state::sub_map);
	m_subcpu->set_addrmap(AS_IO, &splashms_state::sub_portmap);

	ADDRESS_MAP_BANK(config, m_subram).set_map(&splashms_state::subrambank_map).set_options(ENDIANNESS_LITTLE, 8, 17, 0x4000);
	ADDRESS_MAP_BANK(config, m_subrom).set_map(&splashms_state::subrombank_map).set_options(ENDIANNESS_LITTLE, 8, 22, 0x8000);

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_raw(28_MHz_XTAL / 4, 464, 0, 384, 264, 0, 240);
	m_screen->set_screen_update(FUNC(splashms_state::screen_update));
	m_screen->set_palette(m_mod9);

	GFXDECODE(config, m_gfxdecode, m_mod9, gfx_splashms);

	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD9(config, m_mod9, 28_MHz_XTAL); // not verified, the crystal of the other MOD 9/2 boards
	m_mod9->set_screen(m_screen);
	m_mod9->irq_callback().set_inputline(m_maincpu, 4);
	m_mod9->in_callback<0>().set_ioport("IN0");
	m_mod9->in_callback<1>().set_ioport("IN1");
	m_mod9->in_callback<2>().set_ioport("DSW1");
	m_mod9->in_callback<3>().set_ioport("DSW2");
	m_mod9->add_route(ALL_OUTPUTS, "mono", 1.0);

	GAELCO_MS_MOD51(config, m_mod51);
	m_mod51->set_timing(m_mod9);
	m_mod51->set_list(128, 0x200, true);
	m_mod51->list_r().set([this] (offs_t offset) { return sprite_byte(offset); });
	m_mod51->set_sprite_callback(FUNC(splashms_state::sprite_cb));
}

// reorganize graphics into something we can decode with a single pass
void splashms_state::descramble_16x16tiles(uint8_t* src, int len)
{
	std::vector<uint8_t> buffer(len);
	{
		for (int i = 0; i < len; i++)
		{
			int j = bitswap<20>(i, 19,18,17,16,15,12,11,10,9,8,7,6,5,14,13,4,3,2,1,0);
			buffer[j] = src[i];
		}

		std::copy(buffer.begin(), buffer.end(), &src[0]);
	}
}

void splashms_state::init_splashms()
{
	descramble_16x16tiles(memregion("bgtile")->base(), memregion("bgtile")->bytes());
	// fgtile is 8x8 tiles
}


ROM_START( splashm )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "cpu1_6-1_sp_608a.ic8",  0x00001, 0x20000, CRC(befdbaf0) SHA1(94efdeec1e1311317ffd0fe3d5fdbb02e151b985) )
	ROM_LOAD16_BYTE( "cpu1_6-1_sp_617a.ic17", 0x00000, 0x20000, CRC(080edb2b) SHA1(6104345bc72cd20051d66c04b97c9a365a88ec3f) )

	ROM_REGION( 0x400000, "subcpu", ROMREGION_ERASE00 ) // extra Z80 for backgrounds!
	ROM_LOAD( "cpu2_c_sp_c2.ic2", 0x000000, 0x080000, CRC(3a0be09f) SHA1(83abc10ff2c810c8451f583700f140f569e5b6ee) )
	ROM_LOAD( "cpu2_c_sp_c3.ic3", 0x080000, 0x080000, CRC(c3dc5e9d) SHA1(ce5fb65935cfe225132242e058cd63fa33f9da63) )
	ROM_LOAD( "cpu2_c_sp_c4.ic4", 0x100000, 0x080000, CRC(4d7b643d) SHA1(40bdcf7eedddc3244cb41530d10009b23b7ac473) )
	ROM_LOAD( "cpu2_c_sp_c5.ic5", 0x180000, 0x080000, CRC(7ba31717) SHA1(000cf6ec261ac90efc3e4f2dbf6720a54fb3bbdb) )
	ROM_LOAD( "cpu2_c_sp_c6.ic6", 0x200000, 0x080000, CRC(994e8e16) SHA1(cbe7d9e192d0390b123f1b585a0463634f33b485) )
	ROM_LOAD( "cpu2_c_sp_c7.ic7", 0x280000, 0x080000, CRC(6ea0be42) SHA1(5cc45ef1f3c8f46e1e7b3ad3313d221a65fb0025) )

	ROM_REGION( 0x10000, "mod9", 0 ) // on MOD 9/2 board, sound program and samples
	ROM_LOAD( "snd_9-2_sp_916.ic6", 0x00000, 0x10000, CRC(5567fa22) SHA1(3993c733a0222ca292b60f409c78b45280a5fec6) )

	ROM_REGION( 0x40000, "fgtile", ROMREGION_ERASEFF )
	ROM_LOAD32_BYTE( "8_sp_837.ic37", 0x00000, 0x10000, CRC(3b544131) SHA1(e7fd97cb24b84739f2481efb1d232f86df4a3d8d) ) // 1xxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_830.ic30", 0x00001, 0x10000, CRC(09bb675b) SHA1(49c41ccfce1b0077c430c6bb38bc858aeaf87fb8) ) // has some garbage in the blank space of the paired ROMs
	ROM_LOAD32_BYTE( "8_sp_822.ic22", 0x00002, 0x10000, CRC(621fcf26) SHA1(a7ff6b12fbbea1bba7c4a397a82ac2fb5c09558a) ) // 1xxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_815.ic15", 0x00003, 0x10000, CRC(5641b621) SHA1(e71df1ab5c9b2254495d99657477b52e8843d128) ) // 1xxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x080000, "sprites", ROMREGION_ERASEFF | ROMREGION_INVERT )
	ROM_LOAD32_BYTE( "5-1_sp_524.ic24", 0x00000, 0x10000, CRC(841c24c1) SHA1(70cb26033999f8184c51849e00bfcb2270f646e8) )
	ROM_LOAD32_BYTE( "5-1_sp_518.ic18", 0x00001, 0x10000, CRC(499cb813) SHA1(4d22e58530ff8a85b7ffc8ae1ab5986215986b49) )
	ROM_LOAD32_BYTE( "5-1_sp_512.ic12", 0x00002, 0x10000, CRC(8cb0b132) SHA1(894f84b6a8171ed8c22298ebf1303da020f747ee) )
	ROM_LOAD32_BYTE( "5-1_sp_503.ic3",  0x00003, 0x10000, CRC(ace09666) SHA1(d223718118b9912643d320832414df942e411e70) )
	ROM_LOAD32_BYTE( "5-1_sp_525.ic25", 0x40000, 0x10000, CRC(46bde779) SHA1(b4e1dd1952276c2d2a8f3c150d1ba2a1c2b738b7) ) // 11xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "5-1_sp_519.ic19", 0x40001, 0x10000, CRC(69cc9e06) SHA1(85e1495d01e6986f9cd88d6cdbef194c623be111) ) // 11xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "5-1_sp_513.ic13", 0x40002, 0x10000, CRC(2f7eeb33) SHA1(27b5eb0ce299931f9fb1831bb218a27b2b6c66d7) )
	ROM_LOAD32_BYTE( "5-1_sp_504.ic4",  0x40003, 0x10000, CRC(b6806390) SHA1(d95247bcd90bd7b7be355c267f023c19a9d60f66) ) // 11xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x080000, "bgtile", ROMREGION_ERASEFF )
	ROM_LOAD32_BYTE( "8_sp_833.ic33", 0x00000, 0x20000, CRC(8ac7cd1f) SHA1(aad88db9a82f417774f1d5eef830cc97c0d4b0de) ) // 111xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_826.ic26", 0x00001, 0x20000, CRC(b7ec71d8) SHA1(3d4b62559c0ba688b94e605594f3e8e9f2cbefa2) ) // 111xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_818.ic18", 0x00002, 0x20000, CRC(ae62a832) SHA1(f825a186e25a1c292aa6f880055341ec14373c0b) ) // 111xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_811.ic11", 0x00003, 0x20000, CRC(02e474b4) SHA1(11446655cb73ec4961339fa4ee41200f8b2b81d3) ) // 111xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x100, "mod51", 0 ) // on MOD 51 board
	ROM_LOAD( "51-3_502_82s129.ic10", 0x000, 0x100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) ) // common PROM found on all? Modular System sets?

	ROM_REGION( 0x157, "mod9:timing", 0 ) // on MOD 9/2 board
	ROM_LOAD( "snd_9-2_9250_gal20v8as.ic18", 0x000, 0x157, CRC(d123dec1) SHA1(e68b91467924d3f94aa0495feeeeeee273a8b443) )

	ROM_REGION( 0x117, "mod9:decoder", ROMREGION_ERASEFF ) // on MOD 9/2 board, sound CPU decoder
	ROM_LOAD( "snd_9-2_9359_gal16v8as.ic10", 0x000, 0x117, NO_DUMP )
	// not dumped, the 93xx of Euro League's, Big Karnak's and World Rally's boards
	ROM_FILL( 0x000, 0x02, 0x00 )
	ROM_FILL( 0x002, 0x01, 0x08 )
	ROM_FILL( 0x003, 0x01, 0x92 )
	ROM_FILL( 0x008, 0x01, 0x66 )
	ROM_FILL( 0x009, 0x01, 0xd6 )
	ROM_FILL( 0x00a, 0x01, 0xed )
	ROM_FILL( 0x00c, 0x01, 0x66 )
	ROM_FILL( 0x00d, 0x01, 0xfa )
	ROM_FILL( 0x00e, 0x01, 0xed )
	ROM_FILL( 0x010, 0x01, 0xa6 )
	ROM_FILL( 0x011, 0x01, 0xfe )
	ROM_FILL( 0x012, 0x01, 0xed )
	ROM_FILL( 0x014, 0x01, 0xea )
	ROM_FILL( 0x015, 0x01, 0xfe )
	ROM_FILL( 0x016, 0x01, 0xed )
	ROM_FILL( 0x018, 0x4c, 0x00 )
	ROM_FILL( 0x068, 0x01, 0xdf )
	ROM_FILL( 0x069, 0x01, 0xfe )
	ROM_FILL( 0x06a, 0x01, 0xed )
	ROM_FILL( 0x06b, 0x01, 0xbf )
	ROM_FILL( 0x06c, 0x18, 0x00 )
	ROM_FILL( 0x089, 0x01, 0xfd )
	ROM_FILL( 0x08a, 0x01, 0xed )
	ROM_FILL( 0x08c, 0x01, 0xdf )
	ROM_FILL( 0x08d, 0x01, 0xfe )
	ROM_FILL( 0x08e, 0x01, 0xed )
	ROM_FILL( 0x08f, 0x01, 0x7f )
	ROM_FILL( 0x090, 0x14, 0x00 )
	ROM_FILL( 0x0a8, 0x01, 0xa6 )
	ROM_FILL( 0x0a9, 0x01, 0xfe )
	ROM_FILL( 0x0ac, 0x01, 0x6a )
	ROM_FILL( 0x0ad, 0x01, 0xfe )
	ROM_FILL( 0x0b0, 0x01, 0xaa )
	ROM_FILL( 0x0b1, 0x01, 0xf6 )
	ROM_FILL( 0x0b4, 0x10, 0x00 )
	ROM_FILL( 0x0c8, 0x01, 0xdf )
	ROM_FILL( 0x0c9, 0x01, 0xfe )
	ROM_FILL( 0x0cb, 0x01, 0xef )
	ROM_FILL( 0x0cc, 0x01, 0xe6 )
	ROM_FILL( 0x0cd, 0x01, 0xf6 )
	ROM_FILL( 0x0d0, 0x01, 0x6a )
	ROM_FILL( 0x0d1, 0x01, 0xfe )
	ROM_FILL( 0x0d4, 0x01, 0xaa )
	ROM_FILL( 0x0d5, 0x01, 0xfa )
	ROM_FILL( 0x0d8, 0x0c, 0x00 )
	ROM_FILL( 0x0e8, 0x01, 0xdf )
	ROM_FILL( 0x0e9, 0x01, 0xfd )
	ROM_FILL( 0x0ec, 0x01, 0xdf )
	ROM_FILL( 0x0ed, 0x01, 0xfe )
	ROM_FILL( 0x0ef, 0x01, 0xfd )
	ROM_FILL( 0x0f0, 0x01, 0xe6 )
	ROM_FILL( 0x0f1, 0x01, 0xfa )
	ROM_FILL( 0x0f4, 0x01, 0x6a )
	ROM_FILL( 0x0f5, 0x01, 0xfe )
	ROM_FILL( 0x0f8, 0x01, 0xaa )
	ROM_FILL( 0x0f9, 0x01, 0xfa )
	ROM_FILL( 0x0fc, 0x09, 0x00 )
	ROM_FILL( 0x116, 0x01, 0x03 )

	ROM_REGION( 0x157, "plds", 0 )
	ROM_LOAD( "5-1_5150_gal16v8as.ic9",      0x000, 0x117, CRC(0070e8b9) SHA1(8a3ef9445599dc88c001a919e01f47844eec81eb) )
	ROM_LOAD( "5-1_5250_gal16v8as.ic8",      0x000, 0x117, CRC(f3d686c9) SHA1(6bc1bd40f49663e776c0d40b2f146338a2057097) )
	ROM_LOAD( "7-4_7150_gal20v8as.ic7",      0x000, 0x157, CRC(b0644601) SHA1(8b0410006bdffe552ee27d027bdb5b3c642fe49c) )
	ROM_LOAD( "7-4_7250_gal20v8as.ic54",     0x000, 0x157, CRC(e9ec1e9e) SHA1(90d74aa6e15a3bba93c8264baffc72545be1474f) )
	ROM_LOAD( "7-4_7350_gal16v8as.ic5",      0x000, 0x117, CRC(e112efd9) SHA1(8005da091200f2482c0ff5bf8f9c0fd2ab767fac) )
	ROM_LOAD( "7-4_7450_gal16v8as.ic9",      0x000, 0x117, CRC(2634cce1) SHA1(e7ac65e24bae2973eb1ac914797e3b3140c267eb) )
	ROM_LOAD( "7-4_7550_gal16v8as.ic59",     0x000, 0x117, CRC(b3e2f5c5) SHA1(d03fbcb28818c8a29ed42369ce4a6734f2a87b4f) )
	ROM_LOAD( "7-4_7650_gal20v8as.ic44",     0x000, 0x157, CRC(91030d03) SHA1(c30662f1b70ef0e92629824ac240d878cac64b5e) )
	ROM_LOAD( "51-3_503_gal16v8.ic46",       0x000, 0x117, CRC(11470ea1) SHA1(cfcafbcc7e55be717348f895df61e144fdd0cc9b) )
	ROM_LOAD( "cpu1_6-1_605_gal16v8as.ic13", 0x000, 0x117, CRC(45b8e164) SHA1(2e227ac5171ebf14e1d1f7214626ef4df0d5ae99) )
	ROM_LOAD( "cpu1_6-1_650_gal16v8as.ic7",  0x000, 0x117, CRC(b80b8485) SHA1(ea7f6312877cef31c93a8e02270565bd511f4aaa) )
	ROM_LOAD( "cpu2_c_cso_gal16v8as.ic10",   0x000, 0x117, CRC(0ebc7cd7) SHA1(95f0cba1f588920634b5f09b85197474201d9aed) )
	ROM_LOAD( "snd_9-2_9159_gal16v8as.ic42", 0x000, 0x117, NO_DUMP )
ROM_END

ROM_START( splashma )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "sp_608_27c010.bin", 0x00001, 0x20000, CRC(5367f36f) SHA1(fa42b4571bb14ce2f2adc681f1141fcffc9de757) )
	ROM_LOAD16_BYTE( "sp_617_27c010.bin", 0x00000, 0x20000, CRC(754c2d1a) SHA1(7f9417ad3009e7d2c4e9df618b24fd87a4233cbd) )

	ROM_REGION( 0x400000, "subcpu", ROMREGION_ERASE00 ) // extra Z80 for backgrounds!
	ROM_LOAD( "sp_c2_20f4_27c4001.bin", 0x000000, 0x080000, CRC(3a0be09f) SHA1(83abc10ff2c810c8451f583700f140f569e5b6ee) )
	ROM_LOAD( "sp_c3_8edb_27c4001.bin", 0x080000, 0x080000, CRC(c3dc5e9d) SHA1(ce5fb65935cfe225132242e058cd63fa33f9da63) )
	ROM_LOAD( "sp_c4_9b95_27c4001.bin", 0x100000, 0x080000, CRC(4d7b643d) SHA1(40bdcf7eedddc3244cb41530d10009b23b7ac473) )
	ROM_LOAD( "sp_c5_5188_27c4001.bin", 0x180000, 0x080000, CRC(7ba31717) SHA1(000cf6ec261ac90efc3e4f2dbf6720a54fb3bbdb) )
	ROM_LOAD( "sp_c6_1a0f_27c4001.bin", 0x200000, 0x080000, CRC(994e8e16) SHA1(cbe7d9e192d0390b123f1b585a0463634f33b485) )
	ROM_LOAD( "sp_c7_08d7_27c4001.bin", 0x280000, 0x080000, CRC(b5a729a5) SHA1(7e8c887264678d018477e64e2e8f531ef450c742) )

	ROM_REGION( 0x010000, "mod9", 0 ) // on MOD 9/2 board, sound program and samples
	ROM_LOAD( "snd_9-2_sp_916.ic6", 0x000000, 0x010000, CRC(5567fa22) SHA1(3993c733a0222ca292b60f409c78b45280a5fec6) )

	ROM_REGION( 0x40000, "fgtile", ROMREGION_ERASEFF )
	ROM_LOAD32_BYTE( "8_sp_837.ic37", 0x00000, 0x10000, CRC(3b544131) SHA1(e7fd97cb24b84739f2481efb1d232f86df4a3d8d) ) // 1xxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_830.ic30", 0x00001, 0x10000, CRC(09bb675b) SHA1(49c41ccfce1b0077c430c6bb38bc858aeaf87fb8) ) // has some garbage in the blank space of the paired ROMs
	ROM_LOAD32_BYTE( "8_sp_822.ic22", 0x00002, 0x10000, CRC(621fcf26) SHA1(a7ff6b12fbbea1bba7c4a397a82ac2fb5c09558a) ) // 1xxxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_815.ic15", 0x00003, 0x10000, CRC(5641b621) SHA1(e71df1ab5c9b2254495d99657477b52e8843d128) ) // 1xxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x80000, "sprites", ROMREGION_ERASEFF | ROMREGION_INVERT )
	ROM_LOAD32_BYTE( "5-1_sp_524.ic24", 0x00000, 0x10000, CRC(841c24c1) SHA1(70cb26033999f8184c51849e00bfcb2270f646e8) )
	ROM_LOAD32_BYTE( "5-1_sp_518.ic18", 0x00001, 0x10000, CRC(499cb813) SHA1(4d22e58530ff8a85b7ffc8ae1ab5986215986b49) )
	ROM_LOAD32_BYTE( "5-1_sp_512.ic12", 0x00002, 0x10000, CRC(8cb0b132) SHA1(894f84b6a8171ed8c22298ebf1303da020f747ee) )
	ROM_LOAD32_BYTE( "5-1_sp_503.ic3",  0x00003, 0x10000, CRC(ace09666) SHA1(d223718118b9912643d320832414df942e411e70) )
	ROM_LOAD32_BYTE( "5-1_sp_525.ic25", 0x40000, 0x10000, CRC(46bde779) SHA1(b4e1dd1952276c2d2a8f3c150d1ba2a1c2b738b7) ) // 11xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "5-1_sp_519.ic19", 0x40001, 0x10000, CRC(69cc9e06) SHA1(85e1495d01e6986f9cd88d6cdbef194c623be111) ) // 11xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "5-1_sp_513.ic13", 0x40002, 0x10000, CRC(2f7eeb33) SHA1(27b5eb0ce299931f9fb1831bb218a27b2b6c66d7) )
	ROM_LOAD32_BYTE( "5-1_sp_504.ic4",  0x40003, 0x10000, CRC(b6806390) SHA1(d95247bcd90bd7b7be355c267f023c19a9d60f66) ) // 11xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x80000, "bgtile", ROMREGION_ERASEFF )
	ROM_LOAD32_BYTE( "8_sp_833.ic33", 0x00000, 0x20000, CRC(8ac7cd1f) SHA1(aad88db9a82f417774f1d5eef830cc97c0d4b0de) ) // 111xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_826.ic26", 0x00001, 0x20000, CRC(b7ec71d8) SHA1(3d4b62559c0ba688b94e605594f3e8e9f2cbefa2) ) // 111xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_818.ic18", 0x00002, 0x20000, CRC(ae62a832) SHA1(f825a186e25a1c292aa6f880055341ec14373c0b) ) // 111xxxxxxxxxxxxxx = 0xFF
	ROM_LOAD32_BYTE( "8_sp_811.ic11", 0x00003, 0x20000, CRC(02e474b4) SHA1(11446655cb73ec4961339fa4ee41200f8b2b81d3) ) // 111xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x100, "mod51", 0 ) // on MOD 51 board
	ROM_LOAD( "51-3_502_82s129.ic10", 0x000, 0x100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )

	ROM_REGION( 0x157, "mod9:timing", 0 ) // on MOD 9/2 board
	ROM_LOAD( "snd_9-2_9250_gal20v8as.ic18", 0x000, 0x157, CRC(d123dec1) SHA1(e68b91467924d3f94aa0495feeeeeee273a8b443) )

	ROM_REGION( 0x117, "mod9:decoder", ROMREGION_ERASEFF ) // on MOD 9/2 board, sound CPU decoder
	ROM_LOAD( "snd_9-2_9359_gal16v8as.ic10", 0x000, 0x117, NO_DUMP )
	// not dumped, the 93xx of Euro League's, Big Karnak's and World Rally's boards
	ROM_FILL( 0x000, 0x02, 0x00 )
	ROM_FILL( 0x002, 0x01, 0x08 )
	ROM_FILL( 0x003, 0x01, 0x92 )
	ROM_FILL( 0x008, 0x01, 0x66 )
	ROM_FILL( 0x009, 0x01, 0xd6 )
	ROM_FILL( 0x00a, 0x01, 0xed )
	ROM_FILL( 0x00c, 0x01, 0x66 )
	ROM_FILL( 0x00d, 0x01, 0xfa )
	ROM_FILL( 0x00e, 0x01, 0xed )
	ROM_FILL( 0x010, 0x01, 0xa6 )
	ROM_FILL( 0x011, 0x01, 0xfe )
	ROM_FILL( 0x012, 0x01, 0xed )
	ROM_FILL( 0x014, 0x01, 0xea )
	ROM_FILL( 0x015, 0x01, 0xfe )
	ROM_FILL( 0x016, 0x01, 0xed )
	ROM_FILL( 0x018, 0x4c, 0x00 )
	ROM_FILL( 0x068, 0x01, 0xdf )
	ROM_FILL( 0x069, 0x01, 0xfe )
	ROM_FILL( 0x06a, 0x01, 0xed )
	ROM_FILL( 0x06b, 0x01, 0xbf )
	ROM_FILL( 0x06c, 0x18, 0x00 )
	ROM_FILL( 0x089, 0x01, 0xfd )
	ROM_FILL( 0x08a, 0x01, 0xed )
	ROM_FILL( 0x08c, 0x01, 0xdf )
	ROM_FILL( 0x08d, 0x01, 0xfe )
	ROM_FILL( 0x08e, 0x01, 0xed )
	ROM_FILL( 0x08f, 0x01, 0x7f )
	ROM_FILL( 0x090, 0x14, 0x00 )
	ROM_FILL( 0x0a8, 0x01, 0xa6 )
	ROM_FILL( 0x0a9, 0x01, 0xfe )
	ROM_FILL( 0x0ac, 0x01, 0x6a )
	ROM_FILL( 0x0ad, 0x01, 0xfe )
	ROM_FILL( 0x0b0, 0x01, 0xaa )
	ROM_FILL( 0x0b1, 0x01, 0xf6 )
	ROM_FILL( 0x0b4, 0x10, 0x00 )
	ROM_FILL( 0x0c8, 0x01, 0xdf )
	ROM_FILL( 0x0c9, 0x01, 0xfe )
	ROM_FILL( 0x0cb, 0x01, 0xef )
	ROM_FILL( 0x0cc, 0x01, 0xe6 )
	ROM_FILL( 0x0cd, 0x01, 0xf6 )
	ROM_FILL( 0x0d0, 0x01, 0x6a )
	ROM_FILL( 0x0d1, 0x01, 0xfe )
	ROM_FILL( 0x0d4, 0x01, 0xaa )
	ROM_FILL( 0x0d5, 0x01, 0xfa )
	ROM_FILL( 0x0d8, 0x0c, 0x00 )
	ROM_FILL( 0x0e8, 0x01, 0xdf )
	ROM_FILL( 0x0e9, 0x01, 0xfd )
	ROM_FILL( 0x0ec, 0x01, 0xdf )
	ROM_FILL( 0x0ed, 0x01, 0xfe )
	ROM_FILL( 0x0ef, 0x01, 0xfd )
	ROM_FILL( 0x0f0, 0x01, 0xe6 )
	ROM_FILL( 0x0f1, 0x01, 0xfa )
	ROM_FILL( 0x0f4, 0x01, 0x6a )
	ROM_FILL( 0x0f5, 0x01, 0xfe )
	ROM_FILL( 0x0f8, 0x01, 0xaa )
	ROM_FILL( 0x0f9, 0x01, 0xfa )
	ROM_FILL( 0x0fc, 0x09, 0x00 )
	ROM_FILL( 0x116, 0x01, 0x03 )

	ROM_REGION( 0x157, "plds", 0 )
	ROM_LOAD( "5-1_5150_gal16v8as.ic9",      0x000, 0x117, CRC(0070e8b9) SHA1(8a3ef9445599dc88c001a919e01f47844eec81eb) )
	ROM_LOAD( "5-1_5250_gal16v8as.ic8",      0x000, 0x117, CRC(f3d686c9) SHA1(6bc1bd40f49663e776c0d40b2f146338a2057097) )
	ROM_LOAD( "7-4_7150_gal20v8as.ic7",      0x000, 0x157, CRC(b0644601) SHA1(8b0410006bdffe552ee27d027bdb5b3c642fe49c) )
	ROM_LOAD( "7-4_7250_gal20v8as.ic54",     0x000, 0x157, CRC(e9ec1e9e) SHA1(90d74aa6e15a3bba93c8264baffc72545be1474f) )
	ROM_LOAD( "7-4_7350_gal16v8as.ic5",      0x000, 0x117, CRC(e112efd9) SHA1(8005da091200f2482c0ff5bf8f9c0fd2ab767fac) )
	ROM_LOAD( "7-4_7450_gal16v8as.ic9",      0x000, 0x117, CRC(2634cce1) SHA1(e7ac65e24bae2973eb1ac914797e3b3140c267eb) )
	ROM_LOAD( "7-4_7550_gal16v8as.ic59",     0x000, 0x117, CRC(b3e2f5c5) SHA1(d03fbcb28818c8a29ed42369ce4a6734f2a87b4f) )
	ROM_LOAD( "7-4_7650_gal20v8as.ic44",     0x000, 0x157, CRC(91030d03) SHA1(c30662f1b70ef0e92629824ac240d878cac64b5e) )
	ROM_LOAD( "51-3_503_gal16v8.ic46",       0x000, 0x117, CRC(11470ea1) SHA1(cfcafbcc7e55be717348f895df61e144fdd0cc9b) )
	ROM_LOAD( "cpu1_6-1_605_gal16v8as.ic13", 0x000, 0x117, CRC(45b8e164) SHA1(2e227ac5171ebf14e1d1f7214626ef4df0d5ae99) )
	ROM_LOAD( "cpu1_6-1_650_gal16v8as.ic7",  0x000, 0x117, CRC(b80b8485) SHA1(ea7f6312877cef31c93a8e02270565bd511f4aaa) )
	ROM_LOAD( "cpu2_c_cso_gal16v8as.ic10",   0x000, 0x117, CRC(0ebc7cd7) SHA1(95f0cba1f588920634b5f09b85197474201d9aed) )
	ROM_LOAD( "snd_9-2_9159_gal16v8as.ic42", 0x000, 0x117, NO_DUMP )
ROM_END

GAME( 1992, splashm,  splash, splashm, splashm, splashms_state, init_splashms, ROT0, "Gaelco / OMK Software", "Splash (Modular System, ver. 1.0, checksum 16EDD8FB, non North America, censored)",   MACHINE_IMPERFECT_GRAPHICS | MACHINE_IMPERFECT_SOUND )
GAME( 1992, splashma, splash, splashm, splashm, splashms_state, init_splashms, ROT0, "Gaelco / OMK Software", "Splash (Modular System, ver. 1.0, checksum A6C431A4, non North America, uncensored)", MACHINE_IMPERFECT_GRAPHICS | MACHINE_IMPERFECT_SOUND )
