// license:BSD-3-Clause
// copyright-holders: Tomás García-Merás Capote (ClawGrip)
/*******************************************************************************

 Gaelco "ADAP" coin adapters and related boards

 Small PIC16C5x boards found with Gaelco's 4x4 and 2x1 credit boards.  No
 photograph, schematic or manual of them has turned up, so the hardware here
 is what the programs imply.

 ADAP coin adapters (ADAP-TE, ADAP-POR and A-T.E, PIC16C54):

 - The program shifts in nine bits, as from the 74166 with a switch on its
   serial input on the PIC Loto-Play board: five switches at reset, and every
   loop, about 15 ms at 4 MHz, the strobe and three bit code of a parallel
   coin acceptor.

 - RA3 and RA2 pulse 12 loops high and 6 low per unit or credit.

 - The display is lit one segment at a time.  To measure, the program ramps
   the DAC down from 0x58, counts the rising edges of the comparator on RTCC,
   looks the step up in a table of 31 thresholds from 0x55 down to 0x16 and
   shows 40 to 70, "--" above the range and "E" below it, changing the reading
   after eight that differ from the shown one.  The thresholds shrink by about
   4% a step, like a thermistor's, but what the board measures is unknown, so
   the other comparator input is an adjuster in DAC steps.

 - When the credits are halved, a 1 unit coin alternates between 0 and 1
   credits.  A-T.E only counts a code it reads twice.

 2x1 credit distributor (M.2x1, PIC16C56):

 - The ADAP hardware plus an output latch.  RA3 is set every loop and never
   cleared.

 - A coin pulse counts after two low readings.  The bonus credit comes every 4
   pulses with up to 3 pulses per credit, every 8 with 4 to 7 and never with 8,
   counting from the last credit handed out.  Each credit handed out pulses the
   meter on its side once per coin pulse not yet counted, up to the pulses per
   credit.

 VC meter (M. VC, PIC16C54):

 - Only the measuring part, with the ramp starting at 0x7f and the same table.
   The reading changes after 31 that differ from the shown one, and shows as
   4.0 to 7.0, "-.-" above the range and "E." below it.

 - The config word turns the watchdog on, but with a reading in range the main
   loop takes 18,400 to 19,900 instruction cycles, and the PIC core always
   times the watchdog out after 18,000 while the real one runs from its own RC
   oscillator, so it is left off.

 TODO:
  - Create layouts for the displays.
  - Find out what the boards measure.

*******************************************************************************/

#include "emu.h"
#include "gaelco_adap.h"


DEFINE_DEVICE_TYPE(GAELCO_ADAP,     gaelco_adap_device,     "gadap",     "Gaelco ADAP-TE coin adapter (Portugal, latest)")
DEFINE_DEVICE_TYPE(GAELCO_ADAP_TE,  gaelco_adap_te_device,  "gadap_te",  "Gaelco ADAP-TE coin adapter")
DEFINE_DEVICE_TYPE(GAELCO_ADAP_POR, gaelco_adap_por_device, "gadap_por", "Gaelco ADAP-POR coin adapter")
DEFINE_DEVICE_TYPE(GAELCO_ADAP_ATE, gaelco_adap_ate_device, "gadap_ate", "Gaelco A-T.E coin adapter")
DEFINE_DEVICE_TYPE(GAELCO_ADAP_2X1, gaelco_adap_2x1_device, "gadap_2x1", "Gaelco 2x1 credit distributor (PIC16C56)")
DEFINE_DEVICE_TYPE(GAELCO_ADAP_VC,  gaelco_adap_vc_device,  "gadap_vc",  "Gaelco VC meter")


