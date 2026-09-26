// license:BSD-3-Clause
// copyright-holders:

/***************************************************************************

 ETRA (https://www.grupoetra.com/) semaphore controller for a crosswalk
 (unknown model, mid-1980s according to the date codes on the main PCB).

Main PCB
  __________________________________________________
 |     _______    _______    _______                |
 |    |      |   |      |   |      |                |
 |    |______|   |______|   |______|                |
 |                           _________    _________ |
 |                          |_74LS03_|   DM74LS122N |
 |              _________                           |
 |             |_UM6114_|    _________    _________ |
 |                          |T74LS04B1   |_74LS90_| |__
 |                                                   __|
 |              _________    _________               __|
 |             |_UM6114_|   DM74LS155N    _________  __|
 |       ________________                DM74LS155N  __|
  \     | X2816CP-12    |  Xtal                      __|
  _\    |_______________|  4.000 MHz                 __|
 |__     ________________    ________________        __|
 |__    | AT2716 EPROM  |   | MC6802P       |        __|
 |__    |_______________|   |_______________|        __|
 |__                                                 __|
 |__     _________           ________________        __|
 |__    |_74LS156|          | MC6821P       |       |
 |__                        |_______________|       |
 |__                                                |
 |__     _________    _________    _________        |
 |__    |________|   |_74LS132|   |_74LS90_|        |
   |                                                |
   |________________________________________________|

Relays PCB
           _________________________________________
          |                                         |
          |                                         |
          |                        _________        |
          |                       |74LS122N|        |
          |                 ________________        |
         /                 | MC6821P       |        |
        /                  |_______________|        |
       /                                            |__
  ____/                   _______ _______            __|
 |       _______          MOC3020 MOC3020            __|
 |      TXAL2215B         _______ _______   _______  __|
 |       _______          MOC3020 MOC3020  |_7404N|  __|
  \     TXAL2215B         _______ _______   _______  __|
   \     _______          MOC3020 MOC3020  |_7404N|  __|
   |    TXAL2215B         _______ _______   _______  __|
   |     _______          MOC3020 MOC3020  |_7404N|  __|
   |    TXAL2215B         _______ _______            __|
   |     _______          MOC3020 MOC3020            __|
   |    TXAL2215B         _______ _______            __|
   |     _______          MOC3020 MOC3020           |
   |    TXAL2215B                                   |
   |                                                |
   |                                                |
   |                                                |
   |                                                |
   |________________________________________________|

Programmer PCB (keyboard)
    _______________________________________________________
   |  ______ ______ ______ ______ ______ ______           |
   | | ___ || ___ || ___ || ___ || ___ || ___ |           |
   | ||__| |||__| |||__| |||__| |||__| |||__| |  ___      |
   | ||__| |||__| |||__| |||__| |||__| |||__| | |  |      |
   | |_____||_____||_____||_____||_____||_____| |  |<-7407N
   |                                            |__|      |
   |       __________________________________             |
   |      | ____   ____   ____   ____   _   |             |
   |      || R |  | M |  | N |  | K |  (_)  |             |
   |      ||___|  |___|  |___|  |___|       |             |
   | ___  | ____   ____   ____   ____       |      SWITCH |
   ||  |  || 0 |  | 1 |  | 2 |  | 3 |   __  |             |
   ||  |  ||___|  |___|  |___|  |___|  (||) |             |
   ||__|  | ____   ____   ____   ____       |             |
 74LS155N || 4 |  | 5 |  | 6 |  | 7 |   __  |             |
   |      ||___|  |___|  |___|  |___|  (||) |             |
   | ___  | ____   ____   ____   ____       |    ___      |
   ||  |  || 8 |  | 9 |  | A |  | B |   __  |   |  |      |
   ||  |  ||___|  |___|  |___|  |___|  (||) |   |  |<-SN74LS03N
   ||__|  | ____   ____   ____   ____       |   |__|      |
SCL4052BE || C |  | D |  | E |  | F |       |    ___  ___ |
   |      ||___|  |___|  |___|  |___|       |   |  |<-TC4093BP
   | ___  |_________________________________|   |  | |  | |
   ||  |                                        |__| |__|<-7407N
   ||  |<-CD4093BE                                        |
   ||__|   ____  ____  ____  ____  ____  ____  ____  ____ |
   |       4N32  B250  4N32  B250  4N32  B250  4N32  B250 |
   |            C1000       C1000       C1000       C1000 |
   |                               __________             |
   |                              |  CONN   |             |
   |______________________________________________________|

Notes from one operator that used to work with this controller model:
 For programming the semaphore controller, you just put the memory values with the keyboard.
 From 100 to 200 you'll find the first program, from 200 to 300 the second, and so on up to
 seven programs, with 100 for green, 101 for yellow, 102 for clear, and then repeat it again.

Hardware details deduced from the firmware, not verified on real hardware:
 - 2000-23FF is assumed to be the battery backed RAM holding the programs. The EEPROM dump
   has a configuration laid out for this range, probably for another firmware version, but
   its programs (17 and 3 steps) don't match the 6 step lamp tables this firmware reads
   from the EEPROM at C625.
 - Up to ten relays PCBs (selected by A2-A11); the lamp tables in the EEPROM drive three.
   Outputs: bits 0/4 red, 1/5 amber, 2/6 green. The lamp current sensors are compared with
   the outputs using the mask at 160, three failed checks restart the flashing start-up.

Keyboard (3 address and 3 data digits, decimal by default): R clears the entry, M toggles
the hexadecimal mode (4 + 2 digits, any address), K stores and advances to the next address,
N clears from the address up to the value entered as data. Addresses 000-099 show the
internal RAM (091 selects the program when not selected externally), 100-999 the RAM at
2064-23E7. The rightmost DP is off while there is no pending pedestrian demand.

Programs 1-4 at 100, 200, 300, 400:
 +0..+23   step durations in seconds, run from step N-1 down to step 0 (main green)
 +24       number of steps N
 +25, +26  start-up all red and steady amber durations
 +29       synchronisation offset
 +30..+53  non zero if the step also times out in manual mode
Other parameters: 127 start-up flashing duration (0 = 255 s), 160 lamp monitor mask,
161/162 synchronisation limits (maximum wait, shortening window), 164 step after which the
lamps rest in step 0 until there is a pedestrian demand.

With the RAM empty the controller flashes amber for about four minutes and then hangs.
Example crosswalk program (R, address, data and K for each value): 100=20, 101=3, 102=5,
103=10, 104=2, 105=3, 124=6, 125=3, 126=3, 127=5, 161=60.

TODO:
 - verify the memory map, the NMI source and the switch / input assignments
 - watchdogs (74LS122 on the main and relays PCBs, retriggered with CA2 and CB2)
 - the lamp current sensors always report working lamps

***************************************************************************/

