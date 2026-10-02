// license:BSD-3-Clause
// copyright-holders:David Haywood, Roberto Fresca
/*************************************************************************

  Cuatro en Linea.
  System I.
  1991, Compumatic

  Driver by David Haywood & Roberto Fresca.

**************************************************************************

  PCB V487-02 (marked on the solder side).

  1x Z84C00AB6 (Z80A) CPU @ 8 MHz for video (see notes).
  1x Z84C00HB6 (Z80H) CPU @ 4 MHz for master/sound (IC20).
  1x AY-3-8910A (Microchip AY38910A/P).
  1x UMC UM487F (HCGA Controller)

  2x NEC D41464C (64K x 4-bit Dynamic NMOS RAM) for VRAM.
  1x UMC UM6264A (8K x 8-bit CMOS SRAM).

  2x 27C256 EPROMs (IC6 video, IC19 master).
  1x X24C16P Serial EEPROM (IC17).

  1x GAL16V8AS
  1x 74HC245, 2x 74HC244 (IC9, IC10), 1x 74LS32, 1x 74LS04.

  1x ES2 CM3080 (unknown DIP-18 IC)
  1x ES2 9046 (unknown PLCC-84 IC, IC21)
  1x 8952 CM 32 (unknown DIP-40 IC, IC26)

  1x 16.0000 MHz crystal. ; Divided by 2 (through CM3080) for the video CPU.
  1x 8.000 MHz crystal.   ; (XT3) Divided by 2 for the master CPU.
  1x 14.31818 MHz crystal ; For HCGA controller.

  CN1: 1 x 8 connector.
  CN2: 1 x 8 connector.
  CN3: 2 x 5 connector.
  CN4: 2 x 5 connector.
  CN5: 2 x 5 connector.
  CN6: 2 x 28 Jamma connector.
  CN7: 1 x 20 connector.
  CN8: 1 x 4 connector.
  CN9: 1 x 4 connector.
  CN10: DB9 video out connector.
  CN11 1 x 2 bridge connector.

**************************************************************************

  UM487F HCGA Controller notes...

  The UM487F is a single chip MGA (Hercules) + CGA video controller with
  an embedded UM6845R CRTC, driving 64 KB of DRAM (2x D41464C).

  The 14.31818 MHz crystal is tied to pin 65 (OSC, CGA base clock), while
  pin 64 (MOSC, the 16.257 MHz MGA base clock) is tied to GND, so the chip
  can only work in CGA mode. The 8 MHz CPU clock entering pin 1 (CLOCK)
  only generates the enable signal of the CPU interface of the embedded
  6845, it isn't used for the video timing.

  The video CPU sees the CGA video memory window (B8000h-BBFFFh on a PC,
  where A15 = 1 selects it) at 8000h-BFFFh, and the registers at the usual
  CGA I/O ports (the UM487F decodes only A0-A9 for I/O):

  3BFh: -W  Configuration register.
  3D4h: -W  CRTC index register.
  3D5h: RW  CRTC data register.
  3D8h: -W  Mode control register.
  3D9h: -W  Color select register.
  3DAh: R-  Status register.

  All the games initialize the controller the same way:

  Config Register (3BFh): 0x40
  (bit 6 active means CGA Mode)

  Mode CTRL (3D8h): 0x6A
  ----- bits -----
  7 6 5 4  3 2 1 0   For CGA Mode.
  - x x -  x - x -
  | | | |  | | | |
  | | | |  | | | '-- 40*25 text.
  | | | |  | | '---- Graphics.
  | | | |  | '------ Color Mode.
  | | | |  '-------- Enable Video (toggled while redrawing the screen).
  | | | '----------- 320x200 Graphics.
  | | '------------- Enable Blink.
  | '--------------- Enable Change Mode.
  '----------------- (not for CGA)

  Color Sel (3D9h): bits 0-3 background color, bits 4-5 palette.

  Index register (3D4h): 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
  Data register (3D5h):  38 28 2D 0A 7F 06 64 70 02 01 06 07 10 00 00 00

  These are the standard CGA 320x200 (BIOS mode 4) parameters, apart from
  the start address (1000h, but MA12 isn't used to address the memory in
  graphics mode, so the picture starts at 0000h anyway):

  CRTC clock is 14.31818 MHz / 16 = 894.886 kHz (8 pixels per character).
  Horizontal: 57 characters total, 40 displayed -> 15.700 kHz.
  Vertical: 128 rows of 2 raster lines + 6 adjust = 262 lines total,
  100 rows (200 lines) displayed -> 59.923 Hz, non interlaced (R8 = 02h).

  Even raster lines are fetched from 0000h-1F3Fh, and odd ones from
  2000h-3F3Fh of the video memory.

  The sprite blitting routines wait for the vertical retrace polling
  bit 7 of the status register (documented only for MGA mode, active low).

**************************************************************************

  Custom IC's...

  8952 CM 32 pinouts and peripheral circuitry:

                               8952 CM 32
                            .------v------.
    74HC244 (IC9), PIN 08 --|01         40|-- 74HC244 (IC9), PIN 17
    74HC244 (IC9), PIN 06 --|02         39|-- 74HC244 (IC9), PIN 15
    74HC244 (IC9), PIN 04 --|03         38|-- 74HC244 (IC9), PIN 13
    74HC244 (IC9), PIN 02 --|04         37|-- 74HC244 (IC9), PIN 11
   74HC244 (IC10), PIN 08 --|05         36|-- 74HC244 (IC10), PIN 17
   74HC244 (IC10), PIN 06 --|06         35|-- 74HC244 (IC10), PIN 15
   74HC244 (IC10), PIN 04 --|07    8    34|-- 74HC244 (IC10), PIN 13
   74HC244 (IC10), PIN 02 --|08    9    33|-- 74HC244 (IC10), PIN 11
                            |      5      |
                         /--|09    2    32|--\
  To CN1 (through IC15) | --|10         31|-- | To CN1 (through IC15)
                        | --|11         30|-- |
                         \--|12    C    29|--/
                            |      M     |
                         /--|13         28|--\
  To CN2 (through IC14) | --|14    3    27|-- | To CN2 (through IC14)
                        | --|15    2    26|-- |
                         \--|16         25|--/
                            |             |
      To CN3 and ES2 9046 --|17         24|--\
      To CN3 and ES2 9046 --|18         23|-- > bridge to GND
                   To CN3 --|19         22|--/
                            |             |
                      GND --|20         21|-- VCC
                            '-------------'

                 74HC244 (IC9)                                 74HC244 (IC10)
                  .---v---.                                     .---v---.
   GAL (PIN 17) --|01   20|-- VCC                GAL (PIN 16) --|01   20|-- VCC
  8952 (PIN 04) --|02   19|-- GAL (PIN 17)      8952 (PIN 08) --|02   19|-- GAL (PIN 16)
  MAIN Z80 (D7) --|03   18|-- MAIN Z80 (D0)     MAIN Z80 (D7) --|03   18|-- MAIN Z80 (D0)
  8952 (PIN 03) --|04   17|-- 8952 (PIN 40)     8952 (PIN 07) --|04   17|-- 8952 (PIN 36)
  MAIN Z80 (D6) --|05   16|-- MAIN Z80 (D1)     MAIN Z80 (D6) --|05   16|-- MAIN Z80 (D1)
  8952 (PIN 02) --|06   15|-- 8952 (PIN 39)     8952 (PIN 06) --|06   15|-- 8952 (PIN 35)
  MAIN Z80 (D5) --|07   14|-- MAIN Z80 (D2)     MAIN Z80 (D5) --|07   14|-- MAIN Z80 (D2)
  8952 (PIN 01) --|08   13|-- 8952 (PIN 38)     8952 (PIN 05) --|08   13|-- 8952 (PIN 34)
  MAIN Z80 (D4) --|09   12|-- MAIN Z80 (D3)     MAIN Z80 (D4) --|09   12|-- MAIN Z80 (D3)
            GND --|10   11|-- 8952 (PIN 37)               GND --|10   11|-- 8952 (PIN 33)
                  '-------'                                     '-------'


  ES2 CM3080 pinouts and peripheral circuitry:

                    CM3080
                   .---v---.
             VCC --|01   18|-- VCC          .--------.
             N/C --|02   17|----------------+ 16 MHz |
             N/C --|03   16|----------------+  Xtal  |
             N/C --|04   15|-- CLK OUT --.  '--------'
             GND --|05   14|-- N/C       |
             GND --|06   13|-- N/C       '--+-- (8MHz) UM487F (PIN 01, CLK)
  MAIN Z80 (/M1) --|07   12|-- GND          +-- (8MHz) MAIN Z80 (PIN 06, CLK)
    GAL (PIN 11) --|08   11|-- VCC
             GND --|09   10|-- MAIN Z80 (/INT)
                   '-------'


  Notes:

  - Looks like the GAL is switching the different '8952 CM 32' outputs
     through the 74HC244 drivers to the Z80 data bus.
  - CN1, CN2 & CN3 are blind connectors.
  - 8952 pinouts to CN1 & CN2, are also passing through locations
     IC14 & IC15 (both are unpopulated from factory).
  - GAL is GAL16V8 at location IC4.
  - CM3080 pins 16 & 17 have a 1 Megohm resistor in parallel before connect the
     16 MHz. crystal.
  - The CPU roles come from the PCB layout: the Z84C00AB6 sits along with the
     video parts (IC6 EPROM, 6264, CM3080, GAL, the 74HC245 and the IC9/IC10
     link buffers), and the Z84C00HB6 along with the master ones (IC19 EPROM,
     9046, AY-3-8910 and the 8 MHz crystal). So the 4 MHz rated Z80A would be
     the one running at 8 MHz. Previous versions of this driver had them the
     other way around.
  - The 8952 CM 32 looks like a 32-bit serial input latched driver: it gets
     the master to video CPU link words from the 9046 (pins 17 & 18), and
     drives 16 bits to the video CPU data bus through IC9/IC10 (read at
     E000h-E001h) and the other 16 bits to CN1/CN2 through IC14/IC15 (DIP-18
     footprints, probably for ULN2803 style drivers). That's why the master
     sends 32-bit words: Cuatro en Linea and Dardos always send 0 in the
     CN1/CN2 half (Dardos has an unused routine that builds 9 lamp bits
     there).
  - BT1 (battery) is unpopulated, the settings are kept in the EEPROM.
  - The photographed PCB has some rework: wires around the 74LS04/74LS32 and
     the COMP pads next to CN10, a capacitor and a resistor on the solder
     side, and a wire from the 9046 to a hand-added transistor next to the
     AY-3-8910.

**************************************************************************

  Known games on this or similar hardware:

  - [DUMPED]  4 en Línea (Compumatic)
  - [DUMPED]  Dardos (Oper Coin)
  - [DUMPED]  Olympic Darts (K7 Kursaal. At least three different hardware revisions,
              the 1997 Kursaal schematics show a Z180 based board with a
              27C4001 EPROM, a DAC for sound and a monochrome monitor output)
  - [DUMPED]  Sport Darts TV (Compumatic / Desarrollos y Recambios S.L.)
  - [MISSING] Dart Queen (Compumatic / Daryde)

**************************************************************************

  Compumatic dual CPU boards (Cuatro en Linea, Dardos):

  The video CPU (8 MHz) only drives the UM487F, drawing the screens as
  commanded by the master CPU (4 MHz), which runs the game logic and the
  AY-3-8910 sound, and handles the inputs and the EEPROM through the ES2 9046
  I/O ports. The master sends the commands to the video CPU through a link,
  probably the 9046 shifting them to the 8952 CM 32, which drives the video
  CPU data bus through IC9/IC10 (74HC244).

  Cuatro en Linea has two joysticks with two buttons each, start buttons,
  five coin inputs, a service credit button and a setup key switch. Dardos
  reads the dart board matrix through the AY ports and the 9046, and has
  game selection buttons, a setup key switch, a reset key, and the same
  missed dart and player sensors as the K7 boards.

**************************************************************************

  K7 / Sport Darts single CPU boards:

  Z84C00BB6 @ 7.159 MHz, UM487F, WF19054 (AY-3-8910), X24C16P, 6264 + battery
  and a 64 KB EPROM. The upper half of the EPROM is banked in 4 pages of
  8 KB at C000h-DFFFh.

  The AY ports read the dart board matrix (4 columns of 16 targets, selected
  through port 1 bits 0-3) and the panel buttons (selected through port 1
  bit 4). The Sport Darts PCB uses active low select lines instead.

  The Olympic Darts v3.11 panel has Up, Down, NP (start / next player) and
  Player buttons, plus a setup key switch and a reset key. The v3.00 PCB has
  a button (and lamp) for each game and number of players instead.

  The game registers missed darts through an impact detector microphone
  (only while no sound is being played) and, at the end of a turn, waits for
  the player to cross an ultrasonic detector (40 kHz emitter and receiver on
  their own board) before continuing on its own.

  The EEPROM holds the settings, high scores and accounting. The games
  initialize it when it's blank.

  Sport Darts counts the IRQs from the vertical retrace to split the screen
  in two palettes, so the driver uses the UM487F horizontal sync as IRQ.

  Sport Darts T.V. schematics (Compumatic "YDESUS" CPU board, 1992):

  - The CM3080 gets the 14.31818 MHz clock of the 74LS04 oscillator (which
     also feeds the UM487F) on XTAL1, and outputs the 7.16 MHz CPU clock, an
     H/2 clock (divided by 2 by a 4020 to get the 1.79 MHz AY clock), the
     Z80 /INT (acknowledged with /M1 and /IORQ), and the /NMI and /RESET
     power supervisor signals. It has no horizontal sync input (pins 3 and 5
     go to an RC network), so its IRQ rate comes from an internal divider.
     The later K7 "YDESUS/PLUS" board replaces it with a 74HC74 dividing the
     clock and setting /INT on every UM487 horizontal retrace (HRET), and
     4011 gates for /NMI and /RESET.
  - Port 0 and port 1 outputs are 74LS273 latches, port 1 inputs a 74LS541
    (see the I/O handlers). The dart board columns and the buttons common are
    driven through MOSFETs, the coin counter and inhibit lines through BDX33
    transistors and the lamps through ULN2003 drivers.
  - The AY ports read the IX0-IX15 lines through diodes: the dart board rows
    and, while MPXIN is selected, the edge connector buttons (IX0 up, IX1
    down, IX2 O.K., IX3 player, IX4 key, IX5 reset).
  - A GAL drives the Z80 /WAIT from the UM487F IORDY on video memory
    accesses (not emulated).
  - Each UM487F color output goes to the monitor through a 100 ohm resistor,
    and a 2N2369 switched by IOUT loads it with another 100 ohm resistor, so
    the intensity scales the colors instead of adding a gray level (not
    emulated, the polarity of IOUT isn't clear from the datasheet and the
    later schematics, which label it -IOUT).
  - The AY outputs are mixed through resistors to a TDA2003 amplifier with a
    volume trimmer.

**************************************************************************

  TODO:

  - IRQ sources of the Compumatic and K7 boards (the frequencies are guessed
    from the software timings, the CM3080 internal divider is unknown).
  - Master to video CPU link (guessed from the code of both CPUs, the
    transfer time is unknown).
  - Master CPU wait states (one per memory access, guessed from the Dardos
    stuck sector detection, see machine_start).
  - Unknown inputs of the Compumatic boards (9046 port A and some port C/D
    lines).
  - Outputs of the Compumatic boards (9046 port A, CN1/CN2) and Olympic Darts
    v3.00 lamps.
  - UM487F IORDY wait states and the RGB output circuit of the K7 / Sport
    Darts boards.

*************************************************************************/

