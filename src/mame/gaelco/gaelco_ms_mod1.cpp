// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 1 sound board

    Z80, 2 x YM2203C + 2 x Y3014B, one EPROM (IC12), 8 KB RAM, 82S123
    decoder PROM (IC20) and two banks of 8 DIP switches. Some games add a
    small sub board with an OKI M5205 and its latch; its VCK is then the
    interrupt source. Gryzor and Double Dragon use a 6809 version.

    The memory map comes from the decoder PROM. Deduced from the PROM
    contents and the sound programs: the PROM is addressed by A15, A14, A13,
    latch bit 7 and A11. A12 isn't decoded, so every 2 KB area is mirrored
    4 KB above. EPROM A15 is CPU A15. Outputs, all but bit 0 active low:
      bit 0  EPROM A14
      bit 1  EPROM select
      bit 2  RAM select
      bit 3  strobe, acknowledges the interrupt (P0102 reads it)
      bit 4  YM2203s: writes +0/+1 #1, +2/+3 #2, +4 interrupt acknowledge;
             reads +8/+9 #1, +A/+B #2
      bit 5  sub board latch: bits 0-3 M5205 data, bit 4 M5205 reset,
             bit 7 bank
      bit 6  strobe, acknowledges the interrupt (with the latch, it's where
             the YM2203s are when the bank bit is set)
      bit 7  strobe, acknowledges the NMI (used with P0101)

    The top 16 bytes of the RAM are shared with the CPU board. The sound
    programs copy the connector inputs (YM2203 #1 ports) and the DIP
    switches (SW1 on YM2203 #2 port B, SW2 on port A) there and take the
    sound commands from there.

    Known PROMs:
    PROM   ROM               RAM        YM2203  latch  games
    P0101  0000-7fff         c000-dfff  e000    -      Black Tiger, Master Boy (PROM not dumped)
    P0102  0000-3fff         c000-dfff  e000    -      Slap Fight
    105    0000-bfff banked  e000-ffff  c000    8000   Bad Lands, Legend of Hero Tonma, Power Spikes,
                                                       Shadow Warriors (PROM not dumped)
    110    0000-bfff banked  c000-dfff  e000    8000   Automat, Dragon Ninja, Fighting Fantasy, Raiden,
                                                       Secret Agent, Toki, Blood Bros. (PROM not dumped)
    112    0000-7fff         8000-9fff  c000    -      Twin Cobra
    116    0000-bfff         e000-ffff  c000    8000   Last Duel
    -      0000-7fff, c000-ffff banked
                             8000-9fff  a000    c000   Gals Panic (PROM not dumped)

    The banked area shows EPROM 8000-bfff or c000-ffff, depending on the
    latch bank bit.

    TODO:
    - clocks (there's no crystal on the board)
    - interrupt source without the M5205 sub board
    - Cabal's program wants RAM at 2000, the YM2203s at 4000 and EPROM A15
      from the bank bit, which no known PROM or board wiring gives
    - 6809 version (P0109 and P0111 PROMs)

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod1.h"


// player inputs from the 18-pin connector, read through the YM2203 #1 ports
INPUT_PORTS_START( gaelco_ms_mod1 )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
INPUT_PORTS_END


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD1, gaelco_ms_mod1_device, "gaelco_ms_mod1", "Gaelco Modular System MOD 1 sound board")
DEFINE_DEVICE_TYPE(GAELCO_MS_MOD1_ADPCM, gaelco_ms_mod1_adpcm_device, "gaelco_ms_mod1_adpcm", "Gaelco Modular System MOD 1 sound board with M5205 sub board")


gaelco_ms_mod1_device::gaelco_ms_mod1_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	gaelco_ms_mod1_device(mconfig, GAELCO_MS_MOD1, tag, owner, clock)
{
}

gaelco_ms_mod1_device::gaelco_ms_mod1_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_mixer_interface(mconfig, *this),
	m_cpu(*this, "cpu"),
	m_ym(*this, "ym%u", 1U),
	m_rom(*this, DEVICE_SELF),
	m_prom(*this, "prom"),
	m_ram(*this, "ram", 0x2000, ENDIANNESS_LITTLE),
	m_view(*this, "view"),
	m_in_cb(*this, 0xff),
	m_int_period(attotime::zero),
	m_host_nmi(false)
{
}

void gaelco_ms_mod1_device::device_add_mconfig(machine_config &config)
{
	Z80(config, m_cpu, DERIVED_CLOCK(1, 1));
	m_cpu->set_addrmap(AS_PROGRAM, &gaelco_ms_mod1_device::program_map);

	YM2203(config, m_ym[0], DERIVED_CLOCK(1, 1));
	m_ym[0]->port_a_read_callback().set([this] () -> u8 { return m_in_cb[0](); });
	m_ym[0]->port_b_read_callback().set([this] () -> u8 { return m_in_cb[1](); });
	m_ym[0]->add_route(ALL_OUTPUTS, *this, 0.50);

	YM2203(config, m_ym[1], DERIVED_CLOCK(1, 1));
	m_ym[1]->port_a_read_callback().set([this] () -> u8 { return m_in_cb[2](); });
	m_ym[1]->port_b_read_callback().set([this] () -> u8 { return m_in_cb[3](); });
	m_ym[1]->add_route(ALL_OUTPUTS, *this, 0.50);
}

void gaelco_ms_mod1_device::device_start()
{
	// one view for each value of the bank bit, built from the PROM in 2 KB pages
	for (unsigned bank = 0; bank < 2; bank++)
	{
		for (offs_t start = 0x0000; start < 0x10000; start += 0x800)
		{
			offs_t const end = start + 0x7ff;
			u8 const data = m_prom[(BIT(start, 13, 3) << 2) | (bank << 1) | BIT(start, 11)];
			u8 const sel = ~data;

			if (BIT(sel, 2))
			{
				m_view[bank].install_ram(start, end, &m_ram[start & 0x1fff]);
				continue;
			}

			if (BIT(sel, 1))
				m_view[bank].install_rom(start, end, &m_rom[((start & 0xbfff) | (BIT(data, 0) << 14)) & (m_rom.length() - 1)]);

			bool const ym = BIT(sel, 4);
			bool const latch = BIT(sel, 5);
			bool const intack = BIT(sel, 3) || BIT(sel, 6);
			bool const nmiack = BIT(sel, 7);

			if (!BIT(sel, 1) && (ym || intack || nmiack))
			{
				m_view[bank].install_read_handler(start, end, read8sm_delegate(*this, [this, ym, intack, nmiack] (offs_t offset) -> u8
				{
					u8 const data = ym ? ym_r(offset) : 0xff;
					if (!machine().side_effects_disabled())
					{
						if (intack)
							int_ack();
						if (nmiack)
							nmi_ack();
					}
					return data;
				}, "decoder_r"));
			}

			if (ym || latch || intack || nmiack)
			{
				m_view[bank].install_write_handler(start, end, write8sm_delegate(*this, [this, ym, latch, intack, nmiack] (offs_t offset, u8 data)
				{
					if (ym)
						ym_w(offset, data);
					if (latch)
						latch_w(data);
					if (intack)
						int_ack();
					if (nmiack)
						nmi_ack();
				}, "decoder_w"));
			}
		}
	}

	m_int_timer = timer_alloc(FUNC(gaelco_ms_mod1_device::int_tick), this);
	if (m_int_period != attotime::zero)
		m_int_timer->adjust(m_int_period, 0, m_int_period);

	save_item(NAME(m_bank));
}

void gaelco_ms_mod1_device::device_post_load()
{
	m_view.select(m_bank);
}

void gaelco_ms_mod1_device::device_reset()
{
	set_bank(0);
	m_cpu->set_input_line(INPUT_LINE_IRQ0, CLEAR_LINE);
	m_cpu->set_input_line(INPUT_LINE_NMI, CLEAR_LINE);
}


void gaelco_ms_mod1_device::program_map(address_map &map)
{
	map(0x0000, 0xffff).view(m_view);
	m_view[0];
	m_view[1];
}


u8 gaelco_ms_mod1_device::host_r(offs_t offset)
{
	return m_ram[0x1ff0 + offset];
}

void gaelco_ms_mod1_device::host_w(offs_t offset, u8 data)
{
	m_ram[0x1ff0 + offset] = data;
	if (m_host_nmi)
		m_cpu->set_input_line(INPUT_LINE_NMI, ASSERT_LINE);
}

u16 gaelco_ms_mod1_device::host16_r(offs_t offset)
{
	return (m_ram[0x1ff0 + (offset << 1)] << 8) | m_ram[0x1ff1 + (offset << 1)];
}

void gaelco_ms_mod1_device::host16_w(offs_t offset, u16 data, u16 mem_mask)
{
	if (ACCESSING_BITS_8_15)
		host_w(offset << 1, data >> 8);
	if (ACCESSING_BITS_0_7)
		host_w((offset << 1) + 1, data & 0xff);
}


void gaelco_ms_mod1_device::latch_w(u8 data)
{
	// the latch is on the M5205 sub board
}

void gaelco_ms_mod1_device::set_bank(u8 bank)
{
	m_bank = bank;
	m_view.select(bank);
}

u8 gaelco_ms_mod1_device::ym_r(offs_t offset)
{
	return m_ym[BIT(offset, 1)]->read(offset & 1);
}

void gaelco_ms_mod1_device::ym_w(offs_t offset, u8 data)
{
	if (BIT(offset, 2))
		int_ack();
	else
		m_ym[BIT(offset, 1)]->write(offset & 1, data);
}

void gaelco_ms_mod1_device::int_w(int state)
{
	if (state)
		m_cpu->set_input_line(INPUT_LINE_IRQ0, ASSERT_LINE);
}

TIMER_CALLBACK_MEMBER(gaelco_ms_mod1_device::int_tick)
{
	int_w(1);
}

void gaelco_ms_mod1_device::int_ack()
{
	m_cpu->set_input_line(INPUT_LINE_IRQ0, CLEAR_LINE);
}

void gaelco_ms_mod1_device::nmi_ack()
{
	m_cpu->set_input_line(INPUT_LINE_NMI, CLEAR_LINE);
}


gaelco_ms_mod1_adpcm_device::gaelco_ms_mod1_adpcm_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	gaelco_ms_mod1_device(mconfig, GAELCO_MS_MOD1_ADPCM, tag, owner, clock),
	m_msm(*this, "msm")
{
}

void gaelco_ms_mod1_adpcm_device::set_adpcm_prescaler(int select)
{
	m_msm.lookup()->set_prescaler_selector(select);
}

void gaelco_ms_mod1_adpcm_device::device_add_mconfig(machine_config &config)
{
	gaelco_ms_mod1_device::device_add_mconfig(config);

	MSM5205(config, m_msm, 384'000); // unverified
	m_msm->set_prescaler_selector(msm5205_device::S48_4B);
	m_msm->vck_callback().set(FUNC(gaelco_ms_mod1_adpcm_device::int_w));
	m_msm->add_route(ALL_OUTPUTS, *this, 0.80);
}

void gaelco_ms_mod1_adpcm_device::latch_w(u8 data)
{
	m_msm->data_w(data & 0x0f);
	m_msm->reset_w(BIT(data, 4));
	set_bank(BIT(data, 7));
}
