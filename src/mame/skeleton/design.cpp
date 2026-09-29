// license:BSD-3-Clause
// copyright-holders:Dirk Best
/*
    Azkoyen "Design" series tobacco vending machines

    The "Design" line (D6, D8, D10, D12, D14, D21...) was sold in Spain
    from the early 1990s (the operator manual is dated 1993/1994).  For
    the D14 and D21, the number in the model name is the number of product
    channels.  Two firmware dumps are supported:

    * design6: pesetas firmware.  Only contains tables for a six channel
      machine (D6).
    * designe: euro firmware, part number 43521600-5, version string
      "05-10-06 DES ESTANDAR-REYNOLDS-FOTOS-CASETES-MEDIAS-AEOPUERTO
      BARCELONA".  Contains 16 model presets (D6, D8, D10 RODE, D10/D12,
      D14, D21, promotional and special variants) selected by the operator
      from the CONFIGURACION menu.

    This driver distinguishes three kinds of information:

    * Confirmed: seen on the PCB, or directly observable in the firmware
      (addresses accessed, values written, conditions tested, messages
      shown).  Firmware facts apply to both dumps unless noted.
    * Assumed: interpretations that fit the firmware behaviour and the
      operator manual but have not been checked on real hardware.  Marked
      with "(assumption)" in comments.
    * Simulated: mechanical parts of the machine (not on the PCB) that are
      modelled with arbitrary timings so the firmware can run.  Marked with
      "(simulated)" in comments.

    Hardware, as listed in the original notes for this driver (it is not
    stated which of the two dumps they refer to):
    * Intel P8051
    * 27C256 EPROM
    * NEC D446C-2 SRAM (2K x 8, battery backed)
    * OKI M62X428 RTC (MSM6242 compatible)
    * Rockwell 10937P-50 A8201-17 VFD controller (16 characters)
    * Azkoyen L66S coin selector with PIC16C76/PIC16F76 (undumped)

    The crystal frequency is not documented.  6 MHz is assumed, the same as
    the related T61 board below.  Both firmware dumps use exactly the same
    I/O map and timer settings, so the same hardware is assumed for both.

    Confirmed from the firmware:

    * P1.0-P1.3: key matrix column select (one hot, active high)
    * P1.4: never used
    * P1.5: selects external data space: 0 = I/O, 1 = SRAM.  The timer
      interrupt saves P1 and clears P1.5 before accessing I/O.
    * P1.6: driven low around SRAM writes and high afterwards (write
      enable, assumption)
    * P1.7: pulsed high/low at the start of most routines (watchdog kick,
      assumption; the watchdog is not emulated)
    * P2 is always 0 when I/O is accessed through MOVX @Ri
    * P3.3 (INT1): polled; must be high for a paid vend to start (with
      credit and P3.3 low, pressing a selection only shows the price).
      Function unknown.
    * designe only: the UART is initialised (mode 1, timer 1 reload 0xf4,
      about 1302 baud at 6 MHz) but SBUF is never accessed; the
      "TRANSMISION" strings are not referenced.

    External data space with P1.5 = 1: SRAM, firmware uses 0x0000-0x03ff.

    External data space with P1.5 = 0 (P2 = 0):
    * 0x00-0x07 w: 8-bit addressable latch, data on D0 (CD4099, as fitted on
      the T61 board, assumption)
      - 0: hopper 1 motor
      - 1: hopper 2 motor
      - 2: hopper 3 motor
      - 3: set while the machine is not out of service, cleared in control
        mode (coin selector enable, assumption)
      - 4: active during the coin return sequence; the firmware waits for
        code 0xe on the coin bus while it is on (escrow to return chute,
        assumption)
      - 5: active after a sale; same handling as 4 (escrow to cash box,
        assumption)
      - 6: recovery motor; runs until IN1 bit 6 has gone high and back low
        ("AVERIA RECUP." when this fails)
      - 7: pulsed for two timer ticks after a valid coin code is read;
        the firmware then waits for the coin selector to send the same code
        again before giving credit (accept handshake)
    * 0x00 r:
      - 0-3: coin code from the coin selector (0 = idle, 0xf ignored)
      - 4: designe: part of the idle check that clears a coin selector
        fault, otherwise ignored
      - 5: designe: coin code bit 4 (5-bit coin codes)
      - 6: service/control switch "CS" (1 = control)
      - 7: hopper 2 full (active low, assumption about the meaning)
    * 0x10-0x17 w: addressable latch
      - 0-2: coin sorter gates; combinations route coins to the hoppers
      - 3-4: never used
      - 5: VFD data
      - 6: VFD clock
      - 7: VFD reset (active low)
    * 0x10 r:
      - 0-2: coin exit sensors of hoppers 3, 2 and 1 (design6: high while a
        coin passes; designe: polarity selected by the model preset)
      - 3-5: coin level of hoppers 3, 2 and 1 (0 = empty, "VACIO DEVOL.")
      - 6: recovery motor position (1 = away from rest).  designe: a
        short high pulse while idle enables sales in the adult access
        modes (see remote_pressed() below)
      - 7: hopper 1 full (active low, assumption about the meaning)
    * 0x20-0x47 w: three addressable latches, 24 extractor motor outputs.
      ROM tables map product channels to latch addresses.
    * 0x20 r: key matrix row (active high)
    * 0x30 r:
      - 0-2: product flaps ("trampillas") 1-3, low while a product lifts
        the flap ("AVERIA TRAMP." when stuck)
      - 3: hopper 3 full (active low, assumption about the meaning)
    * 0x60-0x6f r/w: RTC
    * 0x70 w: message number (0 = no change available, 1 = sold out,
      2 = after a completed sale).  Probably the optional voice
      synthesizer kit listed in the manual (assumption).

    Key matrix (key = column * 8 + row), from the ROM tables:
    * key 0: coin return button
    * keys 1-9 and 16-25: selection buttons (which key drives which
      selection depends on the model preset in designe)
    * keys 28-31: operator keypad A, B, C and D

    Coin codes, from the ROM tables:
    * design6: 1 = 500, 2/8 = 5, 3 = 10, 4/11 = 200, 5/9 = 25, 6/10 = 50,
      7 = 100 pesetas.  The operator chooses whether codes 2/5, 8/9 or
      both feed the hoppers ("ANTIGUA", "NUEVAS", "NUE. VIE." settings),
      so 2/5 are presumably the old 5 and 25 peseta coins and 8/9 the new
      ones (assumption).  The hoppers hold 5, 25 and 100 peseta coins.
    * designe: 0x13-0x18 = 0.05, 0.10, 0.20, 0.50, 1 and 2 euro,
      0x0c = token ("FICHA").  Depending on the "ACCESO ADULTO" setting,
      coins are refused ("ADULTO") until a token or the adult access
      remote enables the sale.  The hopper coin values are selectable:
      0.10-0.20-0.50, 0.05-0.10-0.50 or 0.05-0.20-0.50 euro.

    The operator manual available (models D14 and D21, pesetas, 7/94,
    adapted for euro) describes a service switch "CS", an independent
    A/B/C/D keypad, a coin return button, three hoppers, up to three
    product flaps, an optional voice synthesizer kit and an L60 coin
    selector.  It does not necessarily match the dumped firmware: for
    example it lists 0.05-0.10-0.20 instead of 0.10-0.20-0.50 as a hopper
    configuration.

    Usage: turn on the service switch (CS) to enter control mode and walk
    through the menus with the coin return button; the keypad keys A-D are
    used to program (the first key pressed after choosing a selection
    clears its price).  A fresh machine shows "DESPROGRAMADA" until the
    prices are programmed and, on designe, a model preset is chosen in the
    CONFIGURACION menu (D shows the current one, B steps through the
    presets, D pressed four times confirms).  On designe, press the adult
    access remote before inserting coins, or change the "ACCESO ADULTO"
    setting.

    TODO:
    - The coin selector is simulated (see coin_inserted() below).  The
      protocol timings and the meaning of code 0xe are unknown.
    - Hoppers, product flaps and the recovery motor are simulated with
      made-up timings; empty and full hopper sensors are manual toggles.
    - Voice synthesizer kit
    - Watchdog
    - Verify the assumptions above on real hardware
*/