#include "emu.h"

#include "cpu/z80/z80.h"
#include "machine/i2cmem.h"
#include "machine/nvram.h"
#include "sound/ay8910.h"
#include "video/um487f.h"

#include "screen.h"
#include "speaker.h"


#define MAIN_CLOCK           XTAL(16'000'000)
#define SEC_CLOCK            XTAL(8'000'000)
#define HCGA_CLOCK           XTAL(14'318'181)

#define PRG_CPU_CLOCK        MAIN_CLOCK /2      // 8 MHz. (measured)
#define SND_CPU_CLOCK        SEC_CLOCK /2       // 4 MHz. (measured)
#define SND_AY_CLOCK         SEC_CLOCK /4       // 2 MHz. (measured)


namespace {

class sysi_state : public driver_device
{
protected:
	sysi_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_video(*this, "um487f")
		, m_ay(*this, "aysnd")
		, m_eeprom(*this, "eeprom")
	{ }

	// A15 = 1 selects the CGA video memory window of the UM487F
	uint8_t vram_r(offs_t offset) { return m_video->mem_r(0x8000 | offset); }
	void vram_w(offs_t offset, uint8_t data) { m_video->mem_w(0x8000 | offset, data); }

	void hcga_config(machine_config &config) ATTR_COLD;

	required_device<cpu_device> m_maincpu;
	required_device<um487f_device> m_video;
	required_device<ay8910_device> m_ay;
	required_device<i2cmem_device> m_eeprom;
};


class _4enlinea_state : public sysi_state
{
public:
	_4enlinea_state(const machine_config &mconfig, device_type type, const char *tag)
		: sysi_state(mconfig, type, tag)
		, m_audiocpu(*this, "audiocpu")
		, m_matrix(*this, "MATRIX%u", 0U)
		, m_buttons(*this, "BUTTONS%u", 0U)
		, m_in_pc(*this, "IN_PC")
		, m_in_pd(*this, "IN_PD")
	{ }

