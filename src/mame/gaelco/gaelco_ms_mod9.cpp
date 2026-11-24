// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 9/2 board ("SYSTEM 2")

    Sound, I/O, palette and video timing board of the later cages: a Z80
    with a YM3812 (two YM2203 on Euro League's board) and an OKI M5205,
    the sound EPROM (IC6) and a socket for a second one (IC11), 2 KB of
    RAM, the sound latch, two banks of 8 DIP switches, optocouplers for the
    inputs and 2048 palette entries of 12 bits. The sound has its own
    16 MHz crystal and the M5205 a 384 kHz resonator; the other crystal,
    divided by 4, is the pixel clock.

    Video timing: a GAL20V8 (IC18, labelled 92 and the game code) decodes
    counters into the blanking and the composite sync, see
    gaelco_ms_timing.cpp. Euro League's GAL gives the 256 x 224 picture of
    its original game, Big Karnak's the 320 pixels wide one of its original
    game; Splash's and World Rally's give 384 x 240, 16 pixels wider than
    their original games.

    The sound CPU memory map comes from a GAL16V8 (IC10, labelled 93 and
    the game code), the same logic on every dumped board. Deduced from its
    equations and the programs: pins 1-4 take A12-A15, pin 5 /WR, pin 6
    /MREQ, pin 7 /RFSH, pins 8, 9 and 11 control latch bits 0-2, pin 17
    A10 and pin 18 A11. Pins 15 and 16 select the EPROMs at IC6 and IC11,
    pin 12 gives their A14 and pin 13 inverted their A15. When pin 19 is
    low, pins 12-14 select a device, as a 74LS138 would:
      0  RAM                        4  unknown, unused by the programs
      1  YM3812 or first YM2203     5  control latch
      2  second YM2203              6  M5205 data latch
      3  sound latch                7  -
    This gives EPROM 0x0000-0x7fff at 0000-7fff, a 16 KB window at
    8000-bfff, the control latch at e000 (writes only), the data latch at
    e400, the YMs at e800 and ec00, the RAM at f000-f7ff, the sound latch
    at f800 and the unknown device at fc00.

    Control latch: bits 0-3 select the window (IC6 with bit 2 clear, IC11
    with it set; bits 0 and 1 are A14 and A15, bit 3 is IC11's A16), bit 6
    is the M5205 S1 (4 kHz samples with it clear, 8 kHz set) and bit 7 the
    M5205 reset. A flip-flop toggled by VCK selects the nibble of the data
    latch: the M5205 plays the low nibble of each byte first (Big Karnak's
    and Splash's programs swap the nibbles of their EPROM bytes for it), and
    the switch to the high nibble asks for the next byte with an NMI. The
    sound latch interrupts the Z80.

    The CPU boards see the board at 16 bytes, A1-A3 selecting the player
    inputs (0 and 1), DIP switches SW1 and SW2 (3 and 4), the interrupt
    acknowledge (6, any access) and the sound latch (7, writes). The board
    interrupts the CPU board at the start of the vertical blank, until the
    program acknowledges it.

    TODO:
    - what IC42 (GAL16V8 labelled 91 and the game code) does; its equations
      differ from game to game
    - the mix levels, taken from the drivers of the original games

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod9.h"

#include "cpu/z80/z80.h"

#include "ioprocs.h"
#include "jedparse.h"

#include <array>


namespace {

// GAL16V8 in complex mode: 64 product terms of 32 fuses, an intact fuse connects its column. The macrocells of pins 19
// down to 12 have eight terms each, the first one enabling the output; the product term disable fuses start at 2128
// and the XOR fuses at 2048
struct gal_column { u8 pin, complement, level; };
constexpr gal_column GAL16V8_COLUMNS[] = {
		{  1,  3,  2 }, {  2,  1,  0 }, {  3,  5,  4 }, {  4,  9,  8 }, {  5, 13, 12 }, {  6, 17, 16 }, {  7, 21, 20 },
		{  8, 25, 24 }, {  9, 29, 28 }, { 11, 31, 30 }, { 13, 27, 26 }, { 14, 23, 22 }, { 15, 19, 18 }, { 16, 15, 14 },
		{ 17, 11, 10 }, { 18,  7,  6 } };

class gal16v8
{
public:
	gal16v8(u8 const *data, u32 length, char const *tag);

	// the pin levels for the levels of the input pins, with pins 12-16 and 19 driven by the GAL
	u32 pins(u32 inputs) const;

private:
	bool term(u32 row, u32 levels) const
	{
		return m_enabled[row] && ((levels & m_high[row]) == m_high[row]) && !(levels & m_low[row]);
	}

	std::array<u32, 64> m_high; // the pins each product term needs high
	std::array<u32, 64> m_low; // and low
	std::array<bool, 64> m_enabled;
	std::array<bool, 64> m_blown; // all fuses blown
	u8 m_active_high; // macrocell n drives pin 19 - n
};

gal16v8::gal16v8(u8 const *data, u32 length, char const *tag) :
	m_high{ },
	m_low{ },
	m_enabled{ },
	m_blown{ },
	m_active_high(0)
{
	jed_data jed;
	auto file = util::ram_read(data, length);
	if (!file || (jedbin_parse(*file, &jed) != JEDERR_NONE) || (jed.numfuses != 2194) || !jed_get_fuse(&jed, 2192) ||
			!jed_get_fuse(&jed, 2193))
		throw emu_fatalerror("%s: bad GAL16V8 fuse map, or not in complex mode\n", tag);

	for (u32 row = 0; row < 64; row++)
	{
		m_enabled[row] = jed_get_fuse(&jed, 2128 + row);
		for (auto const &column : GAL16V8_COLUMNS)
		{
			if (!jed_get_fuse(&jed, row * 32 + column.level))
				m_high[row] |= 1U << column.pin;
			if (!jed_get_fuse(&jed, row * 32 + column.complement))
				m_low[row] |= 1U << column.pin;
		}
		m_blown[row] = !m_high[row] && !m_low[row];
	}
	for (u32 cell = 0; cell < 8; cell++)
	{
		if (jed_get_fuse(&jed, 2048 + cell))
			m_active_high |= 1U << cell;
	}
}

u32 gal16v8::pins(u32 inputs) const
{
	// the array sees the pins, so the outputs can feed each other; a product term with all its fuses blown is left
	// out of the sums, as jedutil does, and enables its output
	u32 levels = inputs;
	for (u32 pass = 0; pass < 8; pass++)
	{
		u32 const previous = levels;
		for (u8 const pin : { 12, 13, 14, 15, 16, 19 })
		{
			u32 const cell = 19 - pin;
			bool high = true;
			if (term(cell * 8, levels))
			{
				bool sum = false;
				for (u32 row = cell * 8 + 1; row < (cell + 1) * 8; row++)
					sum = sum || (!m_blown[row] && term(row, levels));
				high = sum == BIT(m_active_high, cell);
			}
			levels = (levels & ~(1U << pin)) | (u32(high) << pin);
		}
		if (levels == previous)
			break;
	}
	return levels;
}

// what the decoder selects in the 0xe000-0xffff pages
enum : u8 { SEL_RAM, SEL_YM1, SEL_YM2, SEL_SOUNDLATCH, SEL_UNKNOWN, SEL_CONTROL, SEL_DATA, SEL_NONE };

} // anonymous namespace


// player inputs from the connector
INPUT_PORTS_START( gaelco_ms_mod9 )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )
INPUT_PORTS_END


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD9, gaelco_ms_mod9_device, "gaelco_ms_mod9", "Gaelco Modular System MOD 9/2 board")
DEFINE_DEVICE_TYPE(GAELCO_MS_MOD9_YM2203, gaelco_ms_mod9_ym2203_device, "gaelco_ms_mod9_ym2203", "Gaelco Modular System MOD 9/2 board (YM2203 version)")


gaelco_ms_mod9_device_base::gaelco_ms_mod9_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_mixer_interface(mconfig, *this),
	device_palette_interface(mconfig, *this),
	device_gaelco_ms_timing_interface(mconfig, *this),
	m_opl(*this, "opl"),
	m_opn(*this, "opn%u", 1U),
	m_msm(*this, "msm"),
	m_cpu(*this, "cpu"),
	m_soundlatch(*this, "soundlatch"),
	m_screen(*this, finder_base::DUMMY_TAG),
	m_rom(*this, DEVICE_SELF),
	m_timing(*this, "timing"),
	m_decoder(*this, "decoder"),
	m_ram(*this, "ram", 0x800, ENDIANNESS_LITTLE),
	m_view(*this, "view"),
	m_irq_cb(*this),
	m_in_cb(*this, 0xff),
	m_palette_ram{ },
	m_control(0),
	m_adpcm_data(0),
	m_adpcm_high(false)
{
}

void gaelco_ms_mod9_device_base::device_add_mconfig(machine_config &config)
{
	Z80(config, m_cpu, 16_MHz_XTAL / 4);
	m_cpu->set_addrmap(AS_PROGRAM, &gaelco_ms_mod9_device_base::program_map);

	GENERIC_LATCH_8(config, m_soundlatch);
	m_soundlatch->data_pending_callback().set_inputline(m_cpu, INPUT_LINE_IRQ0);

	MSM5205(config, m_msm, 384_kHz_XTAL);
	m_msm->set_prescaler_selector(msm5205_device::S96_4B);
	m_msm->vck_legacy_callback().set(FUNC(gaelco_ms_mod9_device_base::adpcm_vck));
}

void gaelco_ms_mod9_device_base::device_start()
{
	set_timing_from_gal(*m_screen, clock() / 4, m_timing.target(), m_timing.bytes());
	build_decoder();
	m_screen->register_vblank_callback(vblank_state_delegate(&gaelco_ms_mod9_device_base::screen_vblank, this));

	save_item(NAME(m_palette_ram));
	save_item(NAME(m_control));
	save_item(NAME(m_adpcm_data));
	save_item(NAME(m_adpcm_high));
}

void gaelco_ms_mod9_device_base::device_reset()
{
	control_w(0);
	m_irq_cb(CLEAR_LINE);
}

void gaelco_ms_mod9_device_base::device_post_load()
{
	m_view.select(m_control & 0x0f);
}


void gaelco_ms_mod9_device_base::program_map(address_map &map)
{
	map(0x0000, 0xffff).view(m_view);
	for (int bank = 0; bank < 16; bank++)
		m_view[bank];
}

void gaelco_ms_mod9_device_base::build_decoder()
{
	gal16v8 const gal(m_decoder.target(), m_decoder.bytes(), tag());
	if (m_rom.bytes() < 0x10000)
		fatalerror("%s: no room for IC6 in the EPROM region\n", tag());

	// one view for each value of control latch bits 0-3, built in 1 KB pages from memory cycles (pin 6 low, pin 7 high)
	for (u8 bank = 0; bank < 16; bank++)
	{
		auto &view = m_view[bank];
		for (offs_t start = 0x0000; start < 0x10000; start += 0x400)
		{
			offs_t const end = start + 0x3ff;
			u8 select[2];
			for (int write = 0; write < 2; write++)
			{
				u32 const inputs = (u32(BIT(start, 12, 4)) << 1) | (u32(!write) << 5) | (1U << 7) | (u32(BIT(bank, 0)) << 8) |
						(u32(BIT(bank, 1)) << 9) | (u32(BIT(bank, 2)) << 11) | (u32(BIT(start, 10)) << 17) |
						(u32(BIT(start, 11)) << 18);
				u32 const pins = gal.pins(inputs);
				select[write] = BIT(pins, 19) ? SEL_NONE : BIT(pins, 12, 3);

				offs_t const window = (u32(BIT(~pins, 13)) << 15) | (u32(BIT(pins, 12)) << 14) | (start & 0x3fff);
				if (!write && !BIT(pins, 15))
					view.install_rom(start, end, &m_rom[window]);
				else if (!write && !BIT(pins, 16) && (m_rom.bytes() >= 0x30000))
					view.install_rom(start, end, &m_rom[0x10000 | (u32(BIT(bank, 3)) << 16) | window]);
			}

			if ((select[0] == SEL_RAM) && (select[1] == SEL_RAM))
			{
				view.install_ram(start, end, &m_ram[start & 0x7ff]);
				continue;
			}

			for (int write = 0; write < 2; write++)
			{
				switch (select[write])
				{
				case SEL_RAM:
					if (write)
						view.install_writeonly(start, end, &m_ram[start & 0x7ff]);
					else
						view.install_rom(start, end, &m_ram[start & 0x7ff]);
					break;

				case SEL_YM1:
				case SEL_YM2:
					if ((select[write] == SEL_YM2) && !m_opn[1])
						break;
					if (write)
						view.install_write_handler(start, end, write8sm_delegate(*this, [this, which = select[write] - SEL_YM1] (offs_t offset, u8 data) { ym_w(which, offset, data); }, "ym_w"));
					else
						view.install_read_handler(start, end, read8sm_delegate(*this, [this, which = select[write] - SEL_YM1] (offs_t offset) { return ym_r(which, offset); }, "ym_r"));
					break;

				case SEL_SOUNDLATCH:
					if (!write)
						view.install_read_handler(start, end, read8smo_delegate(*m_soundlatch, FUNC(generic_latch_8_device::read)));
					break;

				case SEL_CONTROL:
					if (write)
						view.install_write_handler(start, end, write8smo_delegate(*this, FUNC(gaelco_ms_mod9_device_base::control_w)));
					break;

				case SEL_DATA:
					if (write)
						view.install_write_handler(start, end, write8smo_delegate(*this, [this] (u8 data) { m_adpcm_data = data; }, "data_w"));
					break;
				}
			}
		}
	}
}


u8 gaelco_ms_mod9_device_base::io_r(offs_t offset)
{
	switch (BIT(offset, 1, 3))
	{
	case 0: return m_in_cb[0]();
	case 1: return m_in_cb[1]();
	case 3: return m_in_cb[2]();
	case 4: return m_in_cb[3]();
	case 6:
		if (!machine().side_effects_disabled())
			m_irq_cb(CLEAR_LINE);
		break;
	}
	return 0xff;
}

void gaelco_ms_mod9_device_base::io_w(offs_t offset, u8 data)
{
	switch (BIT(offset, 1, 3))
	{
	case 6: m_irq_cb(CLEAR_LINE); break;
	case 7: m_soundlatch->write(data); break;
	}
}

u16 gaelco_ms_mod9_device_base::io16_r(offs_t offset)
{
	return io_r(offset << 1) * 0x0101;
}

void gaelco_ms_mod9_device_base::io16_w(offs_t offset, u16 data, u16 mem_mask)
{
	io_w(offset << 1, ACCESSING_BITS_8_15 ? (data >> 8) : (data & 0xff));
}


u16 gaelco_ms_mod9_device_base::palette_r(offs_t offset)
{
	return m_palette_ram[offset & 0x7ff];
}

void gaelco_ms_mod9_device_base::palette_w(offs_t offset, u16 data, u16 mem_mask)
{
	offset &= 0x7ff;
	COMBINE_DATA(&m_palette_ram[offset]);
	update_pen(offset);
}

u8 gaelco_ms_mod9_device_base::palette8_r(offs_t offset)
{
	return palette_r(offset >> 1) >> (BIT(offset, 0) ? 0 : 8);
}

void gaelco_ms_mod9_device_base::palette8_w(offs_t offset, u8 data)
{
	palette_w(offset >> 1, data * 0x0101, BIT(offset, 0) ? 0x00ff : 0xff00);
}

void gaelco_ms_mod9_device_base::update_pen(offs_t entry)
{
	// green in bits 0-3, red in bits 4-7 and blue in bits 8-11
	u16 const data = m_palette_ram[entry];
	set_pen_color(entry, pal4bit(data >> 4), pal4bit(data), pal4bit(data >> 8));
}


u8 gaelco_ms_mod9_device_base::ym_r(u32 which, offs_t offset)
{
	return m_opl ? m_opl->read(offset & 1) : m_opn[which]->read(offset & 1);
}

void gaelco_ms_mod9_device_base::ym_w(u32 which, offs_t offset, u8 data)
{
	if (m_opl)
		m_opl->write(offset & 1, data);
	else
		m_opn[which]->write(offset & 1, data);
}

void gaelco_ms_mod9_device_base::control_w(u8 data)
{
	m_control = data;
	m_view.select(data & 0x0f);
	m_msm->s1_w(BIT(data, 6));
	m_msm->reset_w(BIT(data, 7));
}

void gaelco_ms_mod9_device_base::adpcm_vck(int state)
{
	m_adpcm_high = !m_adpcm_high;
	if (m_adpcm_high)
	{
		m_msm->data_w(m_adpcm_data >> 4);
		m_cpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);
	}
	else
	{
		m_msm->data_w(m_adpcm_data & 0x0f);
	}
}