#include "emu.h"

#include "cpu/mcs48/mcs48.h"
#include "cpu/mcs51/i8051.h"
#include "machine/74259.h"
#include "machine/i8279.h"
#include "machine/msm5832.h"
#include "machine/msm6242.h"
#include "machine/nvram.h"
#include "machine/ticket.h"
#include "video/roc10937.h"

#include "design6.lh"
#include "designe.lh"

#define LOG_COINSEL (1U << 1)
#define LOG_VOICE   (1U << 2)

//#define VERBOSE (LOG_GENERAL | LOG_COINSEL | LOG_VOICE)
#include "logmacro.h"

#define LOGCOINSEL(...) LOGMASKED(LOG_COINSEL, __VA_ARGS__)
#define LOGVOICE(...)   LOGMASKED(LOG_VOICE, __VA_ARGS__)


namespace {

class design_state : public driver_device
{
public:
	design_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_hopper(*this, "hopper%u", 1U)
		, m_xdata_view(*this, "xdata")
		, m_io_keys(*this, "KEY%u", 0U)
		, m_io_flaps(*this, "FLAPS")
		, m_io_conf(*this, "CONF")
		, m_out_hopper_motor(*this, "hopper_motor%u", 1U)
		, m_out_enable(*this, "enable")
		, m_out_escrow_return(*this, "escrow_return")
		, m_out_escrow_collect(*this, "escrow_collect")
		, m_out_recovery_motor(*this, "recovery_motor")
		, m_out_coin_accept(*this, "coin_accept")
		, m_out_sorter(*this, "sorter_gate%u", 1U)
		, m_out_extractor(*this, "extractor%u", 1U)
		, m_out_flap(*this, "flap")
	{
	}