	void _4enlinea(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

private:
	// master to video CPU link
	uint8_t link_r(offs_t offset);
	uint8_t handshake_status_r();
	void handshake_w(offs_t offset, uint8_t data);
	void handshake_status_w(uint8_t data);
	uint8_t link_status_r();
	void link_control_w(uint8_t data);
	void link_data_w(offs_t offset, uint8_t data);
	void send_to_video(uint8_t data0, uint8_t data1);
	TIMER_CALLBACK_MEMBER(link_ready);

	// ES2 9046 I/O ports
	uint8_t port_r(offs_t offset);
	void port_w(offs_t offset, uint8_t data);
	uint8_t port_out(int port) const { return m_port_latch[port] | m_port_dir[port]; }
	void update_outputs();
	uint8_t ay_porta_r();
	uint8_t ay_portb_r();

	void audio_map(address_map &map) ATTR_COLD;
	void main_map(address_map &map) ATTR_COLD;
	void main_portmap(address_map &map) ATTR_COLD;

	required_device<cpu_device> m_audiocpu;
	optional_ioport_array<4> m_matrix;
	required_ioport_array<2> m_buttons;
	required_ioport m_in_pc;
	required_ioport m_in_pd;

	emu_timer *m_link_timer = nullptr;

	uint8_t m_handshake_status = 0;
	uint8_t m_link_latch[2]{};
	uint8_t m_link_data[4]{};
	bool m_link_ready = true;
	uint8_t m_port_latch[4]{};
	uint8_t m_port_dir[4]{};
};


class k7_state : public sysi_state
{
public:
	k7_state(const machine_config &mconfig, device_type type, const char *tag)
		: sysi_state(mconfig, type, tag)
		, m_rombank(*this, "rombank")
		, m_matrix(*this, "MATRIX%u", 0U)
		, m_buttons(*this, "BUTTONS%u", 0U)
		, m_in1(*this, "IN1")
		, m_in1_col(*this, "IN1_COL")
		, m_in1_alt(*this, "IN1_ALT")
		, m_lamp(*this, "lamp0")
		, m_lamps(*this, "lamp%u", 1U)
	{ }

