// license:BSD-3-Clause
// copyright-holders: Tomás García-Merás Capote (ClawGrip)
/*******************************************************************************

 Gaelco "Adaptador de créditos 4x4" (4x4 credit adapter)

 A small board that goes between the coin mechanisms and start buttons of a
 four player cabinet and the game PCB.  Coins from two coin mechanisms go into
 a common pool of credits, and when a player presses START the board inserts
 one of those credits into that player's coin input on the game and then
 passes the START press on.

 Components:
  - PIC16C54 (socketed, labelled 4x4N on the photographed board).
  - 2 x DM74LS166N.
  - CNY74-2 and CNY74-4 optocouplers for the coin mechanisms and start buttons.
  - 74HCT273 (lamps, START outputs, display) and CD74HCT174E (meters, coin
    outputs), buffered by two ULN2003A.
  - 10-position DIP switch.

 Notes from the program:

 - The main loop is paced by TMR0 with a 1:32 prescaler, waiting for it to
   reach 0xa0.  The oscillator cannot be identified on the photograph; 4 MHz,
   the most the XT mode in the config word allows, gives a 5.12 ms loop.  Each
   input is sampled once per loop and counts after six consecutive low
   samples, about 31 ms, and has to read high again before it can count again.

 - The coin table for coin mechanism 1 in the ROM matches the manual row for
   row.

 - Every START press adds a pending transfer for that player.  The outputs
   advance in steps: a transfer with credits in the pool pulses that player's
   coin line (MON1 to MON4 in the manual) for one step, waits, and then pulses
   the player's START line for one step; with the pool empty only START is
   pulsed, so the button still works as a START button in the game.

 - The number of credits in the pool (up to ten) is blinked on one lamp, then,
   after a pause, on the other.

 - The display shows the pool as a single digit, E meaning ten or more.  The
   program clocks a single one in and walks it through seven positions,
   putting segments a to g on the data line in turn, then flushes it out.  That
   fits a shift register on the display picking one segment at a time, with the
   data line powering the digit.  No photograph of the display has turned up.

 - The config word that survives in all four dumps is 0xff9: XT oscillator,
   watchdog off, code protection off.  The checksums in the file names are the
   sum of the program bytes plus the four ID nibbles plus one in all four, and
   all four programs are coherent from the reset vector on.

 Differences between the programs:

 - 4x4.1 drives the digit from PORTB instead, so it needs a display wired
   there.  Its font also has a blank and a minus that are never used.  It
   reads SW1 to SW7 only.  Steps are 64 loops (330 ms) long, a transfer waits
   five steps between MON and START, and the pending transfers and the meter
   pulses are kept in nibbles, so more than fifteen of them carry into the next
   player's or meter's count.  Twenty words are unused, and a piece of a two
   digit conversion is left behind as dead code.

 - 4x4.2 is 4x4.1 with the two wire display and its font, and nothing else
   changed.

 - 4x4.3 is a rewrite.  Steps are 32 loops (165 ms) long and a transfer waits
   one step between MON and START.  Pending transfers get a byte each and are
   capped at one more than the pool.  It reads SW8 and SW9: with SW8 off a
   START press takes the whole pool for that player, one MON and START pair per
   credit, and the other START buttons are ignored until it is done; with SW9
   off every transfer goes to player 1.  It leaves two of the timing registers
   uninitialised.

 - 4x4.N is 4x4.3 compacted with indirect addressing to fit the whole ROM,
   with SW10 added and SW9 redone to match the manual.  With SW9 off, players 1
   and 2 share MON1 and players 3 and 4 share MON3, each pair taking turns, as
   two two-player boards would need, while the START lines stay separate.  With
   SW10 off the pool is not kept: each credit is pulsed straight out on MON1,
   and START presses only pass through while the pool is empty.

 The DIP switch settings below follow the manual, which matches 4x4.N.

 TODO:
  - Hook the credit display up in a host and create a layout for it.

*******************************************************************************/

#include "emu.h"
#include "gaelco_cred4x4.h"