	void design6(machine_config &config) ATTR_COLD;
	void designe(machine_config &config) ATTR_COLD;

	DECLARE_INPUT_CHANGED_MEMBER(coin_inserted);
	DECLARE_INPUT_CHANGED_MEMBER(remote_pressed);

	ioport_value coin_code_r() { return (m_coinsel_code & 0x0f) | (BIT(m_coinsel_code, 4) << 5); }
	template <unsigned N> int hopper_sensor_r();
	int recovery_position_r() { return m_recovery_pos || m_remote_pulse; }
	ioport_value flaps_r();

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	enum coinsel_state : u8
	{
		COINSEL_IDLE,
		COINSEL_VALIDATE,
		COINSEL_TRAVEL,
		COINSEL_CONFIRM
	};

	required_device<i8051_device> m_maincpu;
	required_device_array<hopper_device, 3> m_hopper;
	memory_view m_xdata_view;
	required_ioport_array<4> m_io_keys;
	required_ioport m_io_flaps;
	optional_ioport m_io_conf;
	output_finder<3> m_out_hopper_motor;
	output_finder<> m_out_enable;
	output_finder<> m_out_escrow_return;
	output_finder<> m_out_escrow_collect;
	output_finder<> m_out_recovery_motor;
	output_finder<> m_out_coin_accept;
	output_finder<3> m_out_sorter;
	output_finder<24> m_out_extractor;
	output_finder<> m_out_flap;

	emu_timer *m_coinsel_timer = nullptr;
	emu_timer *m_recovery_timer = nullptr;
	emu_timer *m_flap_timer = nullptr;
	emu_timer *m_remote_timer = nullptr;

	u8 m_port1 = 0xff;
	u8 m_coinsel_state = COINSEL_IDLE;
	u8 m_coinsel_coin = 0;
	u8 m_coinsel_code = 0;
	bool m_coinsel_accepted = false;
	bool m_recovery_motor = false;
	bool m_recovery_pos = false;
	u32 m_extractors = 0;
	bool m_flap_raised = false;
	bool m_remote_pulse = false;

	void program_map(address_map &map) ATTR_COLD;
	void data_map(address_map &map) ATTR_COLD;

	void port1_w(u8 data);
	u8 keys_r();
	void voice_w(u8 data);

	template <unsigned N> void hopper_motor_w(int state);
	void enable_w(int state);
	void escrow_return_w(int state);
	void escrow_collect_w(int state);
	void recovery_motor_w(int state);
	void coin_accept_w(int state);
	template <unsigned N> void sorter_w(int state);
	template <unsigned N> void extractors_w(u8 data);

	TIMER_CALLBACK_MEMBER(coinsel_update);
	TIMER_CALLBACK_MEMBER(recovery_update);
	TIMER_CALLBACK_MEMBER(flap_update);
	TIMER_CALLBACK_MEMBER(remote_release);
};


class azkoyent_state : public driver_device
{
public:
	azkoyent_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
	{
	}