	void k7_olym(machine_config &config) ATTR_COLD;
	void sprtdart(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;

private:
	uint8_t in1_r();
	void out0_w(uint8_t data);
	void out1_w(uint8_t data);
	uint8_t selected_lines() const { return m_sport_darts ? ~m_out1 : m_out1; }
	uint8_t ay_porta_r();
	uint8_t ay_portb_r();
	void hsync_w(int state);

	void mem_map(address_map &map) ATTR_COLD;
	void io_map(address_map &map) ATTR_COLD;

	required_memory_bank m_rombank;
	required_ioport_array<4> m_matrix;
	optional_ioport_array<2> m_buttons;
	required_ioport m_in1;
	optional_ioport m_in1_col;
	optional_ioport m_in1_alt;
	output_finder<> m_lamp;
	output_finder<4> m_lamps;

	bool m_sport_darts = false; // Compumatic Sport Darts T.V. PCB
	uint8_t m_out0 = 0;
	uint8_t m_out1 = 0;
	uint8_t m_hsync_count = 0;
};


/***********************************
*      Memory Map Information      *
***********************************/

/*
  Master to video CPU link (ES2 9046 and 8952 CM 32?)

  The video CPU gets a NMI and reads a word (command and parameter) at
  E000h-E001h. The master sends the words through FC29h-FC2Ch (it writes
  00h, 00h, parameter, command, twice) and then strobes FC28h, waiting
  for FC28h bit 3 before sending the next one. At boot time, it also checks
  the link writing FC30h/FC31h and waiting for FC32h bit 5, which gets set
  when the video CPU reads the word.

  The 9046 probably shifts the 32 bits to the 8952 CM 32, whose outputs are
  read by the video CPU (FC2Bh-FC2Ch) or go to CN1/CN2 (FC29h-FC2Ah, unused).

  FC28h bit 3 can't depend on the video CPU reading the word: Dardos keeps
  sending null words while idle, and its video CPU stops reading them while
  its command queue is full (during the boot delay).

  TODO: guessed from the code of both CPUs, transfer time unknown.
*/
void _4enlinea_state::send_to_video(uint8_t data0, uint8_t data1)
{
	m_link_latch[0] = data0;
	m_link_latch[1] = data1;
	m_maincpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);
}

TIMER_CALLBACK_MEMBER(_4enlinea_state::link_ready)
{
	m_link_ready = true;
}

uint8_t _4enlinea_state::link_r(offs_t offset)
{
	if (offset == 0 && !machine().side_effects_disabled())
		m_handshake_status |= 0x20;

	return m_link_latch[offset];
}

uint8_t _4enlinea_state::handshake_status_r()
{
	return m_handshake_status;
}

void _4enlinea_state::handshake_status_w(uint8_t data)
{
	m_handshake_status = data; // probably just clears
}

void _4enlinea_state::handshake_w(offs_t offset, uint8_t data)
{
	if (offset == 0)
		send_to_video(data, 0);
}

uint8_t _4enlinea_state::link_status_r()
{
	// bit 3: ready to send the next word
	return m_link_ready ? 0x08 : 0x00;
}

void _4enlinea_state::link_control_w(uint8_t data)
{
	// the master writes 0 and then 7 after loading a word
	if (!BIT(data, 0))
	{
		// the video CPU expects the command (sent in the low byte) at E001h
		m_link_ready = false;
		m_link_timer->adjust(attotime::from_usec(100));
		send_to_video(m_link_data[3], m_link_data[2]);
	}
}

void _4enlinea_state::link_data_w(offs_t offset, uint8_t data)
{
	m_link_data[offset] = data;
}


/*
  ES2 9046 I/O ports, with data (even) and direction (odd, 1 = input) registers:

  FC20h-FC21h: port A, bits 1 and 3 are inputs (unknown)
  FC22h-FC23h: port B, unused
  FC24h-FC25h: port C
               bit 0:    EEPROM SDA
               bits 1-5: coins
               bit 6:    sensor (Dardos)
               bit 7:    dart board matrix column 0 (Dardos)
  FC26h-FC27h: port D
               bits 0-2: dart board matrix columns 1-3 (Dardos)
               bit 4:    sensor (Dardos)
               bit 6:    EEPROM SCL
               bit 7:    buttons (read through the AY ports)
*/
uint8_t _4enlinea_state::port_r(offs_t offset)
{
	int const port = offset >> 1;

	if (BIT(offset, 0))
		return m_port_dir[port];

	uint8_t in = 0xff;
	switch (port)
	{
	case 2:
		in = (m_in_pc->read() & 0xfe) | (m_eeprom->read_sda() ? 0x01 : 0x00);
		break;
	case 3:
		in = m_in_pd->read();
		break;
	}

	return (in & m_port_dir[port]) | (m_port_latch[port] & ~m_port_dir[port]);
}

void _4enlinea_state::port_w(offs_t offset, uint8_t data)
{
	int const port = offset >> 1;

	if (BIT(offset, 0))
		m_port_dir[port] = data;
	else
		m_port_latch[port] = data;

	update_outputs();
}

void _4enlinea_state::update_outputs()
{
	// outputs are driven low through open collector/drain lines, released when set as inputs
	m_eeprom->write_scl(BIT(port_out(3), 6));
	m_eeprom->write_sda(BIT(port_out(2), 0));
}

uint8_t _4enlinea_state::ay_porta_r()
{
	uint8_t const columns = (BIT(port_out(2), 7) << 0) | ((port_out(3) & 0x07) << 1);
	uint8_t data = 0xff;

	for (int i = 0; i < 4; i++)
		if (BIT(columns, i))
			data &= m_matrix[i].read_safe(0xffff);

	if (BIT(port_out(3), 7))
		data &= m_buttons[0]->read();

	return data;
}

uint8_t _4enlinea_state::ay_portb_r()
{
	uint8_t const columns = (BIT(port_out(2), 7) << 0) | ((port_out(3) & 0x07) << 1);
	uint8_t data = 0xff;

	for (int i = 0; i < 4; i++)
		if (BIT(columns, i))
			data &= m_matrix[i].read_safe(0xffff) >> 8;

	if (BIT(port_out(3), 7))
		data &= m_buttons[1]->read();

	return data;
}

void _4enlinea_state::main_map(address_map &map)
{
	map(0x0000, 0x7fff).rom();
	map(0x8000, 0xbfff).rw(FUNC(_4enlinea_state::vram_r), FUNC(_4enlinea_state::vram_w));
	map(0xc000, 0xdfff).ram();

	map(0xe000, 0xe001).r(FUNC(_4enlinea_state::link_r));
}

void _4enlinea_state::main_portmap(address_map &map)
{
	map.global_mask(0x3ff);

	map(0x3b0, 0x3df).rw(m_video, FUNC(um487f_device::io_r), FUNC(um487f_device::io_w));
}

void _4enlinea_state::audio_map(address_map &map)
{
	map(0x0000, 0x7fff).rom();
	map(0xf800, 0xfbff).ram();
	map(0xfc20, 0xfc27).rw(FUNC(_4enlinea_state::port_r), FUNC(_4enlinea_state::port_w));
	map(0xfc28, 0xfc28).rw(FUNC(_4enlinea_state::link_status_r), FUNC(_4enlinea_state::link_control_w));
	map(0xfc29, 0xfc2c).w(FUNC(_4enlinea_state::link_data_w));
	map(0xfc30, 0xfc31).w(FUNC(_4enlinea_state::handshake_w));
	map(0xfc32, 0xfc32).rw(FUNC(_4enlinea_state::handshake_status_r), FUNC(_4enlinea_state::handshake_status_w));
	map(0xfc48, 0xfc48).w(m_ay, FUNC(ay8910_device::address_w));
	map(0xfc49, 0xfc49).r(m_ay, FUNC(ay8910_device::data_r));
	map(0xfc4a, 0xfc4a).w(m_ay, FUNC(ay8910_device::data_w));
}


/*
  K7 port 1 (read, 74LS541 IC7 on the Sport Darts schematics):
  bits 0-3: coin selector lines SELECTOR0-3 (coin 1 and 2 on the edge
            connector, the electronic selector can drive all of them; the
            games give 500 and 100 Pts for lines 0 and 1, nothing for 2 and 3)
  bit 4:    EEPROM SDA
  bit 5:    unknown (labeled ROUT?), the games wait for it to be low at boot
  bit 6:    MICINT, impact detector microphone (missed darts)
  bit 7:    ULTRAS, ultrasonic player detector

  On the Olympic Darts v3.00 PCB, some lines carry other inputs while the
  buttons are selected (port 1 bit 4) or while port 0 bit 7 is set.
*/
uint8_t k7_state::in1_r()
{
	if (BIT(m_out0, 7) && m_in1_alt)
		return m_in1_alt->read();

	uint8_t data = m_in1->read();
	if (BIT(selected_lines(), 4) && m_in1_col)
		data = (data & ~0x0c) | (m_in1_col->read() & 0x0c);

	return data;
}

/*
  K7 port 0 (write, 74LS273 IC1 on the Sport Darts schematics):
  bits 0-1: ROM bank at C000h-DFFFh (EP0/EP1, decoded by a GAL)
  bit 2:    EEPROM SCL
  bit 3:    EEPROM SDA (through an open collector transistor, so inverted)
  bits 4-7: lamps L1-L4 through ULN2003 drivers (Sport Darts)
  bit 7:    input bank select (Olympic Darts v3.00 PCB)
*/
void k7_state::out0_w(uint8_t data)
{
	m_out0 = data;
	m_rombank->set_entry(data & 0x03);
	m_eeprom->write_scl(BIT(data, 2));
	m_eeprom->write_sda(!BIT(data, 3));

	if (m_sport_darts)
		for (int i = 0; i < 4; i++)
			m_lamps[i] = BIT(data, 4 + i); // L1 blinks while waiting for OK
}

/*
  K7 port 1 (write, 74LS273 IC8 on the Sport Darts schematics):
  bits 0-3: dart board matrix columns DIANA1-4 (read through AY ports A and B)
  bit 4:    buttons common MPXIN (read through AY port A)
  (the Sport Darts PCB selects them with active low lines instead)
  bit 5:    K7: lamp (blinks while waiting for a player to start)
            Sport Darts: coin selector inhibit line (BDX33 driver), set
            while coins are accepted
  bit 6:    coin counter (BDX33 driver)
  bit 7:    pulsed low at boot
*/
void k7_state::out1_w(uint8_t data)
{
	m_out1 = data;

	if (m_sport_darts)
		machine().bookkeeping().coin_lockout_global_w(!BIT(data, 5));
	else
		m_lamp = BIT(data, 5);
	machine().bookkeeping().coin_counter_w(0, BIT(data, 6));
}

void k7_state::hsync_w(int state)
{
	if (state && !(++m_hsync_count & 0x07))
		m_maincpu->set_input_line(0, HOLD_LINE);
}

uint8_t k7_state::ay_porta_r()
{
	uint8_t const sel = selected_lines();
	uint8_t data = 0xff;

	for (int i = 0; i < 4; i++)
		if (BIT(sel, i))
			data &= m_matrix[i]->read();

	if (BIT(sel, 4))
		data &= m_buttons[0].read_safe(0xff);

	return data;
}

uint8_t k7_state::ay_portb_r()
{
	uint8_t const sel = selected_lines();
	uint8_t data = 0xff;

	for (int i = 0; i < 4; i++)
		if (BIT(sel, i))
			data &= m_matrix[i]->read() >> 8;

	if (BIT(sel, 4))
		data &= m_buttons[1].read_safe(0xff);

	return data;
}

void k7_state::mem_map(address_map &map)
{
	map(0x0000, 0x7fff).rom().region("maincpu", 0);
	map(0x8000, 0xbfff).rw(FUNC(k7_state::vram_r), FUNC(k7_state::vram_w));
	map(0xc000, 0xdfff).bankr(m_rombank);
	map(0xe000, 0xffff).ram().share("nvram");
}

void k7_state::io_map(address_map &map)
{
	map(0x0000, 0x0000).mirror(0xfc00).w(FUNC(k7_state::out0_w));
	map(0x0001, 0x0001).mirror(0xfc00).rw(FUNC(k7_state::in1_r), FUNC(k7_state::out1_w));
	map(0x0100, 0x0100).w(m_ay, FUNC(ay8910_device::address_w));
	map(0x0101, 0x0101).r(m_ay, FUNC(ay8910_device::data_r));
	map(0x0102, 0x0102).w(m_ay, FUNC(ay8910_device::data_w));
	map(0x03b0, 0x03df).mirror(0xfc00).rw(m_video, FUNC(um487f_device::io_r), FUNC(um487f_device::io_w));
}


/***********************************
*           Input Ports            *
***********************************/

static INPUT_PORTS_START( 4enlinea )
	// read through the AY ports while port D bit 7 is set
	PORT_START("BUTTONS0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_8WAY  PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_8WAY  PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_8WAY  PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY  PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )                   PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )                   PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Service Credit")

