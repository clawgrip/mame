// license:BSD-3-Clause
// copyright-holders:David Haywood

/*
    Big Karnak (Modular System) (checksum 1BD1EFB)

    this appears to be a different revision than the gaelco.cpp version, so could be original code, or
    based off one that is otherwise undumped.

    as with most of the 'Modular System' setups, the hardware is heavily modified from the original
    and consists of a multi-board stack in a cage, hence different driver.

    BOARDS

 Board 5-1
 ___________________________________________________________
 |                      ________________  ________________  |
 |                      |               | |               | |
 |                      | IC21-27C010   | | IC27-27C010   | |
 |                      |_______________| |_______________| |
 |           ::::::::   ________________  ________________  |
 |                      |               | |               | |
 | __                   | IC20-27C010   | | IC26-27C010   | |
 | | |                  |_______________| |_______________| |
 | | |     __________   ________________  ________________  |
 | | |     |_GAL16V8|   |               | |               | |
 | | |        5148      | IC19-27C010   | | IC25-27C010   | |
 | | |                  |_______________| |_______________| |
 | | |     __________   ________________  ________________  |
 | | |     |_GAL16V8|   |               | |               | |
 | | |        5248      | EMPTY         | | EMPTY         | |
 | | |                  |_______________| |_______________| |
 | | |      _________   ________________  ________________  |
 | | |      |74LS138N   |               | |               | |
 | | |                  | EMPTY         | | EMPTY         | |
 | | |                  |_______________| |_______________| |
 | | |________________  ________________                    |
 | |_||               | |               |                   |
 |    | IC6-27C010    | | IC15-27C101   |                   |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |                   |
 |    | IC5-27C010    | | IC14-27C101   |                   |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |     MODULAR       |
 |    | IC4-27C010    | | IC13-27C101   |    SYSTEM 2       |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |                   |
 |    | EMPTY         | | EMPTY         |                   |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |                   |
 |    | EMPTY         | | EMPTY         |                   |
 |    |_______________| |_______________|                   |
 |    ________________  ________________                    |
 |    |               | |               |                   |
 |    | EMPTY         | | EMPTY         |                   |
 |    |_______________| |_______________|                   |
 |__________________________________________________________|

 Board 7/4
 __________________________________________________________
 | _________  _________ __________  _________  _________ __|_
 | 74LS163AN| 74LS163AN||_GAL20V8_| |74F174N_| |74S189BN||   |
 |                         7648                          |   |
 | _________  _________  _________  _________  _________ |   |
 | |74LS157N| MC74F283N  |74LS273P| 74LS290B1| |74S189BN||   |
 |                                                       |   |
 | _________  _________  _________  _________  _________ |   |
 | |74LS393N| MC74F283N 74HCTLS373N 74LS298B1| |74S189BN||   |
 |                                                       |   |
 | _________  __________  _____________  _________       |   |
 | |74LS157N| |_PAL16V8_| |84256A-10L  | |74F174N_|      |   |
 | _________  __75FL____  |____________| _________       |   |
 | |74LS283B1 |74LS245B1| _____________  |74F174N_|      |   |
 | _________  __________  |84256A-10L  | __________      |   |
 | |74LS08N_| |74LS245B1| |____________| |74LS245B1|     |   |
 | _________  __________    __________   _________       |   |
 | |74LS20N | |74LS374B1|   |74LS273N |  |74LS74AN|      |   |
 |                                                       |   |
 | _________  __________    __________   _________       |___|
 | |74LS04N_| |PALCE16V8|   |74LS374N_|  |74LS157N|        |
 |               7348                                      |
 | _________  __________    __________   _________         |
 | |74LS00N_| |_GAL10V8_|   |74LS273N_| 74HCTLS367AN     __|_
 |               7248                                    |   |
 | _________  __________    __________   _________       |   |
 | |74LS08N_| |74LS273N |   |74LS374N_|  PALCE16V8|      |   |
 |                                          7448         |   |
 | _________  __________    __________   _________       |   |
 | |74LS74AN| |74LS273N_|   74LS367AN_|  |74LS373N|      |   |
 |                                                       |   |
 | _________  __________    __________   _________       |   |
 | |74LS32N_| |74LS374N_|   74LS367AN_| GAL20V8-25LP     |   |
 |                                          7148         |   |
 | _________  _________  _________  _________  _________ |   |
 | |74LS139N  |_74F74N_| |74LS157N| |74LS20N_| 74HCT373N |   |
 | _________  _________  _________  _________  _________ |   |
 | |74LS32N_| |74LS157N|  74HCT597E 74HCT597E 74HCT597E  |   |
 | _________  _________  _________  _________  _________ |   |
 | |74LS74AN| |74LS86B1| 74HCT597N  74HCT597N 74HCT597N  |   |
 | _________  _________  _________  _________  _________ |   |
 | |74LS74AN| |74LS86B1| 74HCT597N  74HCT597N  74HCT597N |   |
 |            _________  _________  _________  _________ |   |
 |            |74LS377N| 74HCT597N  74HCT597N  74HCT597N |   |
 |            _________  _________  _________  _________ |___|
 |            |74LS273N| 74LS169BN  74LS169BN  74LS169BN   |
 |_________________________________________________________|

Board 8
 __________________________________________________________________________________
 |           :::::::: <- Jumpers                                                   |
 |          ________________  ________________  ________________  ________________ |
 | _______  |               | |               | |               | |               ||
 | 74LS175N | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 |          |_______________| |_______________| |_______________| |_______________||
 | _______  ________________  ________________  ________________  ________________ |
 | 74LS175N |               | |               | |               | |               ||
 |          | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 | _______  |_______________| |_______________| |_______________| |_______________||
 | 74LS175N ________________  ________________  ________________  ________________ |
 |          |               | |               | |               | |               ||
 | _______  | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 | 74LS175N |_______________| |_______________| |_______________| |_______________||
 |          ________________  ________________  ________________  ________________ |
 | _______  |               | |               | |               | |               ||
 | 74LS175N | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 |          |_______________| |_______________| |_______________| |_______________||
 |          ________________  ________________  ________________  ________________ |
 | _______  |               | |               | |               | |               ||
 | 74LS175N | EMPTY         | | EMPTY         | | EMPTY         | | EMPTY         ||
 |          |_______________| |_______________| |_______________| |_______________||
 |          ________________  ________________  ________________  ________________ |
 | _______  |               | |               | |               | |               ||
 | 74LS175N | KA-814        | | KA-821        | | KA-829        | | KA-836        ||
 |          |_______________| |_______________| |_______________| |_______________||
 | _______  ________________  ________________  ________________  ________________ |
 | 74LS175N |               | |               | |               | |               ||
 |          | KA-815        | | KA-822        | | KA-830        | | KA-837        ||
 |          |_______________| |_______________| |_______________| |_______________||
 |  _____________________________      _______  _________________________________  |
 |__|                            |____74LS138N_|                                |__|
    |____________________________|             |________________________________|

 CPU Board 6/1
 _____________________________________________
 |             _______________              __|_
 |             |              |             |   |
 |             | 84256A-10L   |             |   |
 |             |______________|   _______   |   |
 |  _______   ________________    74LS138N  |   |
 | 74LS367AB1 |               |             |   |
 |            | EMPTY         |             |   |
 |            |_______________|             |   |
 |            ________________    _______   |   |
 |  _______   |               |   74LS245B1 |   |
 | 74LS367AB1 | IC20-27C010   |             |   |
 |            |_______________|             |   |
 |            ________________              |   |
 |  _______   |               |   _______   |   |
 | 74LS138N   | IC17-27C010   |   74LS374N  |   |
 |            |_______________|             |   |
 |             _______________              |   |
 |             |              |   _______   |___|
 |  _______    | 84256A-10L   |   74LS245B1   |
 |  GAL16V8    |______________|               |
 |    606     ________________                |
 |  _______   |               |   _______     |
 | 74LS174N   | EMPTY         |   74LS374N    |
 |            |_______________|               |
 |  _______   ________________                |
 |  PAL16V8   |               |               |
 |    648     | IC11-27C010   |   _______     |
 |            |_______________|   74LS138N    |
 |            ________________                |
 |            |               |               |
 |            | IC8-27C010    |               |
 |            |_______________|   _______     |
 | ___________________________    74LS32N     |
 | |                          |               |
 | |       TS68000CP12        |   _______     |
 | |__________________________|   74LS20N     |
 |                                            |
 |  _______   ______   _______    _______     |
 |  |74S74N    XTAL   74LS368AN   74LS132N    |
 |           24.000MHz                        |
 |____________________________________________|


Board 51/1
 _____________________________________________
 |                                            |
 | __________  ________ ________ ________     |
 | |TO SUB 51| 74LS299N 74LS169N AM2149-35DC  |
 | |_________| ________ ________ ________     |
 |             74LS169J 74LS169N AM2149-35DC  |
 |                                            |
 | __________  ________ ________ ________     |
 | |TO SUB 51| 74LS158N 74LS169B1 82S129N <- P0502
 | |_________| ________ ________ ________     |
 |             74LS299N 74LS169B1 74LS244N    |
 |                                            |
 | __________  ________ ________ ________     |
 | |TO SUB 51| 74LS299N 74LS169B1 74LS244N    |
 | |_________| ________ ________ ________     |
 |             |74LS20N AM2149-35DC 74LS298N  |
 |                                            |
 | __________  ________ ________ ________     |
 | |TO SUB 51| 74LS299N AM2149-35DC 74LS298N  |
 | |_________|                                |
 |   ________  ________ ________ ________  __ |
 |   74LS273N  74LS00B1 |74LS86N 74LS244N  | ||
 |                                         | ||
_|_  ________  ________ ________ ________  |T||
|  | 74LS08B1  74LS158N 74LS74B1 |74LS20B1 |O||
|  |                                       | ||
|  | ________  ________ ________ ________  |S||
IC48->PAL16R6A 74LS393N 74LS368A 74LS373B1 |U||
|  |                                       |B||
|  | ________  ________ ________ ________  | ||
|  | 74LS138PC 74LS283N |UM2148_||_EMPTY_| |5||
|  |                                       |1||
|  | ________  ________ ________ ________  | ||
|  | 74LS175B1 74LS283N |UM2148_| 74LS273P | ||
|  |                                       | ||
|  | ________  ________ ________ ________  |_||
|  | 74LS298N 74LS157B1 74LS157B1 74LS273P    |
|  |                                          |
|  | ________  ________ ________ ________     |
|  | 74LS158N 74LS169B1 74LS169B1 74LS245N    |
|__|                                          |
 |____________________________________________|

Sound Board 9/2
 ____________   _____________   _______________
 |           |_|_|_|_|_|_|_|_|_|             __|_
 |                PRE-JAMMA            XTAL  |   |
 | TDA2003                          28.00000 MHz |
 |           ________   ________   ________  |   |
 |           CNY 74-4   CNY 74-4   74LS368AN |   |
 |                                           |   |
 |           ________   ________   ________  |   |
 |           CNY 74-4   CNY 74-4  |74S112N|  |   |
 |                                           |   |
 |           ________   ________   ________  |   |
 |           74LS245N   74LS245B1  74LS298B1 |   |
 |                                           |   |
 | _______   ________   ________   ________  |   |
 | |LM324N   OKI M5205  |GAL16V8   74LS298B1 |   |
 |                        9148               |   |
 |           ________   ________   ________  |   |
 |           74LS157N   74LS267AN  74LS298B1 |   |
 | _______                                   |___|
 | |LM324N   ________   ________   ________    |
 |           74LS273N   74LS138N  |74LS32N|    |
 | D  74                                       |
 | I  LS  ______        ________   ________    |
 | P  24  Y3014B        74LS174N   74LS174N    |
 | S  5N                                       |
 |                      ________   ________    |
 | D  74                74LS245N  CXK5814P-35L |
 | I  LS  __________                           |
 | P  24  |  YM3812 |   ________   ________    |
 | S  5N  |_________|   74LS374N   74LS367AN   |
 | _______   _________                         |
 | 74LS138N UM6116K-3L  ________   ________    |
 |         ____________ 74LS74AN  |GAL20V8|    |
 | _______ |IC11-27C101             9248       |
 | GAL16V8 |__________| _______ _______ ______ |
 |   9348  ____________ 74LS32N 74LS393N 74LS163AN
 | _______ |IC6-27C512|                        |
 |74LS273N |__________| _______ _______ ______ |
 |         ____________74LS393N 74F74N 74LS163AN
 | _______ |Z0840006PSC                        |
 | 74LS08N |_Z80 CPU__| _______ _______   ____ |
 |                     74LS74AN 74LS368AN XTAL |
 |________________________________________16.000 MHz

*/