	void azkoyent(machine_config &config) ATTR_COLD;
	void azkoyent61(machine_config &config) ATTR_COLD;

private:
	required_device<cpu_device> m_maincpu;
};



/**************************************************************************
    Address maps
**************************************************************************/

void design_state::program_map(address_map &map)
{
	map(0x0000, 0x7fff).rom().region("maincpu", 0);
}

void design_state::data_map(address_map &map)
{
	map(0x0000, 0xffff).view(m_xdata_view);

	// P1.5 = 0: I/O (the firmware only generates addresses 0x00xx here)
	m_xdata_view[0](0x0000, 0x0000).portr("IN0");
	m_xdata_view[0](0x0000, 0x0007).w("outlatch0", FUNC(cd4099_device::write_d0));
	m_xdata_view[0](0x0010, 0x0010).portr("IN1");
	m_xdata_view[0](0x0010, 0x0017).w("outlatch1", FUNC(cd4099_device::write_d0));
	m_xdata_view[0](0x0020, 0x0020).r(FUNC(design_state::keys_r));
	m_xdata_view[0](0x0020, 0x0027).w("outlatch2", FUNC(cd4099_device::write_d0));
	m_xdata_view[0](0x0030, 0x0030).portr("IN3");
	m_xdata_view[0](0x0030, 0x0037).w("outlatch3", FUNC(cd4099_device::write_d0));
	m_xdata_view[0](0x0040, 0x0047).w("outlatch4", FUNC(cd4099_device::write_d0));
	m_xdata_view[0](0x0060, 0x006f).rw("rtc", FUNC(msm6242_device::read), FUNC(msm6242_device::write));
	m_xdata_view[0](0x0070, 0x0070).w(FUNC(design_state::voice_w));

	// P1.5 = 1: 2K SRAM (address decoding above A10 unknown)
	m_xdata_view[1](0x0000, 0x07ff).mirror(0xf800).ram().share("nvram");
}



/**************************************************************************
    Input ports
**************************************************************************/

static INPUT_PORTS_START( design )
	PORT_START("IN0")
	PORT_BIT(0x2f, IP_ACTIVE_HIGH, IPT_CUSTOM) PORT_CUSTOM_MEMBER(FUNC(design_state::coin_code_r))
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_SERVICE) PORT_TOGGLE PORT_NAME("Service/Control Switch (CS)")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("Hopper 2 Full")

	PORT_START("IN1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_CUSTOM) PORT_READ_LINE_MEMBER(FUNC(design_state::hopper_sensor_r<2>))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_CUSTOM) PORT_READ_LINE_MEMBER(FUNC(design_state::hopper_sensor_r<1>))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_CUSTOM) PORT_READ_LINE_MEMBER(FUNC(design_state::hopper_sensor_r<0>))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("Hopper 3 Empty")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("Hopper 2 Empty")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("Hopper 1 Empty")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_CUSTOM) PORT_READ_LINE_MEMBER(FUNC(design_state::recovery_position_r))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("Hopper 1 Full")

	PORT_START("IN3")
	PORT_BIT(0x07, IP_ACTIVE_HIGH, IPT_CUSTOM) PORT_CUSTOM_MEMBER(FUNC(design_state::flaps_r))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("Hopper 3 Full")
	PORT_BIT(0xf0, IP_ACTIVE_HIGH, IPT_UNUSED)

	// the name describes what the firmware does with P3.3, its function is unknown
	PORT_START("P3")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_TOGGLE PORT_NAME("Vend Inhibit (P3.3)")
	PORT_BIT(0xf7, IP_ACTIVE_LOW, IPT_UNUSED)

	// manual control of the product flap sensors, in addition to the simulation
	PORT_START("FLAPS")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("Product Flap 1")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("Product Flap 2")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("Product Flap 3")

	PORT_START("KEY0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_B) PORT_NAME("Coin Return")
	PORT_BIT(0xfe, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEY1")
	PORT_BIT(0xff, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEY2")
	PORT_BIT(0xff, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("KEY3")
	PORT_BIT(0x0f, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_Z) PORT_NAME("Keypad A")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_X) PORT_NAME("Keypad B")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_C) PORT_NAME("Keypad C")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_V) PORT_NAME("Keypad D")
INPUT_PORTS_END

static INPUT_PORTS_START( design6 )
	PORT_INCLUDE(design)

	// only keys 1-6 are mapped to selections
	PORT_MODIFY("KEY0")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_Q) PORT_NAME("Selection 1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_W) PORT_NAME("Selection 2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_E) PORT_NAME("Selection 3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_R) PORT_NAME("Selection 4")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_T) PORT_NAME("Selection 5")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_Y) PORT_NAME("Selection 6")

	// the parameter is the code sent by the coin selector
	PORT_START("COINS")
	PORT_BIT(0x001, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("5 Pesetas (code 8)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 8)
	PORT_BIT(0x002, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("10 Pesetas (code 3)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 3)
	PORT_BIT(0x004, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("25 Pesetas (code 9)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 9)
	PORT_BIT(0x008, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("50 Pesetas (code 6)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 6)
	PORT_BIT(0x010, IP_ACTIVE_HIGH, IPT_COIN5) PORT_NAME("100 Pesetas (code 7)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 7)
	PORT_BIT(0x020, IP_ACTIVE_HIGH, IPT_COIN6) PORT_NAME("200 Pesetas (code 4)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 4)
	PORT_BIT(0x040, IP_ACTIVE_HIGH, IPT_COIN7) PORT_NAME("500 Pesetas (code 1)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 1)
	PORT_BIT(0x080, IP_ACTIVE_HIGH, IPT_COIN8) PORT_NAME("5 Pesetas (code 2)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 2)
	PORT_BIT(0x100, IP_ACTIVE_HIGH, IPT_COIN9) PORT_NAME("25 Pesetas (code 5)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 5)
	PORT_BIT(0x200, IP_ACTIVE_HIGH, IPT_COIN10) PORT_NAME("50 Pesetas (code 10)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 10)
	PORT_BIT(0x400, IP_ACTIVE_HIGH, IPT_COIN11) PORT_NAME("200 Pesetas (code 11)") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 11)
INPUT_PORTS_END

static INPUT_PORTS_START( designe )
	PORT_INCLUDE(design)

	// key names follow the D21 preset (keys 1-9 and 16-25 are selections 1-19)
	PORT_MODIFY("KEY0")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_Q) PORT_NAME("Selection 1")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_W) PORT_NAME("Selection 2")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_E) PORT_NAME("Selection 3")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_R) PORT_NAME("Selection 4")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_T) PORT_NAME("Selection 5")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_Y) PORT_NAME("Selection 6")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_U) PORT_NAME("Selection 7")

	PORT_MODIFY("KEY1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_I) PORT_NAME("Selection 8")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_O) PORT_NAME("Selection 9")

	PORT_MODIFY("KEY2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_A) PORT_NAME("Selection 10")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_S) PORT_NAME("Selection 11")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_D) PORT_NAME("Selection 12")
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_F) PORT_NAME("Selection 13")
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_G) PORT_NAME("Selection 14")
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_H) PORT_NAME("Selection 15")
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_J) PORT_NAME("Selection 16")
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_K) PORT_NAME("Selection 17")

	PORT_MODIFY("KEY3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_L) PORT_NAME("Selection 18")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_COLON) PORT_NAME("Selection 19")

	// pulses IN1 bit 6, see remote_pressed()
	PORT_START("REMOTE")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_OTHER) PORT_CODE(KEYCODE_M) PORT_NAME("Adult Access Remote") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::remote_pressed), 0)

	PORT_START("COINS")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_COIN1) PORT_NAME("0.05 Euro") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 0x13)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_COIN2) PORT_NAME("0.10 Euro") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 0x14)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_COIN3) PORT_NAME("0.20 Euro") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 0x15)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_COIN4) PORT_NAME("0.50 Euro") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 0x16)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN5) PORT_NAME("1 Euro") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 0x17)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_COIN6) PORT_NAME("2 Euro") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 0x18)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_COIN7) PORT_NAME("Token") PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(design_state::coin_inserted), 0x0c)

	// bit 0 of the model preset flags selects the hopper exit sensor polarity
	PORT_START("CONF")
	PORT_CONFNAME(0x01, 0x00, "Hopper Exit Sensors")
	PORT_CONFSETTING(   0x01, "Active High (D6, D8, D10/D12 presets)")
	PORT_CONFSETTING(   0x00, "Active Low (other presets)")