	PORT_START("BUTTONS1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_8WAY  PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_8WAY  PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_8WAY  PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY  PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )                   PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )                   PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Setup") PORT_TOGGLE PORT_CODE(KEYCODE_F2)

	// 9046 port C, each coin line adds a credit (with its own counter)
	PORT_START("IN_PC")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) // EEPROM SDA
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN5 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	// 9046 port D
	PORT_START("IN_PD")
	PORT_BIT( 0x18, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xe7, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/*  K7 dart board matrix, 4 columns selected through port 1 bits 0-3,
    rows read through AY port A (bits 0-7) and port B (bits 8-15).
*/
#define K7_DART(mask, name) \
	PORT_BIT( mask, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME(name)

static INPUT_PORTS_START( k7_matrix )
	PORT_START("MATRIX0")
	K7_DART( 0x0001, "Single 3" )
	K7_DART( 0x0002, "Double 3" )
	K7_DART( 0x0004, "Single 19" )
	K7_DART( 0x0008, "Double 19" )
	K7_DART( 0x0010, "Single 7" )
	K7_DART( 0x0020, "Double 7" )
	K7_DART( 0x0040, "Single 16" )
	K7_DART( 0x0080, "Double 16" )
	K7_DART( 0x0100, "Double 17" )
	K7_DART( 0x0200, "Single 17" )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	K7_DART( 0x0800, "Triple 16" )
	K7_DART( 0x1000, "Triple 7" )
	K7_DART( 0x2000, "Triple 19" )
	K7_DART( 0x4000, "Triple 3" )
	K7_DART( 0x8000, "Triple 17" )

	PORT_START("MATRIX1")
	K7_DART( 0x0001, "Single 6" )
	K7_DART( 0x0002, "Double 6" )
	K7_DART( 0x0004, "Single 10" )
	K7_DART( 0x0008, "Double 10" )
	K7_DART( 0x0010, "Single 15" )
	K7_DART( 0x0020, "Double 15" )
	K7_DART( 0x0040, "Single 2" )
	K7_DART( 0x0080, "Double 2" )
	K7_DART( 0x0100, "Double 13" )
	K7_DART( 0x0200, "Single 13" )
	K7_DART( 0x0400, "Bull's Eye (50)" )
	K7_DART( 0x0800, "Triple 2" )
	K7_DART( 0x1000, "Triple 15" )
	K7_DART( 0x2000, "Triple 10" )
	K7_DART( 0x4000, "Triple 6" )
	K7_DART( 0x8000, "Triple 13" )

	PORT_START("MATRIX2")
	K7_DART( 0x0001, "Single 20" )
	K7_DART( 0x0002, "Double 20" )
	K7_DART( 0x0004, "Single 1" )
	K7_DART( 0x0008, "Double 1" )
	K7_DART( 0x0010, "Single 18" )
	K7_DART( 0x0020, "Double 18" )
	K7_DART( 0x0040, "Single 4" )
	K7_DART( 0x0080, "Double 4" )
	K7_DART( 0x0100, "Double 5" )
	K7_DART( 0x0200, "Single 5" )
	K7_DART( 0x0400, "Bull (25)" )
	K7_DART( 0x0800, "Triple 4" )
	K7_DART( 0x1000, "Triple 18" )
	K7_DART( 0x2000, "Triple 1" )
	K7_DART( 0x4000, "Triple 20" )
	K7_DART( 0x8000, "Triple 5" )

	PORT_START("MATRIX3")
	K7_DART( 0x0001, "Single 11" )
	K7_DART( 0x0002, "Double 11" )
	K7_DART( 0x0004, "Single 14" )
	K7_DART( 0x0008, "Double 14" )
	K7_DART( 0x0010, "Single 9" )
	K7_DART( 0x0020, "Double 9" )
	K7_DART( 0x0040, "Single 12" )
	K7_DART( 0x0080, "Double 12" )
	K7_DART( 0x0100, "Double 8" )
	K7_DART( 0x0200, "Single 8" )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	K7_DART( 0x0800, "Triple 12" )
	K7_DART( 0x1000, "Triple 9" )
	K7_DART( 0x2000, "Triple 14" )
	K7_DART( 0x4000, "Triple 11" )
	K7_DART( 0x8000, "Triple 8" )
INPUT_PORTS_END

static INPUT_PORTS_START( k7_olym )
	PORT_INCLUDE( k7_matrix )

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_NAME("Coin 1 (500 Pts)")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_NAME("Coin 2 (100 Pts)")
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("eeprom", FUNC(i2cmem_device::read_sda))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // must be low at boot
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Missed Dart Sensor")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Player Sensor")

	PORT_START("BUTTONS0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Up")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Down")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("NP (Start / Next Player)")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Player")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Setup") PORT_TOGGLE PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Reset (Clear Credits)")
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( dardos )
	PORT_INCLUDE( k7_matrix )

	// read through AY port A while port D bit 7 is set, like on the K7 PCB
	// the game calls the buttons FLECHA ARRIBA, FLECHA ABAJO, OK and PLAY
	PORT_START("BUTTONS0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Up")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Down")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("OK")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Play")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Setup") PORT_TOGGLE PORT_CODE(KEYCODE_F2) // only read at power on
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Reset (Clear Credits)")
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("BUTTONS1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	// 9046 port C, coin values in 25 Pts units
	PORT_START("IN_PC")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) // EEPROM SDA
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_NAME("Coin 1 (500 Pts)")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_NAME("Coin 2 (100 Pts)")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_NAME("Coin 3 (50 Pts)")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN4 ) PORT_NAME("Coin 4 (200 Pts)")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN5 ) PORT_NAME("Coin 5 (32 units)")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Player Sensor")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	// 9046 port D
	PORT_START("IN_PD")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Missed Dart Sensor")
	PORT_BIT( 0xe7, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

// Sport Darts T.V. edge connector buttons (IX0-IX5): arriba, abajo, O.K., player, key and reset
static INPUT_PORTS_START( sprtdart )
	PORT_INCLUDE( k7_olym )

	PORT_MODIFY("BUTTONS0")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("OK")
INPUT_PORTS_END

// Olympic Darts v3.00 PCB, with a button (and lamp) for each game and number of players
static INPUT_PORTS_START( k7_olym30 )
	PORT_INCLUDE( k7_olym )

	PORT_MODIFY("BUTTONS0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("High Score / Down")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Shanghai / Up")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Scram / Player")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Roulette")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("NP (Start / Next Player)")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Tres en Raya")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Cricket")

	PORT_START("BUTTONS1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("301")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("501")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("Double In / Cut Throat")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Double Out / Team")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1 Player") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2 Players") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3 Players") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4 Players") PORT_CODE(KEYCODE_4_PAD)

	PORT_START("IN1_COL")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("5 Players") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("6 Players") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0xf3, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("IN1_ALT")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON11 ) PORT_NAME("Ahorcado")
	PORT_BIT( 0xfb, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/****************************************
*          Machine Start/Reset          *
****************************************/

void _4enlinea_state::machine_start()
{
	m_link_timer = timer_alloc(FUNC(_4enlinea_state::link_ready), this);

	/* TODO: guessed one wait state for every memory access of the master
	   CPU (opcode fetches included, there's no separate opcodes space).
	   Without it, Dardos registers a dart every few milliseconds while a
	   dart board sector is held down, instead of showing "SECTOR PISADO"
	   (stuck sector) after half a second as the real board does: the main
	   loop discards the sector when it doesn't get a new matrix scan (done
	   by the IRQ handler) since the previous pass, so it must be slower
	   than the IRQ period. */
	address_space &space = m_audiocpu->space(AS_PROGRAM);
	space.install_read_tap(0x0000, 0xffff, "master_wait_r",
			[this] (offs_t offset, u8 &data, u8 mem_mask)
			{
				if (!machine().side_effects_disabled())
					m_audiocpu->adjust_icount(-1);
			});
	space.install_write_tap(0x0000, 0xffff, "master_wait_w",
			[this] (offs_t offset, u8 &data, u8 mem_mask)
			{
				if (!machine().side_effects_disabled())
					m_audiocpu->adjust_icount(-1);
			});

	save_item(NAME(m_handshake_status));
	save_item(NAME(m_link_latch));
	save_item(NAME(m_link_data));
	save_item(NAME(m_link_ready));
	save_item(NAME(m_port_latch));
	save_item(NAME(m_port_dir));
}

void _4enlinea_state::machine_reset()
{
	// all the ports are inputs after reset
	std::fill(std::begin(m_port_dir), std::end(m_port_dir), 0xff);
	update_outputs();

	m_link_ready = true;
	m_link_timer->adjust(attotime::never);
}

void k7_state::machine_start()
{
	m_rombank->configure_entries(0, 4, memregion("maincpu")->base() + 0x8000, 0x2000);
	m_rombank->set_entry(0);

	save_item(NAME(m_out0));
	save_item(NAME(m_out1));
	save_item(NAME(m_hsync_count));
}


/***********************************
*         Machine Drivers          *
***********************************/

void sysi_state::hcga_config(machine_config &config)
{
	// 320x200 CGA timings as programmed by the games, the UM487F reconfigures the screen from its CRTC registers
	screen_device &screen(SCREEN(config, "screen"));
	screen.set_raw(HCGA_CLOCK / 2, 456, 0, 320, 262, 0, 200);
	screen.set_screen_update(m_video, FUNC(um487f_device::screen_update));

	UM487F(config, m_video, HCGA_CLOCK); // MOSC (MGA clock) tied to GND
	m_video->set_screen("screen");
}

void _4enlinea_state::_4enlinea(machine_config &config)
{
	// basic machine hardware
	Z80(config, m_maincpu, PRG_CPU_CLOCK);
	m_maincpu->set_addrmap(AS_PROGRAM, &_4enlinea_state::main_map);
	m_maincpu->set_addrmap(AS_IO, &_4enlinea_state::main_portmap);
	/* TODO: IRQ sources are unknown.
	   All these boards run their main IRQ routine around 1 kHz:
	   - The video CPU runs its software tick every 20 IRQs, and expects it
	     around 50 Hz: it shows the boot logo for 300 ticks, reprograms the
	     UM487F every 50 ticks...
	   - The master CPU runs its tick every 10 IRQs, and counts the game time
	     seconds every 100 ticks.
	   - Sport Darts runs its IRQ routine every 16 horizontal syncs.
	   So assume the 16 MHz clock divided by 16384 for the video CPU (the
	   CM3080 gets the 16 MHz crystal and drives its /INT), and the same rate
	   from the 8 MHz crystal for the master CPU (probably generated by the
	   9046, which sits next to it). */
	m_maincpu->set_periodic_int(FUNC(_4enlinea_state::irq0_line_hold), attotime::from_hz(MAIN_CLOCK / 16384));

	Z80(config, m_audiocpu, SND_CPU_CLOCK); // wait states added in machine_start()
	m_audiocpu->set_addrmap(AS_PROGRAM, &_4enlinea_state::audio_map);
	m_audiocpu->set_periodic_int(FUNC(_4enlinea_state::irq0_line_hold), attotime::from_hz(SEC_CLOCK / 8192));

	I2C_24C16(config, m_eeprom); // X24C16P

	// video hardware
	hcga_config(config);

	// sound hardware
	SPEAKER(config, "mono").front_center();
	AY8910(config, m_ay, SND_AY_CLOCK);
	m_ay->port_a_read_callback().set(FUNC(_4enlinea_state::ay_porta_r));
	m_ay->port_b_read_callback().set(FUNC(_4enlinea_state::ay_portb_r));
	m_ay->add_route(ALL_OUTPUTS, "mono", 0.50);
}


void k7_state::k7_olym(machine_config &config)
{
	Z80(config, m_maincpu, HCGA_CLOCK / 2); // Z84C00BB6
	m_maincpu->set_addrmap(AS_PROGRAM, &k7_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &k7_state::io_map);

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0); // D4464C-15L (6264) + battery

	I2C_24C16(config, m_eeprom); // X24C16P

	hcga_config(config); // UM487F

	/* TODO: IRQ source is unknown.
	   The game runs its IRQ routine every 2 IRQs and its tick every 10 of
	   them, with the same timings as Sport Darts (IRQ routine every 16
	   horizontal syncs) and the Compumatic master CPU (100 ticks per second),
	   so assume the horizontal sync divided by 8 (the CM3080 subboard has two
	   HEF4020 ripple counters). */
	m_video->hsync_callback().set(FUNC(k7_state::hsync_w));

	SPEAKER(config, "mono").front_center();
	AY8910(config, m_ay, HCGA_CLOCK / 8); // Winbond WF19054
	m_ay->port_a_read_callback().set(FUNC(k7_state::ay_porta_r));
	m_ay->port_b_read_callback().set(FUNC(k7_state::ay_portb_r));
	m_ay->add_route(ALL_OUTPUTS, "mono", 0.50);
}