#include "emu.h"

#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "machine/eeprompar.h"
#include "machine/input_merger.h"
#include "machine/nvram.h"
#include "machine/timer.h"
#include "video/pwm.h"

#include "semcrossw.lh"


namespace {

class semcrossw_state : public driver_device
{
public:
	semcrossw_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_pia(*this, "pia")
		, m_relay_pia(*this, "relay_pia%u", 1U)
		, m_display(*this, "display")
		, m_keys(*this, "KEY%u", 0U)
		, m_switches(*this, "SW%u", 0U)
		, m_lamps(*this, "lamp%u_%u", 1U, 0U)
		, m_sync_out(*this, "sync_out")
	{
	}

	void semcrossw(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	static constexpr unsigned RELAY_BOARDS = 3;

	required_device<m6802_cpu_device> m_maincpu;
	required_device<pia6821_device> m_pia;
	required_device_array<pia6821_device, RELAY_BOARDS> m_relay_pia;
	required_device<pwm_display_device> m_display;
	required_ioport_array<5> m_keys;
	required_ioport_array<3> m_switches;
	output_finder<RELAY_BOARDS, 8> m_lamps;
	output_finder<> m_sync_out;

	u8 m_pia_pa = 0xff;
	u8 m_pia_pb = 0xff;
	u8 m_relay_pa[RELAY_BOARDS] = { 0xff, 0xff, 0xff };
	u8 m_mains = 0;
	u8 m_nmi_div = 0;

	void mem_map(address_map &map) ATTR_COLD;

	u8 pia_pa_r();
	void pia_pa_w(u8 data);
	void pia_pb_w(u8 data);
	void update_display();

	u8 relay_r(offs_t offset);
	void relay_w(offs_t offset, u8 data);
	template <unsigned N> void relay_pa_w(u8 data);
	template <unsigned N> u8 relay_pb_r();

	TIMER_DEVICE_CALLBACK_MEMBER(mains_tick);
};


void semcrossw_state::machine_start()
{
	save_item(NAME(m_pia_pa));
	save_item(NAME(m_pia_pb));
	save_item(NAME(m_relay_pa));
	save_item(NAME(m_mains));
	save_item(NAME(m_nmi_div));
}


u8 semcrossw_state::pia_pa_r()
{
	// rows through a 74LS155, columns through a 4052
	u8 const row = m_pia_pa & 0x07;
	u8 const col = (m_pia_pa >> 3) & 0x03;
	u8 data = 0xff;

	if (row < 5)
	{
		if (!BIT(m_keys[row]->read(), col))
			data &= ~0x20;
	}
	else if (col < 3)
	{
		if (!BIT(m_switches[col]->read(), row - 5))
			data &= ~0x40;
	}

	return data;
}

void semcrossw_state::pia_pa_w(u8 data)
{
	m_pia_pa = data;
	m_sync_out = BIT(data, 7);
	update_display();
}

void semcrossw_state::pia_pb_w(u8 data)
{
	m_pia_pb = data;
	update_display();
}

void semcrossw_state::update_display()
{
	u8 const sel = m_pia_pa & 0x07;
	m_display->matrix((sel < 6) ? (1 << sel) : 0, ~m_pia_pb & 0xff);
}


u8 semcrossw_state::relay_r(offs_t offset)
{
	u8 data = 0xff;
	for (unsigned i = 0; i < RELAY_BOARDS; i++)
		if (BIT(offset, i + 2))
			data &= m_relay_pia[i]->read(offset & 0x03);

	return data;
}

void semcrossw_state::relay_w(offs_t offset, u8 data)
{
	for (unsigned i = 0; i < RELAY_BOARDS; i++)
		if (BIT(offset, i + 2))
			m_relay_pia[i]->write(offset & 0x03, data);
}

template <unsigned N>
void semcrossw_state::relay_pa_w(u8 data)
{
	m_relay_pa[N] = data;
	for (unsigned i = 0; i < 8; i++)
		if (i != 3 && i != 7)
			m_lamps[N][i] = BIT(~data, i);
}

template <unsigned N>
u8 semcrossw_state::relay_pb_r()
{
	// lamp current sensors, they read like the outputs when the lamps work
	return m_relay_pa[N];
}


TIMER_DEVICE_CALLBACK_MEMBER(semcrossw_state::mains_tick)
{
	// mains zero crossings, the firmware uses both edges
	m_mains ^= 1;
	m_pia->ca1_w(m_mains);
	m_pia->cb1_w(m_mains);

	// the step and flashing timings need a 20 Hz NMI: assumed to be the zero crossings
	// divided by the 74LS90
	if (++m_nmi_div == 5)
	{
		m_nmi_div = 0;
		m_maincpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);
	}
}


void semcrossw_state::mem_map(address_map &map)
{
	map(0x2000, 0x23ff).ram().share("nvram");
	// 8400-8403: a PIA is initialized by the firmware but never used
	map(0x8800, 0x8803).rw(m_pia, FUNC(pia6821_device::read), FUNC(pia6821_device::write));
	map(0xa000, 0xafff).rw(FUNC(semcrossw_state::relay_r), FUNC(semcrossw_state::relay_w));
	map(0xc000, 0xc7ff).rw("eeprom", FUNC(eeprom_parallel_28xx_device::read), FUNC(eeprom_parallel_28xx_device::write));
	map(0xf800, 0xffff).rom().region("maincpu", 0);
}


static INPUT_PORTS_START(semcrossw)
	PORT_START("KEY0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("R (Clear)") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("M (Decimal/Hex Mode)") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("N (Clear Range)") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("K (Store/Next)") PORT_CODE(KEYCODE_K)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("SW0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("External Program Select Bit 0") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("External Program Select Bit 1") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("Pedestrian Request") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0xf8, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("SW1") // flashing and manual modes are selected with the switch open
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_TOGGLE PORT_NAME("Flashing Mode") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_TOGGLE PORT_NAME("Manual Mode") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("External Program Selection") PORT_CODE(KEYCODE_X)
	PORT_BIT(0xf8, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("SW2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("Synchronisation Input") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("Manual Step Advance") PORT_CODE(KEYCODE_SPACE) // advances on release
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


void semcrossw_state::semcrossw(machine_config &config)
{
	M6802(config, m_maincpu, 4_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &semcrossw_state::mem_map);

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0); // 2 x UM6114, battery backed

	EEPROM_2816(config, "eeprom");

	PIA6821(config, m_pia);
	m_pia->readpa_handler().set(FUNC(semcrossw_state::pia_pa_r));
	m_pia->writepa_handler().set(FUNC(semcrossw_state::pia_pa_w));
	m_pia->writepb_handler().set(FUNC(semcrossw_state::pia_pb_w));
	m_pia->irqa_handler().set("mainirq", FUNC(input_merger_device::in_w<0>));
	m_pia->irqb_handler().set("mainirq", FUNC(input_merger_device::in_w<1>));

	INPUT_MERGER_ANY_HIGH(config, "mainirq").output_handler().set_inputline(m_maincpu, M6802_IRQ_LINE);

	TIMER(config, "mains").configure_periodic(FUNC(semcrossw_state::mains_tick), attotime::from_hz(100));

	PIA6821(config, m_relay_pia[0]);
	m_relay_pia[0]->writepa_handler().set(FUNC(semcrossw_state::relay_pa_w<0>));
	m_relay_pia[0]->readpb_handler().set(FUNC(semcrossw_state::relay_pb_r<0>));

	PIA6821(config, m_relay_pia[1]);
	m_relay_pia[1]->writepa_handler().set(FUNC(semcrossw_state::relay_pa_w<1>));
	m_relay_pia[1]->readpb_handler().set(FUNC(semcrossw_state::relay_pb_r<1>));

	PIA6821(config, m_relay_pia[2]);
	m_relay_pia[2]->writepa_handler().set(FUNC(semcrossw_state::relay_pa_w<2>));
	m_relay_pia[2]->readpb_handler().set(FUNC(semcrossw_state::relay_pb_r<2>));

	PWM_DISPLAY(config, m_display).set_size(6, 8);
	m_display->set_segmask(0x3f, 0xff);
}


ROM_START(semcrossw)
	ROM_REGION(0x800, "maincpu", 0)
	ROM_LOAD("at27c16.bin",    0x000, 0x800, CRC(2e7b10b1) SHA1(fba6465db1baa38ab79ed24a85de460f8be488b9))

	ROM_REGION(0x800, "eeprom", 0)
	ROM_LOAD("x2816cp-12.bin", 0x000, 0x800, BAD_DUMP CRC(c2ef2e80) SHA1(6c3c4215169c2941a37053888174fe0499301bac)) // BAD_DUMP because dumped from an already configured machine
ROM_END

} // anonymous namespace


//    YEAR  NAME       PARENT MACHINE    INPUT      CLASS            INIT        MONITOR COMPANY FULLNAME                                              FLAGS                                        LAYOUT
GAMEL(198?, semcrossw, 0,     semcrossw, semcrossw, semcrossw_state, empty_init, ROT0,   "Etra", "Crosswalk traffic light controller (unknown model)", MACHINE_NO_SOUND_HW | MACHINE_NOT_WORKING, layout_semcrossw)
