// license:BSD-3-Clause
// copyright-holders:David Haywood
/*
    Euro League (Gaelco bootleg, Modular System)

    Modular System is a stack of boards in a cage, there are other games on this 'system'.
    It shares the weird tile decoding with the World Cup '90 bootlegs in wc90b.cpp.

    Gaelco requested the registry of the "Euro League" trademark on 1990, and it was a Gaelco
    protected trademark (in Spain) until 1999 (they paid a 5-year renew in 1994):
    https://www.patentes-y-marcas.com/marca/euro-league-m1546246
*/

#include "emu.h"
#include "wc90b.h"

#include "cpu/z80/z80.h"
#include "machine/gen_latch.h"
#include "sound/ymopn.h"

#include "speaker.h"


namespace {

class euroleag_ms_state : public wc90b_state
{
public:
	euroleag_ms_state(const machine_config &mconfig, device_type type, const char *tag) :
		wc90b_state(mconfig, type, tag),
		m_bgscroll(*this, "bgscroll")
	{ }

	void euroleagm(machine_config &config) ATTR_COLD;

protected:
	virtual uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect) override;
	virtual void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect, int priority) override;

private:
	required_shared_ptr<uint8_t> m_bgscroll;

	void master_irq_ack_w(uint8_t data);

	void main_map(address_map &map) ATTR_COLD;
};


void euroleag_ms_state::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect, int priority)
{
	// draw all visible sprites of specified priority

	// entry at start of RAM might not be a sprite
	for (int offs = 0x200 - 4 ; offs >= 4 ; offs -= 4)
	{
		if (((m_spriteram[offs + 3] >> 4) & 1) == priority)
		{
			// this is wrong

			// 0      bbbb bbbb   b = tile lower
			// 1      yyyy yyyy
			// 2      xxxx xxxx
			// 3      ffXP cccc   f = flip bits, P = priority (inverted vs. other bootlegs) X = X high?, c = tile upper
			// 0x200  ---- -ppp   p = palette

			int tilehigh = (m_spriteram[offs + 3] & 0x0f) << 8;
			int attr = (m_spriteram[offs + 3] & 0xf0) >> 4;

			int tilelow = m_spriteram[offs + 0];
			int flags = m_spriteram[offs + 0x200];

			tilehigh += tilelow;

			int sx = m_spriteram[offs + 2];
			if (!(attr & 0x02)) sx -= 0x0100;

			int sy = 240 - m_spriteram[offs + 1];

			m_gfxdecode->gfx(2)->transpen(bitmap, cliprect, tilehigh,
					(flags & 0x7) | 8, // color - palettes 0x0 - 0x7 never written?
					attr & 4,   // flipx
					attr & 8,   // flipy
					sx,
					sy, 15);
		}
	}
}

uint32_t euroleag_ms_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// the code to write / clear tilemaps for fb and tx layers has been specifically modified to avoid writing to the last 4 bytes
	// and the game instead writes scroll values there instead, there is no code to copy from there, so it looks like these are the scroll regs

	// each of the 3 layer has its own PCB, all PCBs look identical, so why does handling differ slightly?

	int fg_scrollx = ((m_fgvideoram[0xffc]) | (m_fgvideoram[0xffd] << 8)) + 33;
	int fg_scrolly = ((m_fgvideoram[0xffe]) | (m_fgvideoram[0xfff] << 8)) + 1;
	int bg_scrollx = ((m_bgscroll[0xf00]) | (m_bgscroll[0xf01] << 8)) + 33;
	int bg_scrolly = ((m_bgscroll[0xf02]) | (m_bgscroll[0xf03] << 8)) + 1;
	int tx_scrollx = ((m_txvideoram[0xffc]) | (m_txvideoram[0xffd] << 8)) + 33;
	int tx_scrolly = ((m_txvideoram[0xffe]) | (m_txvideoram[0xfff] << 8)) + 1;

	m_bg_tilemap->set_scrollx(0, bg_scrollx);
	m_bg_tilemap->set_scrolly(0, bg_scrolly);
	m_fg_tilemap->set_scrollx(0, fg_scrollx);
	m_fg_tilemap->set_scrolly(0, fg_scrolly);
	m_tx_tilemap->set_scrollx(0, tx_scrollx);
	m_tx_tilemap->set_scrolly(0, tx_scrolly);

	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	draw_sprites(bitmap, cliprect, 1);
	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	m_tx_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	draw_sprites(bitmap, cliprect, 0);
	return 0;
}