INPUT_PORTS_END

static INPUT_PORTS_START( azkoyent )
INPUT_PORTS_END



/**************************************************************************
    Machine emulation
**************************************************************************/

void design_state::port1_w(u8 data)
{
	// 0-3: key matrix column select
	// 5: external data space select
	// 6: SRAM write enable (assumption, not emulated)
	// 7: watchdog (assumption, not emulated)
	m_port1 = data;
	m_xdata_view.select(BIT(data, 5));
}

u8 design_state::keys_r()
{
	u8 data = 0;

	for (unsigned i = 0; i < 4; i++)
		if (BIT(m_port1, i))
			data |= m_io_keys[i]->read();

	return data;
}

void design_state::voice_w(u8 data)
{
	LOGVOICE("%s: message %u\n", machine().describe_context(), data);
}

template <unsigned N>
int design_state::hopper_sensor_r()
{
	const int state = m_hopper[N]->line_r();
	return (m_io_conf.found() && !BIT(m_io_conf->read(), 0)) ? !state : state;
}

ioport_value design_state::flaps_r()
{
	return m_io_flaps->read() & (m_flap_raised ? 0 : 7);
}

template <unsigned N>
void design_state::hopper_motor_w(int state)
{
	m_out_hopper_motor[N] = state;
	m_hopper[N]->motor_w(state);
}

void design_state::enable_w(int state)
{
	m_out_enable = state;
}

void design_state::escrow_return_w(int state)
{
	m_out_escrow_return = state;
}

void design_state::escrow_collect_w(int state)
{
	m_out_escrow_collect = state;
}

template <unsigned N>
void design_state::sorter_w(int state)
{
	m_out_sorter[N] = state;
}

template <unsigned N>
void design_state::extractors_w(u8 data)
{
	const u32 old = m_extractors;
	m_extractors = (m_extractors & ~(0xffU << (N * 8))) | (u32(data) << (N * 8));

	for (unsigned i = 0; i < 8; i++)
		m_out_extractor[N * 8 + i] = BIT(data, i);

	// (simulated) a product lifts the flap a while after a motor starts
	if (!old && m_extractors)
		m_flap_timer->adjust(attotime::from_msec(500));
	else if (old && !m_extractors && !m_flap_raised)
		m_flap_timer->adjust(attotime::never);
}

TIMER_CALLBACK_MEMBER(design_state::flap_update)
{
	m_flap_raised = !m_flap_raised;
	m_out_flap = m_flap_raised;

	if (m_flap_raised)
		m_flap_timer->adjust(attotime::from_msec(200));
	else if (m_extractors)
		m_flap_timer->adjust(attotime::from_msec(500));
}

void design_state::recovery_motor_w(int state)
{
	m_out_recovery_motor = state;

	if (bool(state) == m_recovery_motor)
		return;

	m_recovery_motor = state;

	// (simulated) the cam switch is away from rest for 400 ms per turn
	if (m_recovery_motor)
		m_recovery_timer->adjust(attotime::from_msec(m_recovery_pos ? 400 : 150));
	else
		m_recovery_timer->adjust(attotime::never);
}

TIMER_CALLBACK_MEMBER(design_state::recovery_update)
{
	m_recovery_pos = !m_recovery_pos;
	m_recovery_timer->adjust(attotime::from_msec(m_recovery_pos ? 400 : 150));
}