namespace {

// programmer images: program words, ID words after them and the low byte of the
// config word at 0x1ffe
ROM_START(gadap)
	ROM_REGION(0x1fff, "pic", 0)
	ROM_LOAD("adap-te_portugal_ult._d969_pic16c54.bin", 0x0000, 0x1fff, CRC(fdc26dd9) SHA1(99e942996fe4349441948b866d2e47416b7977e9))
ROM_END

ROM_START(gadap_te)
	ROM_REGION(0x1fff, "pic", 0)
	ROM_LOAD("adap-te_f329_pic16c54.bin", 0x0000, 0x1fff, CRC(9a42da4a) SHA1(99700c63eb32baf0c75cce3d54b24e34cfeb389a))
ROM_END

ROM_START(gadap_por)
	ROM_REGION(0x1fff, "pic", 0)
	ROM_LOAD("adap-por_eb28_pic16c54.bin", 0x0000, 0x1fff, CRC(cf023a5c) SHA1(c138b4edb8fc87b65f969bdd951df814905a3c9a))
ROM_END

ROM_START(gadap_ate)
	ROM_REGION(0x1fff, "pic", 0)
	ROM_LOAD("mu._a-t.e_c264_pic16c54.bin", 0x0000, 0x1fff, CRC(3abf47d0) SHA1(d961c17b330965e9e94136720132620560927239))
ROM_END

ROM_START(gadap_2x1)
	ROM_REGION(0x1fff, "pic", 0)
	ROM_LOAD("m.2x1_298ef_pic16c56.bin", 0x0000, 0x1fff, CRC(019cd9a6) SHA1(afda82a5c08dc2c46c0850dea22c25791539c40d))
ROM_END

ROM_START(gadap_vc)
	ROM_REGION(0x1fff, "pic", 0)
	ROM_LOAD("m._vc_17658_pic16c54.bin", 0x0000, 0x1fff, CRC(06d86ca1) SHA1(9bb5d5632d562dbc806fd54fc401e20605b43485))
ROM_END


INPUT_PORTS_START(gadap_common)
	PORT_START("IN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) // 20 units
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) // 8 units
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) // 8 units
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 ) // 4 units
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN5 ) // 2 units
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN6 ) // 2 units
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN7 ) // 1 unit
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN8 ) // 1 unit

	PORT_START("SENSOR")
	PORT_BIT( 0x7f, 0x30, IPT_ADJUSTER ) PORT_MINMAX(0x00, 0x7f) PORT_NAME("Sensor")
INPUT_PORTS_END

INPUT_PORTS_START(gadap)
	PORT_INCLUDE(gadap_common)

	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Credits" )        PORT_DIPLOCATION("SW1:1") PORT_CONDITION("DSW", 0x04, EQUALS, 0x04)
	PORT_DIPSETTING(    0x01, "Coin value" )
	PORT_DIPSETTING(    0x00, "Half the coin value" )
	PORT_DIPNAME( 0x02, 0x02, "Coin Values" )    PORT_DIPLOCATION("SW1:2") PORT_CONDITION("DSW", 0x05, EQUALS, 0x05)
	PORT_DIPSETTING(    0x02, "20, 8, 4, 2, 1" )
	PORT_DIPSETTING(    0x00, "25, 10, 5, 2, 1" )
	PORT_DIPNAME( 0x02, 0x02, "Halved Values" )  PORT_DIPLOCATION("SW1:2") PORT_CONDITION("DSW", 0x05, EQUALS, 0x04)
	PORT_DIPSETTING(    0x02, "Rounded down" )
	PORT_DIPSETTING(    0x00, "Made odd" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus" )          PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, "3 for 2, 6 for 4" )
	PORT_DIPNAME( 0x08, 0x08, "Double Credits" ) PORT_DIPLOCATION("SW1:4") PORT_CONDITION("DSW", 0x06, EQUALS, 0x06)
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x10, "SW1:5" )
INPUT_PORTS_END

INPUT_PORTS_START(gadap_te)
	PORT_INCLUDE(gadap_common)

	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Credits" )        PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, "Coin value" )
	PORT_DIPSETTING(    0x00, "Half the coin value" )
	PORT_DIPNAME( 0x02, 0x02, "Halved Values" )  PORT_DIPLOCATION("SW1:2") PORT_CONDITION("DSW", 0x01, EQUALS, 0x00)
	PORT_DIPSETTING(    0x02, "Rounded down" )
	PORT_DIPSETTING(    0x00, "Made odd" )
	PORT_DIPUNUSED_DIPLOC( 0x04, 0x04, "SW1:3" )
	PORT_DIPUNUSED_DIPLOC( 0x08, 0x08, "SW1:4" )
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x10, "SW1:5" )
INPUT_PORTS_END