void gaelco_ms_mod9_device_base::screen_vblank(screen_device &screen, bool vblank_state)
{
	if (vblank_state)
		m_irq_cb(ASSERT_LINE);
}


// YM3812 version

gaelco_ms_mod9_device::gaelco_ms_mod9_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	gaelco_ms_mod9_device_base(mconfig, GAELCO_MS_MOD9, tag, owner, clock)
{
}

void gaelco_ms_mod9_device::device_add_mconfig(machine_config &config)
{
	gaelco_ms_mod9_device_base::device_add_mconfig(config);

	YM3812(config, m_opl, 16_MHz_XTAL / 4);
	m_opl->add_route(ALL_OUTPUTS, *this, 0.80);
	m_msm->add_route(ALL_OUTPUTS, *this, 0.80);
}


// YM2203 version, on Euro League's board

gaelco_ms_mod9_ym2203_device::gaelco_ms_mod9_ym2203_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	gaelco_ms_mod9_device_base(mconfig, GAELCO_MS_MOD9_YM2203, tag, owner, clock)
{
}

void gaelco_ms_mod9_ym2203_device::device_add_mconfig(machine_config &config)
{
	gaelco_ms_mod9_device_base::device_add_mconfig(config);

	YM2203(config, m_opn[0], 16_MHz_XTAL / 4);
	m_opn[0]->add_route(ALL_OUTPUTS, *this, 0.40);
	YM2203(config, m_opn[1], 16_MHz_XTAL / 4);
	m_opn[1]->add_route(ALL_OUTPUTS, *this, 0.40);
	m_msm->add_route(ALL_OUTPUTS, *this, 0.20);
}