DEFINE_DEVICE_TYPE(GAELCO_CRED4X4, gaelco_cred4x4_device, "gcred4x4", "Gaelco 4x4 credit adapter")


namespace {

ROM_START(gcred4x4)
	ROM_REGION(0x1fff, "pic", 0)
	ROM_DEFAULT_BIOS("vn")

	ROM_SYSTEM_BIOS(0, "v1", "4x4.1")
	ROMX_LOAD("m._4x4.1_e774_pic16c54.bin", 0x0000, 0x1fff, CRC(eed0b219) SHA1(35aee42b2f09292f42ca10b22a725655c89c8ff6), ROM_BIOS(0))

	ROM_SYSTEM_BIOS(1, "v2", "4x4.2")
	ROMX_LOAD("m._4x4.2_e9ee_pic16c54.bin", 0x0000, 0x1fff, CRC(6f25a128) SHA1(ad115b1d5ca6d9acf485b87f1ed9457cf3f4e682), ROM_BIOS(1))

	ROM_SYSTEM_BIOS(2, "v3", "4x4.3")
	ROMX_LOAD("m._4x4.3_ea25_pic16c54.bin", 0x0000, 0x1fff, CRC(1a3faf9b) SHA1(4075cb132676ef14e7ca6c467af8645f3175c3cd), ROM_BIOS(2))