void euroleag_ms_state::master_irq_ack_w(uint8_t data)
{
	// this seems to be write based instead of read based
	m_maincpu->set_input_line(0, CLEAR_LINE);
}

void euroleag_ms_state::main_map(address_map &map)
{
	map(0x0000, 0x7fff).rom();
	map(0x8000, 0x9fff).ram(); // Main RAM
	map(0xa000, 0xafff).ram().w(FUNC(euroleag_ms_state::fgvideoram_w)).share(m_fgvideoram);
	map(0xc000, 0xcfff).ram().w(FUNC(euroleag_ms_state::bgvideoram_w)).share(m_bgvideoram);
	map(0xd000, 0xdfff).ram().share(m_bgscroll); // there are a bunch of read / write accesses in here (is it meant to mirror the bgram? - bg scroll regs are at df00 - df03
	map(0xe000, 0xefff).ram().w(FUNC(euroleag_ms_state::txvideoram_w)).share(m_txvideoram);
	map(0xf000, 0xf7ff).bankr(m_mainbank);
	map(0xf800, 0xfbff).ram().share("main_sub");
	map(0xfc00, 0xfc00).w(FUNC(euroleag_ms_state::bankswitch_w));
	map(0xfd00, 0xfd00).portr("P1");
	map(0xfd02, 0xfd02).portr("P2");
	map(0xfd06, 0xfd06).portr("DSW1");
	map(0xfd08, 0xfd08).portr("DSW2");
	map(0xfd0c, 0xfd0c).w(FUNC(euroleag_ms_state::master_irq_ack_w));
	map(0xfd0e, 0xfd0e).w("soundlatch", FUNC(generic_latch_8_device::write));
}


void euroleag_ms_state::euroleagm(machine_config &config)
{
	// main board XTAL near Z80s is 16 MHz here
	// sound board has 20 MHz and 16 MHz XTALs
	// DSWs are on the sound board near the YM2203Cs
	// use is guessed

	// basic machine hardware
	Z80(config, m_maincpu, 16_MHz_XTAL / 2);
	m_maincpu->set_addrmap(AS_PROGRAM, &euroleag_ms_state::main_map);
	m_maincpu->set_vblank_int("screen", FUNC(euroleag_ms_state::irq0_line_assert));

	Z80(config, m_subcpu, 16_MHz_XTAL / 2);
	m_subcpu->set_addrmap(AS_PROGRAM, &euroleag_ms_state::sub_map);
	m_subcpu->set_vblank_int("screen", FUNC(euroleag_ms_state::irq0_line_assert));

	Z80(config, m_audiocpu, 20_MHz_XTAL / 4);
	m_audiocpu->set_addrmap(AS_PROGRAM, &euroleag_ms_state::sound_map);
	// IRQs are triggered by the main CPU

	// video hardware
	screen_device &screen(SCREEN(config, "screen"));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(0));
	screen.set_size(32*8, 32*8);
	screen.set_visarea(0*8, 32*8-1, 2*8, 30*8-1);
	screen.set_screen_update(FUNC(euroleag_ms_state::screen_update));
	screen.set_palette(m_palette);

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_wc90b);
	PALETTE(config, m_palette).set_format(palette_device::xBRG_444, 1024).set_endianness(ENDIANNESS_BIG);

	// sound hardware
	SPEAKER(config, "mono").front_center();

	GENERIC_LATCH_8(config, "soundlatch").data_pending_callback().set_inputline(m_audiocpu, 0);

	YM2203(config, "ymsnd1", 16_MHz_XTAL / 4).add_route(ALL_OUTPUTS, "mono", 0.40); // YM2203C
	YM2203(config, "ymsnd2", 16_MHz_XTAL / 4).add_route(ALL_OUTPUTS, "mono", 0.40); // YM2203C

	MSM5205(config, m_msm, 384_kHz_XTAL);
	m_msm->vck_legacy_callback().set(FUNC(euroleag_ms_state::adpcm_int)); // interrupt function
	m_msm->set_prescaler_selector(msm5205_device::S96_4B);  // 4KHz 4-bit
	m_msm->add_route(ALL_OUTPUTS, "mono", 0.20);
}