#include "emu.h"
#include "gaelco_ms_mod51.h"
#include "gaelco_ms_mod9.h"
#include "cpu/m68000/m68000.h"
#include "screen.h"
#include "speaker.h"
#include "tilemap.h"


namespace {

class bigkarnk_ms_state : public driver_device
{
public:
	bigkarnk_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_screen(*this, "screen"),
		m_gfxdecode(*this, "gfxdecode"),
		m_mod51(*this, "mod51"),
		m_mod9(*this, "mod9"),
		m_spriteram(*this, "spriteram"),
		m_videoram1(*this, "videoram1"),
		m_videoram2(*this, "videoram2"),
		m_videoram3(*this, "videoram3"),
		m_scrollregs(*this, "scrollregs")
	{ }

	void bigkarnkm(machine_config &config);
	void init_bigkarnkm();

protected:
	virtual void video_start() override ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
	required_device<screen_device> m_screen;

	required_device<gfxdecode_device> m_gfxdecode;
	required_device<gaelco_ms_mod51_device> m_mod51;
	required_device<gaelco_ms_mod9_device> m_mod9;
	required_shared_ptr<uint16_t> m_spriteram;
	required_shared_ptr<uint16_t> m_videoram1;
	required_shared_ptr<uint16_t> m_videoram2;
	required_shared_ptr<uint16_t> m_videoram3;
	required_shared_ptr<uint16_t> m_scrollregs;