/*
    designe only: while idle, the firmware treats a high pulse of about
    10 to 45 timer ticks on IN1 bit 6 as the activation for adult access
    ("ACCESO ADULTO" set to "MANDO" or "FICHA+MANDO"; "MANDO" is the default
    after the memory is initialised).  Longer high levels are handled as
    the recovery motor position.  The adult access remote control receiver
    is presumably connected to this input (assumption).
*/

INPUT_CHANGED_MEMBER(design_state::remote_pressed)
{
	if (newval && !m_remote_pulse)
	{
		m_remote_pulse = true;
		m_remote_timer->adjust(attotime::from_msec(30));
	}
}

TIMER_CALLBACK_MEMBER(design_state::remote_release)
{
	m_remote_pulse = false;
}


/*
    Coin selector (simulated)

    The firmware expects the following sequence for every coin:
    1. the selector puts the coin code on the bus (validation)
    2. if the code is valid, the CPU pulses latch 0 bit 7 (accept)
    3. the code must return to 0
    4. the selector sends the same code again (coin accepted)
    5. the code must return to 0, then credit is given

    Pulse lengths and delays are unknown and chosen to satisfy the firmware
    time-outs.  Coins that are not accepted are returned.
*/

INPUT_CHANGED_MEMBER(design_state::coin_inserted)
{
	if (!newval || (m_coinsel_state != COINSEL_IDLE))
		return;

	LOGCOINSEL("coin inserted, code %02x\n", param);

	m_coinsel_coin = param;
	m_coinsel_accepted = false;
	m_coinsel_state = COINSEL_VALIDATE;
	m_coinsel_code = m_coinsel_coin;
	m_coinsel_timer->adjust(attotime::from_msec(50));
}

void design_state::coin_accept_w(int state)
{
	m_out_coin_accept = state;

	if (state && (m_coinsel_state == COINSEL_VALIDATE))
	{
		LOGCOINSEL("coin accepted by CPU\n");
		m_coinsel_accepted = true;
	}
}

TIMER_CALLBACK_MEMBER(design_state::coinsel_update)
{
	switch (m_coinsel_state)
	{
	case COINSEL_VALIDATE:
		m_coinsel_code = 0;
		if (m_coinsel_accepted)
		{
			m_coinsel_state = COINSEL_TRAVEL;
			m_coinsel_timer->adjust(attotime::from_msec(100));
		}
		else
		{
			LOGCOINSEL("coin rejected\n");
			m_coinsel_state = COINSEL_IDLE;
		}
		break;

	case COINSEL_TRAVEL:
		m_coinsel_code = m_coinsel_coin;
		m_coinsel_state = COINSEL_CONFIRM;
		m_coinsel_timer->adjust(attotime::from_msec(50));
		break;

	case COINSEL_CONFIRM:
		m_coinsel_code = 0;
		m_coinsel_state = COINSEL_IDLE;
		break;

	default:
		m_coinsel_code = 0;
		m_coinsel_state = COINSEL_IDLE;
		break;
	}
}


void design_state::machine_start()
{
	m_coinsel_timer = timer_alloc(FUNC(design_state::coinsel_update), this);
	m_recovery_timer = timer_alloc(FUNC(design_state::recovery_update), this);
	m_flap_timer = timer_alloc(FUNC(design_state::flap_update), this);
	m_remote_timer = timer_alloc(FUNC(design_state::remote_release), this);

	save_item(NAME(m_port1));
	save_item(NAME(m_coinsel_state));
	save_item(NAME(m_coinsel_coin));
	save_item(NAME(m_coinsel_code));
	save_item(NAME(m_coinsel_accepted));
	save_item(NAME(m_recovery_motor));
	save_item(NAME(m_recovery_pos));
	save_item(NAME(m_extractors));
	save_item(NAME(m_flap_raised));
	save_item(NAME(m_remote_pulse));
}

void design_state::machine_reset()
{
	// port latches are set to 0xff on reset
	m_port1 = 0xff;
	m_xdata_view.select(1);

	m_coinsel_state = COINSEL_IDLE;
	m_coinsel_code = 0;
	m_coinsel_timer->adjust(attotime::never);
}



/**************************************************************************
    Machine configuration
**************************************************************************/