INPUT_PORTS_START(gadap_por)
	PORT_INCLUDE(gadap_common)

	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Credits" )        PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, "Coin value" )
	PORT_DIPSETTING(    0x00, "Half the coin value" )
	PORT_DIPNAME( 0x02, 0x02, "Halved Values" )  PORT_DIPLOCATION("SW1:2") PORT_CONDITION("DSW", 0x01, EQUALS, 0x00)
	PORT_DIPSETTING(    0x02, "Rounded down" )
	PORT_DIPSETTING(    0x00, "Made odd" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus" )          PORT_DIPLOCATION("SW1:3") PORT_CONDITION("DSW", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, "22 for 20, 6 for 4, 3 for 2" )
	PORT_DIPUNUSED_DIPLOC( 0x08, 0x08, "SW1:4" )
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x10, "SW1:5" )
INPUT_PORTS_END

INPUT_PORTS_START(gadap_ate)
	PORT_INCLUDE(gadap_common)

	PORT_START("DSW")
	PORT_DIPNAME( 0x07, 0x07, "Credits For 20, 8, 4, 2 and 1 Units" ) PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x03, "5, 2, 1, 1/2, 1/4" )
	PORT_DIPSETTING(    0x02, "6, 2, 1, 1/2, 1/4" )
	PORT_DIPSETTING(    0x01, "6, 3, 1, 1/2, 1/4" )
	PORT_DIPSETTING(    0x05, "10, 4, 2, 1, 1/2" )
	PORT_DIPSETTING(    0x00, "10, 5, 3, 1, 1/2" )
	PORT_DIPSETTING(    0x04, "15, 6, 3, 1, 1/2" )
	PORT_DIPSETTING(    0x07, "20, 8, 4, 2, 1" )
	PORT_DIPSETTING(    0x06, "25, 10, 5, 2, 1" )
	PORT_DIPNAME( 0x18, 0x18, "Extra Credits For 20 Units" )          PORT_DIPLOCATION("SW1:4,5")
	PORT_DIPSETTING(    0x18, "0" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x00, "3" )
INPUT_PORTS_END

INPUT_PORTS_START(gadap_2x1)
	PORT_START("IN")
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Credit Button 1") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Credit Button 2") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START("DSW")
	PORT_DIPNAME( 0x07, 0x07, "Pulses Per Credit" )        PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x07, "8" )
	PORT_DIPSETTING(    0x06, "7" )
	PORT_DIPSETTING(    0x05, "6" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus Credit" )             PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Credits Per Button Press" ) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x10, "One" )
	PORT_DIPSETTING(    0x00, "All" )
	PORT_DIPNAME( 0x20, 0x20, "Display" )                  PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x20, "Sensor" )
	PORT_DIPSETTING(    0x00, "Credits" )

	PORT_START("SENSOR")
	PORT_BIT( 0x7f, 0x30, IPT_ADJUSTER ) PORT_MINMAX(0x00, 0x7f) PORT_NAME("Sensor")
INPUT_PORTS_END

INPUT_PORTS_START(gadap_vc)
	PORT_START("SENSOR")
	PORT_BIT( 0x7f, 0x30, IPT_ADJUSTER ) PORT_MINMAX(0x00, 0x7f) PORT_NAME("Sensor")
INPUT_PORTS_END

} // anonymous namespace


gaelco_adap_base::gaelco_adap_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, type, tag, owner, clock)
	, m_pic(*this, "pic")
	, m_shifter(*this, "shifter")
	, m_in(*this, "IN")
	, m_dsw(*this, "DSW")
	, m_porta(0)
	, m_portb(0)
	, m_display(*this, "display")
	, m_sensor(*this, "SENSOR")
	, m_digit_cb(*this)
	, m_qh(0)
{
}

void gaelco_adap_base::device_start()
{
	save_item(NAME(m_porta));
	save_item(NAME(m_portb));
	save_item(NAME(m_qh));
}