	u8 sprite_byte(offs_t offset) const { return m_spriteram[offset >> 1] >> (BIT(offset, 0) ? 0 : 8); }
	void sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void bigkarnkm_map(address_map &map) ATTR_COLD;

	uint16_t vram1_r(offs_t offset, uint16_t mem_mask);
	uint16_t vram2_r(offs_t offset, uint16_t mem_mask);
	uint16_t vram3_r(offs_t offset, uint16_t mem_mask);

	void vram1_w(offs_t offset, uint16_t data, uint16_t mem_mask);
	void vram2_w(offs_t offset, uint16_t data, uint16_t mem_mask);
	void vram3_w(offs_t offset, uint16_t data, uint16_t mem_mask);

	TILE_GET_INFO_MEMBER(get_tile_info_tilemap1);
	TILE_GET_INFO_MEMBER(get_tile_info_tilemap2);
	TILE_GET_INFO_MEMBER(get_tile_info_tilemap3);

	tilemap_t *m_bg_tilemap1 = nullptr;
	tilemap_t *m_bg_tilemap2 = nullptr;
	tilemap_t *m_bg_tilemap3 = nullptr;

	void descramble_16x16tiles(uint8_t* src, int len);
};

TILE_GET_INFO_MEMBER(bigkarnk_ms_state::get_tile_info_tilemap1)
{
	int tile = m_videoram1[tile_index*2];
	int attr = m_videoram1[(tile_index*2)+1] & 0x1f;
//  int fx = (m_videoram1[(tile_index*2)+1] & 0xc0)>>6;

	// we rearranged the tile order for the 16x16 deode, so have to swap back here
	tile = ((tile & 0x300) >> 8) | ((tile & 0xff) << 2) | (tile & 0xfc00);

	tileinfo.set(1,tile,attr,0);
}


TILE_GET_INFO_MEMBER(bigkarnk_ms_state::get_tile_info_tilemap2)
{
	int tile = m_videoram2[tile_index*2];

	tile &= 0x1fff;

	int attr = m_videoram2[(tile_index*2)+1] & 0xff;
	//int fx = (m_videoram2[(tile_index*2)+1] & 0xc0)>>6;

	int col = attr & 0x1f;


	tileinfo.set(0,tile,col,0);
}

TILE_GET_INFO_MEMBER(bigkarnk_ms_state::get_tile_info_tilemap3)
{
	int tile = m_videoram3[tile_index*2];

	tile &= 0x1fff;

	int attr = m_videoram3[(tile_index*2)+1] & 0xff;
	//int fx = (m_videoram3[(tile_index*2)+1] & 0xc0)>>6;

	int col = attr & 0x1f;

	tileinfo.set(0,tile,col,0);
}

uint16_t bigkarnk_ms_state::vram1_r(offs_t offset, uint16_t mem_mask)
{
	return m_videoram1[offset];
}

void bigkarnk_ms_state::vram1_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_videoram1[offset]);
	m_bg_tilemap1->mark_tile_dirty(offset/2);
}