	ROM_SYSTEM_BIOS(3, "vn", "4x4.N")
	ROMX_LOAD("m._4x4.n_e684_pic16c54.bin", 0x0000, 0x1fff, CRC(e56608ba) SHA1(b45518c6807e4dda064abcdfa9f148a7b074de39), ROM_BIOS(3))
ROM_END


INPUT_PORTS_START(gcred4x4)
	PORT_START("IN")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START("DSW")
	PORT_DIPNAME( 0x00f, 0x000, DEF_STR( Coin_A ) )  PORT_DIPLOCATION("SW:1,2,3,4")
	PORT_DIPSETTING(     0x00e, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(     0x00c, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(     0x00f, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(     0x007, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(     0x00d, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(     0x000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(     0x008, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(     0x009, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(     0x001, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(     0x00a, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(     0x00b, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(     0x002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(     0x003, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(     0x004, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(     0x005, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(     0x006, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x070, 0x000, DEF_STR( Coin_B ) )  PORT_DIPLOCATION("SW:5,6,7")
	PORT_DIPSETTING(     0x000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(     0x010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(     0x020, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(     0x030, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(     0x040, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(     0x050, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(     0x060, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(     0x070, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x180, 0x000, "Operating Mode" )   PORT_DIPLOCATION("SW:8,9")
	PORT_DIPSETTING(     0x000, "One 4-player board, one credit per start" )
	PORT_DIPSETTING(     0x080, "One 4-player board, all credits on start" )
	PORT_DIPSETTING(     0x100, "Two boards (MON1, MON3), one credit per start" )
	PORT_DIPSETTING(     0x180, "Two boards (MON1, MON3), all credits on start" )
	PORT_DIPNAME( 0x200, 0x000, "Accumulate Credits" ) PORT_DIPLOCATION("SW:10")
	PORT_DIPSETTING(     0x200, "No, straight to MON1" )
	PORT_DIPSETTING(     0x000, DEF_STR( Yes ) )
INPUT_PORTS_END

} // anonymous namespace


gaelco_cred4x4_device::gaelco_cred4x4_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, GAELCO_CRED4X4, tag, owner, clock)
	, m_pic(*this, "pic")
	, m_shifter(*this, "shifter%u", 0U)
	, m_display(*this, "display")
	, m_in(*this, "IN")
	, m_dsw(*this, "DSW")
	, m_meter_cb(*this)
	, m_lamp_cb(*this)
	, m_digit_cb(*this)
	, m_direct_display(false)
	, m_porta(0)
	, m_portb(0)
	, m_qh(0)
	, m_start_latch(0)
	, m_coin_latch(0)
	, m_display_shift(0)
{
}

const tiny_rom_entry *gaelco_cred4x4_device::device_rom_region() const
{
	return ROM_NAME(gcred4x4);
}

ioport_constructor gaelco_cred4x4_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gcred4x4);
}

void gaelco_cred4x4_device::device_add_mconfig(machine_config &config)
{
	PIC16C54(config, m_pic, 4'000'000); // oscillator not identified, see notes
	m_pic->read_a().set(FUNC(gaelco_cred4x4_device::porta_r));
	m_pic->write_a().set(FUNC(gaelco_cred4x4_device::porta_w));
	m_pic->write_b().set(FUNC(gaelco_cred4x4_device::portb_w));

	TTL166(config, m_shifter[0]);
	m_shifter[0]->data_callback().set(FUNC(gaelco_cred4x4_device::switches_r));
	m_shifter[0]->qh_callback().set(m_shifter[1], FUNC(ttl166_device::serial_w));

	TTL166(config, m_shifter[1]);
	m_shifter[1]->data_callback().set(FUNC(gaelco_cred4x4_device::inputs_r));
	m_shifter[1]->qh_callback().set(FUNC(gaelco_cred4x4_device::qh_w));

	PWM_DISPLAY(config, m_display).set_size(1, 7);
	m_display->set_segmask(1, 0x7f);
	m_display->output_digit().set(FUNC(gaelco_cred4x4_device::digit_w));
}

void gaelco_cred4x4_device::device_start()
{
	m_direct_display = system_bios() == 1;

	save_item(NAME(m_porta));
	save_item(NAME(m_portb));
	save_item(NAME(m_qh));
	save_item(NAME(m_start_latch));
	save_item(NAME(m_coin_latch));
	save_item(NAME(m_display_shift));
}


u8 gaelco_cred4x4_device::switches_r()
{
	return m_dsw->read() & 0xff;
}

u8 gaelco_cred4x4_device::inputs_r()
{
	return (m_in->read() & 0xfc) | BIT(m_dsw->read(), 8, 2);
}

u8 gaelco_cred4x4_device::porta_r()
{
	return m_qh << 3;
}

void gaelco_cred4x4_device::porta_w(u8 data)
{
	u8 const rising = data & ~m_porta;
	m_porta = data;

	if (BIT(rising, 0))
		start_latch_w(m_portb);
	if (BIT(rising, 1))
		coin_latch_w(m_portb & 0x3f);

	m_shifter[0]->clock_w(BIT(data, 2));
	m_shifter[1]->clock_w(BIT(data, 2));
}

void gaelco_cred4x4_device::portb_w(u8 data)
{
	m_portb = data;
	if (m_direct_display)
		update_display();

	m_shifter[0]->shift_load_w(BIT(data, 7));
	m_shifter[1]->shift_load_w(BIT(data, 7));
}

void gaelco_cred4x4_device::start_latch_w(u8 data)
{
	u8 const old = m_start_latch;
	m_start_latch = data;

	for (unsigned i = 0; i < 2; i++)
	{
		if (BIT(old ^ data, i))
			m_lamp_cb[i](BIT(data, i));
	}

	if (BIT(data & ~old, 6))
		m_display_shift = (m_display_shift << 1) | BIT(data, 7);
	update_display();
}

void gaelco_cred4x4_device::coin_latch_w(u8 data)
{
	u8 const old = m_coin_latch;
	m_coin_latch = data;

	for (unsigned i = 0; i < 2; i++)
	{
		if (BIT(old ^ data, i))
			m_meter_cb[i](BIT(data, i));
	}
}

void gaelco_cred4x4_device::update_display()
{
	u8 segments = 0;
	if (BIT(m_start_latch, 7))
	{
		if (m_direct_display)
			segments = m_portb & 0x7f;
		else if ((m_display_shift & 0xfe) && !(m_display_shift & (m_display_shift - 1)))
			segments = m_display_shift >> 1;
	}
	m_display->matrix(1, segments);
}