void gaelco_adap_base::common(machine_config &config)
{
	m_pic->set_config(0xfff); // RC oscillator, watchdog on
	m_pic->read_b().set(FUNC(gaelco_adap_base::portb_r));
	m_pic->write_b().set(FUNC(gaelco_adap_base::portb_w));

	TTL166(config, m_shifter);
	m_shifter->qh_callback().set(FUNC(gaelco_adap_base::qh_w));

	PWM_DISPLAY(config, m_display).set_size(2, 7);
	m_display->set_segmask(0x3, 0x7f);
	m_display->output_digit().set(FUNC(gaelco_adap_base::digit_w));
}

void gaelco_adap_base::common_porta_w(u8 data)
{
	m_porta = data;
	m_shifter->serial_w(BIT(m_dsw->read(), 0));
	m_shifter->shift_load_w(BIT(data, 1));
	m_shifter->clock_w(BIT(data, 0));
	update_display();
}

void gaelco_adap_base::portb_w(u8 data)
{
	m_portb = data;
	m_pic->set_input_line(PIC16C5x_RTCC, (m_sensor->read() > (data >> 1)) ? ASSERT_LINE : CLEAR_LINE);
	update_display();
}

void gaelco_adap_base::update_display()
{
	m_display->matrix(bitswap<2>(m_porta, 0, 1), (~m_portb >> 1) & 0x7f);
}


gaelco_adap_device::gaelco_adap_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: gaelco_adap_device(mconfig, GAELCO_ADAP, tag, owner, clock)
{
}

gaelco_adap_device::gaelco_adap_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock)
	: gaelco_adap_base(mconfig, type, tag, owner, clock)
{
}

const tiny_rom_entry *gaelco_adap_device::device_rom_region() const
{
	return ROM_NAME(gadap);
}

ioport_constructor gaelco_adap_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gadap);
}

void gaelco_adap_device::device_add_mconfig(machine_config &config)
{
	PIC16C54(config, m_pic, 4'000'000); // RC oscillator, frequency unknown
	m_pic->write_a().set(FUNC(gaelco_adap_device::common_porta_w));
	common(config);

	m_shifter->data_callback().set(FUNC(gaelco_adap_device::acceptor_r));
}

u8 gaelco_adap_device::acceptor_r()
{
	// strobe, code and SW5 to SW2 in the order they are shifted out
	u32 const coins = ~m_in->read() & 0xff;
	u8 lines = 0x80;
	for (unsigned i = 0; i < 8; i++)
	{
		if (BIT(coins, i))
		{
			lines = (7 - i) << 4;
			break;
		}
	}
	return lines | BIT(m_dsw->read(), 1, 4);
}


gaelco_adap_te_device::gaelco_adap_te_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: gaelco_adap_device(mconfig, GAELCO_ADAP_TE, tag, owner, clock)
{
}

const tiny_rom_entry *gaelco_adap_te_device::device_rom_region() const
{
	return ROM_NAME(gadap_te);
}

ioport_constructor gaelco_adap_te_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gadap_te);
}


gaelco_adap_por_device::gaelco_adap_por_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: gaelco_adap_device(mconfig, GAELCO_ADAP_POR, tag, owner, clock)
{
}

const tiny_rom_entry *gaelco_adap_por_device::device_rom_region() const
{
	return ROM_NAME(gadap_por);
}

ioport_constructor gaelco_adap_por_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gadap_por);
}


gaelco_adap_ate_device::gaelco_adap_ate_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: gaelco_adap_device(mconfig, GAELCO_ADAP_ATE, tag, owner, clock)
{
}

const tiny_rom_entry *gaelco_adap_ate_device::device_rom_region() const
{
	return ROM_NAME(gadap_ate);
}

ioport_constructor gaelco_adap_ate_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gadap_ate);
}


gaelco_adap_2x1_device::gaelco_adap_2x1_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: gaelco_adap_base(mconfig, GAELCO_ADAP_2X1, tag, owner, clock)
	, m_credit_display(*this, "credit_display")
	, m_meter_cb(*this)
	, m_credit_digit_cb(*this)
	, m_latch(0)
	, m_display_shift(0)
{
}

const tiny_rom_entry *gaelco_adap_2x1_device::device_rom_region() const
{
	return ROM_NAME(gadap_2x1);
}

ioport_constructor gaelco_adap_2x1_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gadap_2x1);
}