uint16_t bigkarnk_ms_state::vram2_r(offs_t offset, uint16_t mem_mask)
{
	return m_videoram2[offset];
}

void bigkarnk_ms_state::vram2_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_videoram2[offset]);
	m_bg_tilemap2->mark_tile_dirty(offset/2);
}


uint16_t bigkarnk_ms_state::vram3_r(offs_t offset, uint16_t mem_mask)
{
	return m_videoram3[offset];
}

void bigkarnk_ms_state::vram3_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_videoram3[offset]);
	m_bg_tilemap3->mark_tile_dirty(offset/2);
}



void bigkarnk_ms_state::video_start()
{
	m_bg_tilemap1 = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(bigkarnk_ms_state::get_tile_info_tilemap1)), TILEMAP_SCAN_ROWS,  8,  8, 64, 32);
	m_bg_tilemap2 = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(bigkarnk_ms_state::get_tile_info_tilemap2)), TILEMAP_SCAN_ROWS,  16,  16, 32, 32);
	m_bg_tilemap3 = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(bigkarnk_ms_state::get_tile_info_tilemap3)), TILEMAP_SCAN_ROWS,  16,  16, 32, 32);

	m_bg_tilemap1->set_transparent_pen(15);
	m_bg_tilemap2->set_transparent_pen(0);
}