void design_state::design6(machine_config &config)
{
	I8051(config, m_maincpu, 6_MHz_XTAL); // XTAL not documented
	m_maincpu->set_addrmap(AS_PROGRAM, &design_state::program_map);
	m_maincpu->set_addrmap(AS_DATA, &design_state::data_map);
	m_maincpu->port_out_cb<1>().set(FUNC(design_state::port1_w));
	m_maincpu->port_in_cb<3>().set_ioport("P3");

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);

	cd4099_device &outlatch0(CD4099(config, "outlatch0"));
	outlatch0.q_out_cb<0>().set(FUNC(design_state::hopper_motor_w<0>));
	outlatch0.q_out_cb<1>().set(FUNC(design_state::hopper_motor_w<1>));
	outlatch0.q_out_cb<2>().set(FUNC(design_state::hopper_motor_w<2>));
	outlatch0.q_out_cb<3>().set(FUNC(design_state::enable_w));
	outlatch0.q_out_cb<4>().set(FUNC(design_state::escrow_return_w));
	outlatch0.q_out_cb<5>().set(FUNC(design_state::escrow_collect_w));
	outlatch0.q_out_cb<6>().set(FUNC(design_state::recovery_motor_w));
	outlatch0.q_out_cb<7>().set(FUNC(design_state::coin_accept_w));

	cd4099_device &outlatch1(CD4099(config, "outlatch1"));
	outlatch1.q_out_cb<0>().set(FUNC(design_state::sorter_w<0>));
	outlatch1.q_out_cb<1>().set(FUNC(design_state::sorter_w<1>));
	outlatch1.q_out_cb<2>().set(FUNC(design_state::sorter_w<2>));
	outlatch1.q_out_cb<5>().set("vfd", FUNC(roc10937_device::data));
	outlatch1.q_out_cb<6>().set("vfd", FUNC(roc10937_device::sclk));
	outlatch1.q_out_cb<7>().set("vfd", FUNC(roc10937_device::por));

	CD4099(config, "outlatch2").parallel_out_cb().set(FUNC(design_state::extractors_w<0>));
	CD4099(config, "outlatch3").parallel_out_cb().set(FUNC(design_state::extractors_w<1>));
	CD4099(config, "outlatch4").parallel_out_cb().set(FUNC(design_state::extractors_w<2>));

	// (simulated) coin hoppers
	for (auto &hopper : m_hopper)
		HOPPER(config, hopper, attotime::from_msec(100));

	ROC10937(config, "vfd");

	MSM6242(config, "rtc", 32.768_kHz_XTAL);

	config.set_default_layout(layout_design6);
}

void design_state::designe(machine_config &config)
{
	design6(config);

	config.set_default_layout(layout_designe);
}


void azkoyent_state::azkoyent(machine_config &config)
{
	I8039(config, m_maincpu, 6.144_MHz_XTAL);
	I8279(config, "i8279", 6.144_MHz_XTAL); // Unknown clock
}

void azkoyent_state::azkoyent61(machine_config &config)
{
	I8051(config, m_maincpu, 6_MHz_XTAL);
	I8279(config, "i8279", 6_MHz_XTAL); // Unknown clock
	MSM5832(config, "rtc", 6_MHz_XTAL); // Unknown clock, has its own oscillator (unknown frequency)
}



/**************************************************************************
    ROM definitions
**************************************************************************/

ROM_START( design6 )
	ROM_REGION(0x8000, "maincpu", 0)
	ROM_LOAD("1.bin", 0x0000, 0x8000, CRC(1155999c) SHA1(2896af89011c496f905ed0e57d7035a3b612c718))

	ROM_REGION(0x4000, "coinsel", 0)
	ROM_LOAD("pic16x76_l56s-l66s.bin", 0x0000, 0x4000, NO_DUMP)
ROM_END

ROM_START( designe )
	ROM_REGION(0x8000, "maincpu", 0)
	ROM_LOAD("designe.bin", 0x0000, 0x8000, CRC(693d40bd) SHA1(9596bbf9c367bc919393923460da15563d9447ca))

	ROM_REGION(0x4000, "coinsel", 0)
	ROM_LOAD("pic16x76_l56s-l66s.bin", 0x0000, 0x4000, NO_DUMP)
ROM_END


// Different Azkoyen tobacco vending machines on similar hardware

/* Azkoyen models T6, T8, and T12 (Azkoyen PCB 104-4455-02-80/1). MCS-48-based.
  ___________________________________________________________
 |                                       __________         |
_|_           ___                       | BATT    |        _|_
_|_         LM555CN                     |_________|        _|_
 |   ___          ____________________                     _|_
 |  BDX53A  Xtal | PCB 80C39 11P     |    __________       _|_
 |     6.144 MHz |___________________|   |_MC14069U|       _|_
_|_                                          _____________ _|_
_|_               ____________________      | EPROM      |  |
_|_              | NEC D8279C-5      |      |____________| =|
 |        ___    |___________________|       __________    =|
 |=      |..|                               |M74HC373B1    =|
 |=      |..|                                _________     =|
 |=      |..|                               |TC4011BP|     =|
 |=      |..|                        ___     ___            |
 |       |..|              TC4011BP->|  |    |  |          =|
 |=      |..|                        |  |    |  <-TC4011BP =|
 |=      |..|                        |  |    |  |          =|
 |                                   |__|    |__|          =|
 |__________________________________________________________|

*/

// T6 uses a 4 digits 7-segments display.
ROM_START( azkoyent6 )
	ROM_REGION(0x2000, "maincpu", 0)
	ROM_LOAD("43504560-0_t-6.u04",   0x0000, 0x2000, CRC(a4289b26) SHA1(40587094b11c6cf9308673ffac2ed9d445d458e9))
