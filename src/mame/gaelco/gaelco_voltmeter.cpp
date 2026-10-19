// license:BSD-3-Clause
// copyright-holders: Tomás García-Merás Capote (ClawGrip)
/*******************************************************************************

 Gaelco voltmeter

 PIC16C54 programs labelled M. Voltime., M. V2 and M. V3, found with Gaelco's
 4x4 and 2x1 credit boards in a box labelled VOLTIMETR.  No photograph,
 schematic or manual of the board has turned up, so the hardware here is what
 the programs imply.

 - The board turns coins into credits with the coin table of the 4x4 credit
   adapter and pulses them out one at a time, low for about 95 ms and at least
   95 ms apart at 4 MHz.  The coin inputs are read every 1 to 4 ms, and a coin
   counts after two low readings.  The switches are only read at reset.

 - The two digits are lit in turn, for about 1 ms each every 7 ms.  When the
   tens digit goes off, the program counts in steps of 6 instruction cycles
   until a falling edge on RTCC, looks the count up in a table of thresholds
   from 0x32 up to 0xec and shows 70 down to 41, 40 above the table, "E" below
   45 steps and "--" without an edge within 255 steps.  The display takes a new
   reading every 48 measurements.  What drives RTCC is unknown, so the time
   from RA3 going high to the edge is an adjuster in microseconds.

 - V2 makes the credit pulses about 205 ms long.  V3 also needs four low
   readings for a coin, and its config word turns the watchdog on.

 TODO:
  - Create a layout for the display.
  - Find out what the board measures.

*******************************************************************************/

#include "emu.h"
#include "gaelco_voltmeter.h"


DEFINE_DEVICE_TYPE(GAELCO_VOLTMETER, gaelco_voltmeter_device, "gvoltmeter", "Gaelco voltmeter")


namespace {

// programmer images: program words, ID words at 0x400 and the low byte of the
// config word at 0x1ffe
ROM_START(gvoltmeter)
	ROM_REGION(0x1fff, "pic", 0)
	ROM_DEFAULT_BIOS("v3")

	ROM_SYSTEM_BIOS(0, "voltime", "Voltime")
	ROMX_LOAD("m._voltime._11c7b_pic16c54.bin", 0x0000, 0x1fff, CRC(62095656) SHA1(d94ecbd8419420fe8906b84c547427db1742596d), ROM_BIOS(0))

	ROM_SYSTEM_BIOS(1, "v2", "V2")
	ROMX_LOAD("m._v2_11b95_pic16c54.bin", 0x0000, 0x1fff, CRC(78840928) SHA1(d08611f41f04f6f17b75977370c12318cafcd70e), ROM_BIOS(1))

	ROM_SYSTEM_BIOS(2, "v3", "V3")
	ROMX_LOAD("m._v3_11cfb_pic16c54a.bin", 0x0000, 0x1fff, CRC(478489bc) SHA1(7e55b920d88a6e79f840881dddc4c64837e21df1), ROM_BIOS(2))
ROM_END


INPUT_PORTS_START(gvoltmeter)
	PORT_START("IN")
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START("DSW")
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SW1:1,2,3,4")
	PORT_DIPSETTING(    0x0e, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x70, 0x00, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SW1:5,6,7")
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START("SENSOR")
	PORT_BIT( 0x7ff, 900, IPT_ADJUSTER ) PORT_MINMAX(0, 2000) PORT_NAME("Sensor")
INPUT_PORTS_END

} // anonymous namespace


gaelco_voltmeter_device::gaelco_voltmeter_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, GAELCO_VOLTMETER, tag, owner, clock)
	, m_pic(*this, "pic")
	, m_shifter(*this, "shifter")
	, m_display(*this, "display")
	, m_in(*this, "IN")
	, m_dsw(*this, "DSW")
	, m_sensor(*this, "SENSOR")
	, m_digit_cb(*this)
	, m_rtcc_timer(nullptr)
	, m_porta(0xff)
	, m_portb(0xff)
	, m_qh(0)
{
}

const tiny_rom_entry *gaelco_voltmeter_device::device_rom_region() const
{
	return ROM_NAME(gvoltmeter);
}

ioport_constructor gaelco_voltmeter_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gvoltmeter);
}

void gaelco_voltmeter_device::device_add_mconfig(machine_config &config)
{
	PIC16C54(config, m_pic, 4'000'000); // XT oscillator, frequency unknown
	m_pic->read_a().set(FUNC(gaelco_voltmeter_device::porta_r));
	m_pic->write_a().set(FUNC(gaelco_voltmeter_device::porta_w));
	m_pic->write_b().set(FUNC(gaelco_voltmeter_device::portb_w));

	TTL166(config, m_shifter);
	m_shifter->data_callback().set(FUNC(gaelco_voltmeter_device::inputs_r));
	m_shifter->qh_callback().set(FUNC(gaelco_voltmeter_device::qh_w));

	PWM_DISPLAY(config, m_display).set_size(2, 7);
	m_display->set_segmask(0x3, 0x7f);
	m_display->output_digit().set(FUNC(gaelco_voltmeter_device::digit_w));
}

void gaelco_voltmeter_device::device_start()
{
	m_pic->set_config((system_bios() == 3) ? 0xffd : 0xff9); // XT oscillator, watchdog on only in V3

	m_rtcc_timer = timer_alloc(FUNC(gaelco_voltmeter_device::rtcc_fall), this);

	save_item(NAME(m_porta));
	save_item(NAME(m_portb));
	save_item(NAME(m_qh));
}


u8 gaelco_voltmeter_device::inputs_r()
{
	// coins on H and G, SW1 to SW6 on F to A
	return (m_in->read() & 0xc0) | bitswap<6>(m_dsw->read(), 0, 1, 2, 3, 4, 5);
}

void gaelco_voltmeter_device::porta_w(u8 data)
{
	u8 const changed = data ^ m_porta;
	m_porta = data;

	m_shifter->serial_w(BIT(m_dsw->read(), 6));
	m_shifter->shift_load_w(BIT(data, 0));
	m_shifter->clock_w(BIT(data, 1));

	if (BIT(changed, 3))
	{
		if (BIT(data, 3))
		{
			m_rtcc_timer->adjust(attotime::from_usec(m_sensor->read()));
		}
		else
		{
			m_rtcc_timer->adjust(attotime::never);
			m_pic->set_input_line(PIC16C5x_RTCC, ASSERT_LINE);
		}
	}

	update_display();
}

void gaelco_voltmeter_device::portb_w(u8 data)
{
	m_portb = data;
	update_display();
}

TIMER_CALLBACK_MEMBER(gaelco_voltmeter_device::rtcc_fall)
{
	m_pic->set_input_line(PIC16C5x_RTCC, CLEAR_LINE);
}

void gaelco_voltmeter_device::update_display()
{
	m_display->matrix(bitswap<2>(~m_porta, 0, 3), ~m_portb & 0x7f);
}