void bigkarnk_ms_state::bigkarnkm_map(address_map &map)
{
	map(0x000000, 0x07ffff).rom();

	map(0x080000, 0x081fff).rw(FUNC(bigkarnk_ms_state::vram1_r), FUNC(bigkarnk_ms_state::vram1_w)).share("videoram1");

	map(0x090000, 0x090fff).rw(FUNC(bigkarnk_ms_state::vram2_r), FUNC(bigkarnk_ms_state::vram2_w)).share("videoram2");
	map(0x091000, 0x091fff).ram();

	map(0x0a0000, 0x0a0fff).rw(FUNC(bigkarnk_ms_state::vram3_r), FUNC(bigkarnk_ms_state::vram3_w)).share("videoram3");

	map(0x0c0000, 0x0c000f).ram().share("scrollregs");

	map(0x100000, 0x100fff).ram().share("spriteram");

	map(0x102000, 0x103fff).ram();

	map(0x200000, 0x200fff).rw(m_mod9, FUNC(gaelco_ms_mod9_device_base::palette_r), FUNC(gaelco_ms_mod9_device_base::palette_w));

	map(0x400000, 0x40000f).rw(m_mod9, FUNC(gaelco_ms_mod9_device_base::io16_r), FUNC(gaelco_ms_mod9_device_base::io16_w));

	map(0xff0000, 0xffffff).ram();
}


void bigkarnk_ms_state::sprite_cb(u8 attr, u8 color, u32 &code, u32 &palette, bool &flipx, bool &flipy, bool &x8)
{
	// code bits 8-13 in attribute bits 0-5, flip X in bit 6, flip Y in bit 7 and X bit 8 in colour byte bit 7
	code |= (attr & 0x3f) << 8;
	flipx = BIT(attr, 6);
	flipy = BIT(attr, 7);
	x8 = BIT(color, 7);
}

uint32_t bigkarnk_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0, cliprect);

	m_bg_tilemap3->set_scrollx(0, 112-(m_scrollregs[6]-0x4));
	m_bg_tilemap3->set_scrolly(0, -m_scrollregs[7]);

	m_bg_tilemap2->set_scrollx(0, 112-(m_scrollregs[0]-0x2));
	m_bg_tilemap2->set_scrolly(0, -m_scrollregs[1]);

	m_bg_tilemap1->set_scrollx(0, 112-(m_scrollregs[2]));
	m_bg_tilemap1->set_scrolly(0, -m_scrollregs[3]);

	m_bg_tilemap3->draw(screen, bitmap, cliprect, 0, 0);
	m_bg_tilemap2->draw(screen, bitmap, cliprect, 0, 0);

	m_mod51->draw(bitmap, cliprect, *m_gfxdecode->gfx(2));

	m_bg_tilemap1->draw(screen, bitmap, cliprect, 0, 0);

	return 0;
}