ROM_END

// T8 uses a 3 digits 7-segments display.
ROM_START( azkoyent8 )
	ROM_REGION(0x2000, "maincpu", 0)
	ROM_LOAD("43504570-2_t8_3.u04",  0x0000, 0x2000, CRC(76ac54bf) SHA1(da4c4a9f1c9c85d59169d62682bb7b73a9dd133b))
ROM_END

ROM_START( azkoyent12 )
	ROM_REGION(0x2000, "maincpu", 0)
	ROM_LOAD("43504580-0_t12-17.u04", 0x0000, 0x2000, CRC(10d4d4a7) SHA1(96804bc173abf2d51de7e7f84decba286916eba7))
ROM_END

/* Azkoyen model T61 (with OKI M5832 RTC, Azkoyen PCB 131000060-1). MCS-51-based. Unknown display.

  ___|||_||||||||||____________________________________
 |   ||| ||||||||||          ||||||||   |||||||||||   |
 | _____________                    _____             |
 ||::::::::::::|     __________     ·····             |
 |                  |ULN2803A_|                       |
 |      __________   __________   __________         =|
 |     |CD4099BCN|  |CD4099BCN|  |CF74HC240E         =|
 |                                                   =|
 |  L7805CV          __________                      =|
 |                  |_UM6104__|                      =|
 |      __________   __________   __________          |
 |     |_TC4011BP|  |_TC4071BP|  |CD4099BCN|          |
 |      __________   __________   __________          |
 |     |_TC4011BP|  |GD74HC138|  |TC4099BP_|<-Not present on some versions
 |            ______________      __________         =|
 |           | EPROM       |     |TC4099BP_|<-Not present on some versions
 |           |_____________|                         =|
 |          ___   __________                         =|
 |       LM555CN |TC4069UBP|                         =|
 | ______        ___________           ___________    |
 || BATT|       |MM74HC373N|          |TD62083AP_|    |
 ||_____|        ___________                          |
 |              |MM74HC373N|    _____________         |
 | Osc                         |::::::::::::|         |
 | xxx MHz      _________________   _________________ |
 | __________  | Intel P80C51AH |  | NEC D8279C-2   | |
 ||OKI_M5832|  |________________|  |________________| |
 |                ____     Xtal     __________        |
 |                BDX53  6.000 MHz |SN74HC240N        |
 |                                                    |
 |_____________|_|____|_|__|__|||_|||||||||___||||____|

*/

ROM_START( azkoyent61 )
	ROM_REGION(0x1000, "maincpu", 0)
	ROM_LOAD("t-61.u4",       0x0000, 0x1000, CRC(16d9b843) SHA1(7c6f177eca9163b5284d2cbe1bdeb3b0bf1a6698))
ROM_END

ROM_START( azkoyent61a )
	ROM_REGION(0x1000, "maincpu", 0)
	ROM_LOAD("t-61-6_t-m.u4", 0x0000, 0x1000, CRC(ce1ed720) SHA1(42cb78ddd8d06764599e97b72b557d164940f7df))
ROM_END

} // anonymous namespace



/**************************************************************************
    System drivers
**************************************************************************/

//    YEAR   NAME         PARENT      COMPAT  MACHINE     INPUT     CLASS           INIT        COMPANY    FULLNAME                                       FLAGS
SYST( 1995?, design6,     0,          0,      design6,    design6,  design_state,   empty_init, "Azkoyen", "Design D6 (pesetas)",                         MACHINE_SUPPORTS_SAVE | MACHINE_NO_SOUND_HW | MACHINE_NOT_WORKING )
SYST( 2006,  designe,     0,          0,      designe,    designe,  design_state,   empty_init, "Azkoyen", "Design (euro, 43521600-5)",                   MACHINE_SUPPORTS_SAVE | MACHINE_NO_SOUND_HW | MACHINE_NOT_WORKING )

SYST( 19??,  azkoyent6,   0,          0,      azkoyent,   azkoyent, azkoyent_state, empty_init, "Azkoyen", "Vending machine model T6",                    MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
SYST( 19??,  azkoyent8,   0,          0,      azkoyent,   azkoyent, azkoyent_state, empty_init, "Azkoyen", "Vending machine model T8",                    MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
SYST( 19??,  azkoyent12,  0,          0,      azkoyent,   azkoyent, azkoyent_state, empty_init, "Azkoyen", "Vending machine model T12",                   MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
SYST( 19??,  azkoyent61,  0,          0,      azkoyent61, azkoyent, azkoyent_state, empty_init, "Azkoyen", "Vending machine model T61 (set 1)",           MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
SYST( 19??,  azkoyent61a, azkoyent61, 0,      azkoyent61, azkoyent, azkoyent_state, empty_init, "Azkoyen", "Vending machine model T61 (set 2)",           MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