void k7_state::sprtdart(machine_config &config)
{
	k7_olym(config);

	/* The IRQ handler counts the IRQs and resyncs with the vertical retrace to
	   change the background color at a given raster line, so the IRQ comes from
	   the UM487F horizontal sync (the main IRQ routine runs every 16 IRQs). */
	m_video->hsync_callback().set_inputline(m_maincpu, 0, HOLD_LINE); // TODO: polarity

	m_sport_darts = true;
}


/***********************************
*             Rom Load             *
***********************************/

ROM_START( 4enlinea )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cuatro_en_linea_27c256__cicplay-2.ic6",  0x0000, 0x8000, CRC(f8f14bf8) SHA1(e48fbedbd1b9be6fb56a0f65db80eddbedb487c7) )

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "cuatro_en_linea_27c256__cicplay-1.ic19", 0x0000, 0x8000, CRC(307a57a3) SHA1(241329d919ec43d0eeb1dad0a4db6cf6de06e7e1) )

	ROM_REGION( 0x0800, "eeprom", 0 ) // default serial EEPROM
	ROM_LOAD( "cuatro_en_linea_x24c16p__nosticker.ic17", 0x0000, 0x0800, CRC(21f81f5a) SHA1(00b10eee5af1ca79ced2878f4be4cac2bb8d26a0) )

	ROM_REGION( 0x0200, "plds", 0 )
	ROM_LOAD( "cuatro_en_linea_gal16v8as__nosticker.ic04", 0x0000, 0x0117, CRC(094edf29) SHA1(428a2f6568ac1032833ee0c65fa8304967a58607) )