static INPUT_PORTS_START( bigkarnkm )
	PORT_INCLUDE( gaelco_ms_mod9 )

	// on the MOD 9/2 board
	PORT_START("DSW1")
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:8,7,6,5")
	PORT_DIPSETTING(      0x0007, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, "Free Play (if Coin B too)" )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:4,3,2,1")
	PORT_DIPSETTING(      0x0070, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, "Free Play (if Coin A too)" )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x0007, 0x0006, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:8,7,6")
	PORT_DIPSETTING(      0x0007, "0" )
	PORT_DIPSETTING(      0x0006, "1" )
	PORT_DIPSETTING(      0x0005, "2" )
	PORT_DIPSETTING(      0x0004, "3" )
	PORT_DIPSETTING(      0x0003, "4" )
	PORT_DIPSETTING(      0x0002, "5" )
	PORT_DIPSETTING(      0x0001, "6" )
	PORT_DIPSETTING(      0x0000, "7" )
	PORT_DIPNAME( 0x0018, 0x0008, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW2:5,4")
	PORT_DIPSETTING(      0x0018, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0008, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Impact" ) PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_SERVICE_DIPLOC(  0x0080, IP_ACTIVE_LOW, "SW2:1" )
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


static GFXDECODE_START( gfx_bigkarnk_ms )
	GFXDECODE_ENTRY( "bgtile", 0, tiles16x16x4_layout, 0, 32 )
	GFXDECODE_ENTRY( "bgtile", 0, tiles8x8x4_layout, 0, 32 )
	GFXDECODE_ENTRY( "sprites", 0, tiles16x16x4_layout, 0x200, 32 )
GFXDECODE_END


void bigkarnk_ms_state::bigkarnkm(machine_config &config)
{
	// basic machine hardware
	M68000(config, m_maincpu, 12_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &bigkarnk_ms_state::bigkarnkm_map);

	// video hardware
	SCREEN(config, m_screen);
	m_screen->set_raw(28_MHz_XTAL / 4, 448, 0, 320, 264, 0, 240);
	m_screen->set_screen_update(FUNC(bigkarnk_ms_state::screen_update));
	m_screen->set_palette(m_mod9);

	GFXDECODE(config, m_gfxdecode, m_mod9, gfx_bigkarnk_ms);

	SPEAKER(config, "mono").front_center();

	GAELCO_MS_MOD9(config, m_mod9, 28_MHz_XTAL);
	m_mod9->set_screen(m_screen);
	m_mod9->irq_callback().set_inputline(m_maincpu, 6);
	m_mod9->in_callback<0>().set_ioport("IN0");
	m_mod9->in_callback<1>().set_ioport("IN1");
	m_mod9->in_callback<2>().set_ioport("DSW1");
	m_mod9->in_callback<3>().set_ioport("DSW2");
	m_mod9->add_route(ALL_OUTPUTS, "mono", 1.0);

	GAELCO_MS_MOD51(config, m_mod51);
	m_mod51->set_timing(m_mod9);
	m_mod51->set_list(256, 0x400, true);
	m_mod51->list_r().set([this] (offs_t offset) { return sprite_byte(offset); });
	m_mod51->set_sprite_callback(FUNC(bigkarnk_ms_state::sprite_cb));
}


// reorganize graphics into something we can decode with a single pass
void bigkarnk_ms_state::descramble_16x16tiles(uint8_t* src, int len)
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

void bigkarnk_ms_state::init_bigkarnkm()
{
	descramble_16x16tiles(memregion("bgtile")->base(), memregion("bgtile")->bytes());
}



ROM_START( bigkarnkm )
	ROM_REGION( 0x080000, "maincpu", 0 )    // 68000 code
	ROM_LOAD16_BYTE( "cpu_ka_6.ic8",  0x000001, 0x020000, CRC(ab71c1d3) SHA1(3174f1c68e4aa5b6053b118da1fed1f4001193b0) )
	ROM_LOAD16_BYTE( "cpu_ka_6.ic17", 0x000000, 0x020000, CRC(9f5c0dac) SHA1(a8089f58e34d7ba581303f7cf819297d21867a6a) )
	ROM_LOAD16_BYTE( "cpu_ka_6.ic11", 0x040001, 0x020000, CRC(30674ef3) SHA1(d1b29337068ed7323c104a48de593c9ac4668e66) )
	ROM_LOAD16_BYTE( "cpu_ka_6.ic20", 0x040000, 0x020000, CRC(332d6dea) SHA1(cd7e402642f57c12cb7405c49b75bfaa0d104421) )

	ROM_REGION( 0x30000, "mod9", 0 ) // on MOD 9/2 board, sound program and samples
	ROM_LOAD( "snd_ka.ic6",           0x000000, 0x010000, CRC(48a66be8) SHA1(0ca8e4ef5b5e257d56afda6946c5f2a0712917a3) )
	ROM_LOAD( "snd_ka.ic11",          0x010000, 0x020000, CRC(8e53a6b8) SHA1(5082bbcb042216a6d58c654a52c98d75df700ac8) )

	ROM_REGION( 0x180000, "sprites", ROMREGION_ERASEFF | ROMREGION_INVERT ) // sprites (same rom subboard type as galpanic_ms.cpp)
	ROM_LOAD32_BYTE( "5_ka.ic4",      0x080003, 0x020000, CRC(2bee07ea) SHA1(afd8769955314768db894e4e98f65422fc0dbb4f) )
	ROM_LOAD32_BYTE( "5_ka.ic13",     0x080002, 0x020000, CRC(d55e3024) SHA1(71c84a76b08f8983f65ac4b99430eeb30dc3f8ea) )
	ROM_LOAD32_BYTE( "5_ka.ic19",     0x080001, 0x020000, CRC(fc682c21) SHA1(c3fa9907fbe276bc4b74b79dda52713e702e441c) )
	ROM_LOAD32_BYTE( "5_ka.ic25",     0x080000, 0x020000, CRC(1157b739) SHA1(fdea10f808f258409e19e41dedfb3d4699e7daa2) )

	ROM_LOAD32_BYTE( "5_ka.ic5",      0x100003, 0x020000, CRC(507056f7) SHA1(e754c803f85862a37d2a48be6554ff5bc4128b4d) )
	ROM_LOAD32_BYTE( "5_ka.ic14",     0x100002, 0x020000, CRC(ef936e76) SHA1(ed93b04a45c38c0fa7333f182beba33fafe17f38) )
	ROM_LOAD32_BYTE( "5_ka.ic20",     0x100001, 0x020000, CRC(38854cd6) SHA1(b32efc1c621a9d8559c294f7431c219a05a37db6) )
	ROM_LOAD32_BYTE( "5_ka.ic26",     0x100000, 0x020000, CRC(3f63c4ed) SHA1(e0dd3ec27e7aa0b7db1587e83d20d1b9333ca405) )

	ROM_LOAD32_BYTE( "5_ka.ic6",      0x000003, 0x020000, CRC(2fdbc484) SHA1(6e8ac1a8bde8189b7ebf32c59185425c512ab911) )
	ROM_LOAD32_BYTE( "5_ka.ic15",     0x000002, 0x020000, CRC(802128e4) SHA1(20cfdf28aa7ada404ceca236c6eb554dcaa8e633) )
	ROM_LOAD32_BYTE( "5_ka.ic21",     0x000001, 0x020000, CRC(5ccc0f99) SHA1(ae2b2d4b2aa77a099ad2711032e6a05ab52789b9) )
	ROM_LOAD32_BYTE( "5_ka.ic27",     0x000000, 0x020000, CRC(55509d96) SHA1(ddd064695ca7e8c2377f13484e385bf7ea7df610) )

	ROM_REGION( 0x100000, "bgtile", 0 )
	ROM_LOAD32_BYTE( "8_ka_815.ic15", 0x000003, 0x020000, CRC(59d79b33) SHA1(70b9c60a72e517ac70f807c918f0ad4dd6c98f98) )
	ROM_LOAD32_BYTE( "8_ka_822.ic22", 0x000002, 0x020000, CRC(12fc89c0) SHA1(883144d0c453cd8f829b2209d9a8028b7f87d0d5) )
	ROM_LOAD32_BYTE( "8_ka_830.ic30", 0x000001, 0x020000, CRC(9904ae87) SHA1(5df3b35185c53a64c0647d297a19b9c013a3b3c2) )
	ROM_LOAD32_BYTE( "8_ka_837.ic37", 0x000000, 0x020000, CRC(f475eaa7) SHA1(8e5c7f0231d7f84bc377b756b99d055a4791e3bf) )
	ROM_LOAD32_BYTE( "8_ka_814.ic14", 0x080003, 0x020000, CRC(50e6cab6) SHA1(5af8b27f35a59611484ea35a2883b1e59d5c7517) )
	ROM_LOAD32_BYTE( "8_ka_821.ic21", 0x080002, 0x020000, CRC(90c1d93e) SHA1(581a1e2f30e8b467c8d8f5c8e528c78c0c3904f2) )
	ROM_LOAD32_BYTE( "8_ka_829.ic29", 0x080001, 0x020000, CRC(8c5df0ec) SHA1(15a5b847d6d035f27300435a03bd254dd9b3f99c) )
	ROM_LOAD32_BYTE( "8_ka_836.ic36", 0x080000, 0x020000, CRC(43de75db) SHA1(419e7702d17c52365addb8bfda582e916762ead5) )

	ROM_REGION( 0x157, "mod9:timing", 0 ) // on MOD 9/2 board
	ROM_LOAD( "snd_9248_gal20v8-25lp.ic18", 0x000, 0x157, CRC(13123ce6) SHA1(85b012d706021963899ad3bc6d29fbe50d8433ab) )

	ROM_REGION( 0x157, "plds", 0 )
	ROM_LOAD( "51_p0503_pal16r6acn.ic48",     0x000, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) ) // matches one of the Euro League Modular System PALs
	ROM_LOAD( "5_5148_gal16v8-25lnc.ic9",     0x000, 0x117, CRC(86f8b803) SHA1(8dc5117344c95a81ec6f856e5063f521dc428f19) )
	ROM_LOAD( "5_5248_gal16v8-25lnc.ic8",     0x000, 0x117, CRC(fff0d91b) SHA1(e8e60087af4f4c1c53a63956c5ff8af917b1a293) )
	ROM_LOAD( "7_75flv_gal16v8-25hb1.ic59",   0x000, 0x117, CRC(b3e2f5c5) SHA1(d03fbcb28818c8a29ed42369ce4a6734f2a87b4f) )
	ROM_LOAD( "7_7148_gal20v8-25lp.ic7",      0x000, 0x157, CRC(74d831ce) SHA1(ad27b516f188a20241791d4c40db8d2f7688469c) )
	ROM_LOAD( "7_7248_gal20v8-25lp.ic54",     0x000, 0x157, CRC(77d7ba3b) SHA1(3653b5405b42f16c9250bc371a313f0a0bfacf68) )
	ROM_LOAD( "7_7348_gal16v8-25hb1.ic55",    0x000, 0x117, CRC(64b5ff93) SHA1(99c54950c1f4dbe541205cd32a494f4c4f630767) )
	ROM_LOAD( "7_7448_gal16v8-25hb1.ic9",     0x000, 0x117, CRC(e3733d5e) SHA1(a19613fc4ccefb0fe8ce0d403b55e48295bfef9c) )
	ROM_LOAD( "7_7648_gal20v8-25lp.ic44",     0x000, 0x157, CRC(e4f67764) SHA1(93e28584a0acac4c9cb5e6844e6caa316407c91b) )
	ROM_LOAD( "cpu_606_gal16v8-25hb1.ic13",   0x000, 0x117, CRC(6fc7e412) SHA1(b9512d597bb9cec765b8caf26a51e7e4ed5d07b3) )
	ROM_LOAD( "cpu_648_gal16v8-25ln.ic7",     0x000, 0x117, CRC(4b415042) SHA1(4804558a5f5459d5f880fd1979c7fce8d5486beb) )
	ROM_LOAD( "snd_9148_gal16v8-25hb1.ic142", 0x000, 0x117, CRC(65e89f3d) SHA1(58f392598094547896ed7871626282136f00a120) )
ROM_END

} // anonymous namespace


GAME( 1991, bigkarnkm,  bigkarnk,  bigkarnkm,  bigkarnkm,  bigkarnk_ms_state, init_bigkarnkm, ROT0, "Gaelco", "Big Karnak (Modular System)", MACHINE_NOT_WORKING )