void gaelco_adap_2x1_device::device_add_mconfig(machine_config &config)
{
	PIC16C56(config, m_pic, 4'000'000); // RC oscillator, frequency unknown
	m_pic->write_a().set(FUNC(gaelco_adap_2x1_device::porta_w));
	common(config);

	m_shifter->data_callback().set(FUNC(gaelco_adap_2x1_device::inputs_r));

	PWM_DISPLAY(config, m_credit_display).set_size(1, 7);
	m_credit_display->set_segmask(1, 0x7f);
	m_credit_display->output_digit().set(FUNC(gaelco_adap_2x1_device::credit_digit_w));
}

void gaelco_adap_2x1_device::device_start()
{
	gaelco_adap_base::device_start();

	save_item(NAME(m_latch));
	save_item(NAME(m_display_shift));
}

u8 gaelco_adap_2x1_device::inputs_r()
{
	// coin, button 2, button 1 and SW6 to SW2 in the order they are shifted out
	return (m_in->read() & 0xe0) | BIT(m_dsw->read(), 1, 5);
}

void gaelco_adap_2x1_device::porta_w(u8 data)
{
	u8 const rising = data & ~m_porta;
	common_porta_w(data);

	if (BIT(rising, 2))
		latch_w(m_portb);
}

void gaelco_adap_2x1_device::latch_w(u8 data)
{
	u8 const old = m_latch;
	m_latch = data;

	for (unsigned i = 0; i < 2; i++)
	{
		if (BIT(old ^ data, 3 + i))
			m_meter_cb[i](BIT(data, 3 + i));
	}

	if (BIT(data & ~old, 5))
		m_display_shift = (m_display_shift << 1) | BIT(data, 6);

	u8 segments = 0;
	if (BIT(data, 6) && (m_display_shift & 0xfe) && !(m_display_shift & (m_display_shift - 1)))
		segments = m_display_shift >> 1;
	m_credit_display->matrix(1, segments);
}


gaelco_adap_vc_device::gaelco_adap_vc_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, GAELCO_ADAP_VC, tag, owner, clock)
	, m_pic(*this, "pic")
	, m_display(*this, "display")
	, m_sensor(*this, "SENSOR")
	, m_digit_cb(*this)
	, m_porta(0)
	, m_portb(0)
{
}

const tiny_rom_entry *gaelco_adap_vc_device::device_rom_region() const
{
	return ROM_NAME(gadap_vc);
}

ioport_constructor gaelco_adap_vc_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(gadap_vc);
}

void gaelco_adap_vc_device::device_add_mconfig(machine_config &config)
{
	PIC16C54(config, m_pic, 4'000'000); // RC oscillator, frequency unknown
	m_pic->set_config(0xffb); // RC oscillator, watchdog off (see notes)
	m_pic->read_a().set(FUNC(gaelco_adap_vc_device::porta_r));
	m_pic->write_a().set(FUNC(gaelco_adap_vc_device::porta_w));
	m_pic->write_b().set(FUNC(gaelco_adap_vc_device::portb_w));

	PWM_DISPLAY(config, m_display).set_size(2, 8);
	m_display->set_segmask(0x3, 0xff);
	m_display->output_digit().set(FUNC(gaelco_adap_vc_device::digit_w));
}

void gaelco_adap_vc_device::device_start()
{
	save_item(NAME(m_porta));
	save_item(NAME(m_portb));
}

u8 gaelco_adap_vc_device::porta_r()
{
	u8 const dac = bitswap<7>(m_portb, 3, 4, 2, 5, 6, 7, 1);
	return (m_sensor->read() > dac) ? 0x04 : 0x00;
}

void gaelco_adap_vc_device::porta_w(u8 data)
{
	m_porta = data;
	update_display();
}

void gaelco_adap_vc_device::portb_w(u8 data)
{
	m_portb = data;
	update_display();
}

void gaelco_adap_vc_device::update_display()
{
	u8 const segments = ~m_portb;
	if (BIT(m_porta, 1))
		m_display->matrix(1, bitswap<8>(segments, 1, 0, 5, 3, 2, 4, 7, 6));
	else if (BIT(m_porta, 0))
		m_display->matrix(2, bitswap<8>(segments, 1, 6, 3, 5, 0, 7, 4, 2));
	else
		m_display->matrix(0, 0);
}