ROM_START( euroleagm )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD( "3z-1_fu301.ic17", 0x00000, 0x10000, CRC(74acc161) SHA1(d8660dd6d05164df4a66125c68627e955b35bef3) )  // c000-ffff is not used
	ROM_LOAD( "3z-1_fu302.ic15", 0x10000, 0x10000, CRC(f54ff17a) SHA1(a19850fc28a5a0da20795a5cc6b56d9c16554bce) )  // banked at f000-f7ff

	ROM_REGION( 0x20000, "sub", 0 )
	ROM_LOAD( "3z-1_fu303.ic19", 0x00000, 0x10000, CRC(348195fa) SHA1(41c59e38ec4ba4f3c2185dd32dbf4ea0318ab375) )  // c000-ffff is not used
	ROM_LOAD( "3z-1_fu304.ic16", 0x10000, 0x10000, CRC(9e421c4b) SHA1(e23a1f1d5d1e960696f45df653869712eb889839) )  // banked at f000-f7ff

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "system2_fu101.ic6", 0x00000, 0x10000, CRC(712566ca) SHA1(9e46f9d449ff549b7a6d82283d8f903189b058e7) )

	ROM_REGION( 0x20000, "chargfx", ROMREGION_INVERT ) // 1ST AND 2ND HALF IDENTICAL (all ROMs in this region)
	ROM_LOAD( "r1_fu404.ic14", 0x00000, 0x8000, CRC(1659529a) SHA1(ad7a2bab43922871ce7081d46efdeefa9db5e45f) )
	ROM_LOAD( "r1_fu403.ic15", 0x08000, 0x8000, CRC(f1effd53) SHA1(0299678e1716a92cd6ecc67d28dd95422f84d15d) )
	ROM_LOAD( "r1_fu402.ic16", 0x10000, 0x8000, CRC(da7f3ce3) SHA1(80ae95208a807e3866560ad5f192951d33f1da34) )
	ROM_LOAD( "r1_fu401.ic17", 0x18000, 0x8000, CRC(b93201e0) SHA1(edc8dbab15d901aab678244811d8e08d4cb974f2) )

	ROM_REGION( 0x080000, "tilegfx", ROMREGION_INVERT )
	ROM_LOAD( "r2_fu4a01.ic17", 0x060000, 0x10000, CRC(1bcdd99d) SHA1(e37a8308eff79b04ce3da75fec2fffe4021c70d3) )  // fg layer
	ROM_LOAD( "r2_fu4a02.ic16", 0x040000, 0x10000, CRC(e625de0c) SHA1(ca5a00ea27a5107d2957a1637b260d5aa1ff5563) )
	ROM_LOAD( "r2_fu4a03.ic15", 0x020000, 0x10000, CRC(995df90d) SHA1(121874c807001bf65a9cec90dd30212f1889a398) )
	ROM_LOAD( "r2_fu4a04.ic14", 0x000000, 0x10000, CRC(44031f34) SHA1(0b417c6cc1ed029394bcdce80e81262d776105b7) )
	ROM_LOAD( "r3_fu4b01.ic17", 0x070000, 0x10000, CRC(adc7feda) SHA1(ff82e973599f13c76ecaf3c027a2967468cd9e72) )  // bg layer
	ROM_LOAD( "r3_fu4b02.ic16", 0x050000, 0x10000, CRC(1e5c0fda) SHA1(74163098810ff079467bad77796898426acbfac5) )
	ROM_LOAD( "r3_fu4b03.ic15", 0x030000, 0x10000, CRC(cc9dab0b) SHA1(7e5735e57b45de1b344455fa5a047ae17933de27) )
	ROM_LOAD( "r3_fu4b04.ic14", 0x010000, 0x10000, CRC(d54704d2) SHA1(7d4ec0120c3516a88abbe687e8916b44bffb7bcd) )

	ROM_REGION( 0x080000, "spritegfx", ROMREGION_INVERT )
	ROM_LOAD( "r4_fu504.bin",  0x000000, 0x10000, CRC(516b6c09) SHA1(9d02514dece864b087f67886009ce54bd51b5575) )
	ROM_LOAD( "r4_fu5a04.bin", 0x010000, 0x10000, CRC(f36390a9) SHA1(e5ea36e91b3ced068281524ee79d0432f489715c) )
	ROM_LOAD( "r4_fu503.bin",  0x020000, 0x10000, CRC(0da825f9) SHA1(cfba0c85fc767726c1d63f87468335d1c2f1eed8) )
	ROM_LOAD( "r4_fu5a03.bin", 0x030000, 0x10000, CRC(228429d8) SHA1(3b2dbea53807929c24d593c469a83172f7747f66) )
	ROM_LOAD( "r4_fu502.bin",  0x040000, 0x10000, CRC(26371c18) SHA1(0887041d86dc9f19dad264ae27dc56fb89ac3265) )
	ROM_LOAD( "r4_fu5a02.bin", 0x050000, 0x10000, CRC(75aa9b86) SHA1(0c221bd2e8a5472bb0e515f27fb72b0c8e8c0ca4) )
	ROM_LOAD( "r4_fu501.bin",  0x060000, 0x10000, CRC(d5a60096) SHA1(a8e351a4b020b4fc2b2cb7d3f0fdfb43fc44d7d9) )
	ROM_LOAD( "r4_fu5a01.bin", 0x070000, 0x10000, CRC(36bbf467) SHA1(627b5847ffb098c92edfd58c25391799f3b209e0) )

	ROM_REGION( 0x100, "prom", ROMREGION_ERASEFF )
	ROM_LOAD( "r4_p0502_82s129.ic10",      0x000, 0x100, CRC(15085e44) SHA1(646e7100fcb112594023cf02be036bd3d42cc13c) )

	ROM_REGION( 0x157, "plds", ROMREGION_ERASEFF )
	ROM_LOAD( "r2_p0403_pal16r8a.ic29",    0x000, 0x104, CRC(506156cc) SHA1(5560671fc2c9872ed28620491af5dc486909fc6e) )
	ROM_LOAD( "r3_p0403_pal16r8a.ic29",    0x000, 0x104, CRC(d8c6ac25) SHA1(d6184e491313ff8da5b1ce60ffe8ef517716807c) )
	ROM_LOAD( "r4_p0503_pal16r6.ic46",     0x000, 0x104, CRC(07eb86d2) SHA1(482eb325df5bc60353bac85412cf45429cd03c6d) )
	ROM_LOAD( "3z-1_3138_gal16v8.ic22",    0x000, 0x117, CRC(909dab7b) SHA1(e9f4bb239fa7843743e85e236ae0c744784a3b3f) ) // Same as Gaelco Goldart ?
	ROM_LOAD( "3z-1_3238_gal16v8.ic24",    0x000, 0x117, CRC(e9e538d9) SHA1(9ea73a903a06111843fe64ae55cb29ee88803334) ) // Same as Gaelco Goldart ?
	ROM_LOAD( "r1_403_gal16v8.ic29",       0x000, 0x117, CRC(c136de93) SHA1(116f6d3b456d20621ab07a005c1421f57569915c) )
	ROM_LOAD( "system2_9138_gal16v8.ic42", 0x000, 0x117, CRC(bd9ad8c3) SHA1(50e00b0cf7d075f9daed0338bc336a74caa3b66b) )
	ROM_LOAD( "system2_9238_gal20v8.ic18", 0x000, 0x157, CRC(dd571a59) SHA1(ef2c7b33922dd79513a4fbe6f04a2f2c2c795ada) )
	ROM_LOAD( "system2_9338_gal16v8.ic10", 0x000, 0x117, CRC(54b4160f) SHA1(0156a2eda97a9d8e0adb3a3795f6ed547c6e06fc) )
ROM_END

} // anonymous namespace


GAME( 1989, euroleagm, twcup90, euroleagm, wc90b, euroleag_ms_state, init_wc90b, ROT0, "bootleg (Gaelco / Ervisa)", "Euro League (Gaelco bootleg, Modular System)", MACHINE_IMPERFECT_SOUND )