ROM_END

ROM_START( 4enlineb )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cuatro_en_linea_2_a06.ic6",  0x0000, 0x8000, CRC(f8f14bf8) SHA1(e48fbedbd1b9be6fb56a0f65db80eddbedb487c7) )

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "cuatro_en_linea_1_a06.ic19", 0x0000, 0x8000, CRC(993d0581) SHA1(d6e366dd827543508037d2071c4b6e638c2cf87b) )

	ROM_REGION( 0x0800, "eeprom", 0 ) // From an operated PCB, a clean one for default need to be created...
	ROM_LOAD( "cuatro_en_linea_24c16.ic17", 0x0000, 0x0800, CRC(56722dd4) SHA1(f818d882b3070f9b1fac486987a044ab1d418985) )

	ROM_REGION( 0x0200, "plds", 0 )
	ROM_LOAD( "cuatro_en_linea_gal16v8a.ic04", 0x0000, 0x0117, CRC(1edaf06c) SHA1(51e44c2e6b54991330d6ef945e98fa2c8a49408d) )
ROM_END

/*
  Dardos
  Oper Coin. 1991.
  Running in 487 System I.
*/
ROM_START( dardos )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "diana_iv_video_27-1-92.bin",  0x0000, 0x8000, CRC(f23b5313) SHA1(488cf9bedce7b0c7b474bd93da70181c81fa300b) )

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "diana_iv_master_27-1-92.bin", 0x0000, 0x8000, CRC(4b2c868a) SHA1(91120a32fac9c5a6e7746d2e2587921f7d42eaa3) )
ROM_END


/* Kursaal K7 Olympic Darts PCB
    __________________________________________________      SUBBOARD CM3080
    |           ________  __   ______  ______________ |     ________________
    |  _______  | DB9   | |_| |_CN8__| |____CN7______||     |___ __________ |
    |  |______| |_______| CN9                         |__   ||  ||HEF4020BP||
    | ________                                         __|  ||A | _________ |
    | |D41464C|                                        __|  ||  | |________||
    | ________                           _____         __|  ||__| _________ |
    | |D41464C|                         DA741CN        __|  |     |TC4011BP||
    | ________                         _______    ___  __|  |    __________ |
 IC4->|GAL16V8|  _______              HCF4069UBE  XT5  __|  |    |__EMPTY__||
    | ________   |UMC   |  ________   _______________  __|  |    __________ |
IC11->|GAL16V8|  |UM487F|  74HC273AP  |WF19054       | __|  |    |HEF4020BP||
    |            |______|  ________   |______________| __|  |_______________|
    | ________             74HC273AP  _______________  __|    A=74LS368ANA
    | |74LS04N|           ___________ |Z84C00BB6     | __|
    |  _____              | SUBBOARD ||______________| __|
    |  |XT2_|<-14.31818MHz| CM3080   |____________     __|
    |           ________  |          ||M27C512 ROM|    __|
    |          HCF4069UBE |          ||___________|    __|
    |                     |          |____________     __|
    |                     |          ||D4464C-15L |    __|
    |                     |__________||___________|    __|
    | 7808CT    ________    ________  _____   _____   |
    |           |_______|  74LS541B1 X24C16P  |BATT|  |
    | _____  ___________  _____________       |____|  |
    | |CN1_| |__CN5_____| |__CN4_______|              |
    |_________________________________________________|
*/
ROM_START( k7_olym )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "odk7_v3.11_27c512.ic18", 0x00000, 0x10000, CRC(063d24fe) SHA1(ad4509438d2028ede779f5aa9a918d1020c1db41) )

	// The EEPROM contains a custom message (operators can set on-screen messages).
	// A clean one for default need to be created...
	ROM_REGION( 0x0800, "eeprom", 0 )
	ROM_LOAD( "x24c16p.bin", 0x0000, 0x0800, CRC(4c6685b2) SHA1(38c4f64f038d7ce185d6fd0b6eec4c9818f64e8e) )

	ROM_REGION( 0x0300, "plds", 0 )
	ROM_LOAD( "a1_gal16v8a.ic11", 0x0000, 0x0117, NO_DUMP ) // protected
	ROM_LOAD( "b1_gal16v8a.ic4",  0x0117, 0x0117, NO_DUMP ) // protected
ROM_END

ROM_START( k7_olym30 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "dardos_k7_3.0_21-11-94_27c512.ic19", 0x00000, 0x10000,  CRC(87af55a6) SHA1(7d12ce7afe8a50ba895f05029c1bd05a3641f7fd) )

	ROM_REGION( 0x0800, "eeprom", 0 )
	ROM_LOAD( "x24c16p.bin", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x0300, "plds", 0 )
	ROM_LOAD( "a1_gal16v8a.ic11", 0x0000, 0x0117, NO_DUMP ) // protected
	ROM_LOAD( "b1_gal16v8a.ic4",  0x0117, 0x0117, NO_DUMP ) // protected
ROM_END


ROM_START( sprtdart )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sport_dart_27c512.ic19", 0x00000, 0x10000, CRC(6c9ae27f) SHA1(92fbdef7747a9096daf4714f45b119ad8f3a1436) )

	ROM_REGION( 0x0800, "eeprom", 0 )
	ROM_LOAD( "24c16.ic17", 0x0000, 0x0800, NO_DUMP ) // Undumped

	ROM_REGION( 0x0300, "plds", 0 )
	ROM_LOAD( "gal16v8a.ic11", 0x0000, 0x0117, NO_DUMP ) // protected
	ROM_LOAD( "gal16v8a.ic4",  0x0117, 0x0117, NO_DUMP ) // protected
ROM_END


} // anonymous namespace


/***********************************
*           Game Drivers           *
***********************************/

//    YEAR  NAME       PARENT    MACHINE    INPUT      CLASS            INIT        ROT    COMPANY                                      FULLNAME                            FLAGS
GAME( 1991, 4enlinea,  0,        _4enlinea, 4enlinea,  _4enlinea_state, empty_init, ROT0, "Compumatic / CIC Play",                     "Cuatro en Linea (rev. A-07)", MACHINE_NOT_WORKING )
GAME( 1991, 4enlineb,  4enlinea, _4enlinea, 4enlinea,  _4enlinea_state, empty_init, ROT0, "Compumatic / CIC Play",                     "Cuatro en Linea (rev. A-06)", MACHINE_NOT_WORKING )
GAME( 1992, dardos,    0,        _4enlinea, dardos,    _4enlinea_state, empty_init, ROT0, "Oper Coin",                                 "Dardos",                      MACHINE_NOT_WORKING | MACHINE_MECHANICAL )
GAME( 1994, k7_olym,   0,        k7_olym,   k7_olym,   k7_state,        empty_init, ROT0, "K7 Kursaal / NMI Electronics",              "Olympic Darts K7 (v3.11)",    MACHINE_NOT_WORKING | MACHINE_MECHANICAL )
GAME( 1994, k7_olym30, k7_olym,  k7_olym,   k7_olym30, k7_state,        empty_init, ROT0, "K7 Kursaal / NMI Electronics",              "Olympic Darts K7 (v3.00)",    MACHINE_NOT_WORKING | MACHINE_MECHANICAL )
GAME( 1993, sprtdart,  0,        sprtdart,  sprtdart,  k7_state,        empty_init, ROT0, "Compumatic / Desarrollos y Recambios S.L.", "Sport Darts T.V.",            MACHINE_NOT_WORKING | MACHINE_MECHANICAL )
