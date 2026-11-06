// license:BSD-3-Clause
// copyright-holders:Tomás García-Merás Capote (ClawGrip)
/***************************************************************************

    Gaelco Modular System MOD 3 address decoder

    The Z80 CPU board (MOD 3, 3/2, 3/4) and the CPU sub boards that plug
    into it decode the CPU address space with an 82S147 PROM. Depending on
    the board, its address inputs come from CPU address lines, the write
    signal, bank latch bits and, on the 6809 sub board, which of its two
    CPUs makes the access.

    Outputs with bit 7 set select an EPROM or RAM socket (bits 4-6) and
    drive the upper socket address lines (bits 2-3), wired differently on
    each board. The other outputs reach the other boards of the cage, which
    tell their areas apart with the address lines and the bank latch.

    Known PROMs and their inputs 0-8 (Ln = bank latch bit n):
    P0314  MOD 3           A11 A12 A13 A14 /A15 high write L0 L1  Master Boy
    P0322  Z80 sub board   write A11 A12 A13 A14 A15 L0 L2 L4     Black Tiger
    P0312  Z80 sub board   A8 A9 A10 A11 A12 A13 A14 A15 L0       Slap Fight
    P0320  6809 sub board  L2 A11 A12 A13 A14 A15 L0 L1 CPU       Double Dragon
    P0318  6809 sub board  L2 L3 A12 A13 A14 A15 L0 L1 CPU        Gryzor
    Input 5 of P0314 is high during accesses.

    The device builds one memory view per bank latch value, with the areas
    the drivers install for each PROM output, and the latch picks the view.

    All the boards also swap the data lines and address lines A0-A11
    between the CPU and the EPROMs.

    TODO:
    - what the outputs without bit 7 mean to the other boards
    - bits 0-1 of the socket outputs

***************************************************************************/

#include "emu.h"
#include "gaelco_ms_mod3.h"

#include <vector>


DEFINE_DEVICE_TYPE(GAELCO_MS_MOD3_DECODER, gaelco_ms_mod3_decoder_device, "gaelco_ms_mod3_decoder", "Gaelco Modular System MOD 3 address decoder")


gaelco_ms_mod3_decoder_device::gaelco_ms_mod3_decoder_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, GAELCO_MS_MOD3_DECODER, tag, owner, clock),
	m_prom(*this, DEVICE_SELF),
	m_view(*this, "view"),
	m_install_cb(*this),
	m_lines{ },
	m_latch_mask(0),
	m_select(0),
	m_latch(0)
{
}

void gaelco_ms_mod3_decoder_device::map(address_map &map)
{
	map(0x0000, 0xffff).view(m_view);
	for (unsigned i = 0; i < views(); i++)
		m_view[i];
}

void gaelco_ms_mod3_decoder_device::device_start()
{
	m_install_cb.resolve();

	// the PROM output can change at every step of its lowest address line
	unsigned lowest = 16;
	for (u8 const line : m_lines)
	{
		if (line < 0x20)
			lowest = std::min<unsigned>(lowest, line & 0x0f);
	}
	offs_t const step = 1 << lowest;

	for (unsigned i = 0; i < views(); i++)
	{
		u8 const latch = view_latch(i);
		for (bool const write : { false, true })
		{
			offs_t start = 0;
			u8 code = decode(0, write, latch);
			for (offs_t address = step; address <= 0x10000; address += step)
			{
				if ((address == 0x10000) || (decode(address, write, latch) != code))
				{
					m_install_cb(m_view[i], start, address - 1, write, code, latch);
					if (address < 0x10000)
					{
						start = address;
						code = decode(address, write, latch);
					}
				}
			}
		}
	}

	save_item(NAME(m_latch));
}

void gaelco_ms_mod3_decoder_device::device_reset()
{
	latch_w(0);
}

void gaelco_ms_mod3_decoder_device::device_post_load()
{
	m_view.select(view_index(m_latch));
}


void gaelco_ms_mod3_decoder_device::latch_w(u8 data)
{
	m_latch = data;
	m_view.select(view_index(data));
}


u8 gaelco_ms_mod3_decoder_device::decode(offs_t address, bool write, u8 latch) const
{
	offs_t index = 0;
	for (unsigned i = 0; i < m_lines.size(); i++)
	{
		u8 const line = m_lines[i];
		bool state;
		switch (line & 0xf0)
		{
		case 0x00: state = BIT(address, line & 0x0f); break;
		case 0x10: state = !BIT(address, line & 0x0f); break;
		case 0x20: state = BIT(latch, line & 0x07); break;
		default:   state = (line == WRITE) ? write : (line == SELECT) ? bool(m_select) : true; break;
		}
		index |= offs_t(state) << i;
	}
	return m_prom[index];
}

unsigned gaelco_ms_mod3_decoder_device::views() const
{
	return 1U << std::popcount(unsigned(m_latch_mask));
}

unsigned gaelco_ms_mod3_decoder_device::view_index(u8 latch) const
{
	// the latch bits in the mask, packed
	unsigned index = 0;
	for (unsigned bit = 0, n = 0; bit < 8; bit++)
	{
		if (BIT(m_latch_mask, bit))
			index |= BIT(latch, bit) << n++;
	}
	return index;
}

u8 gaelco_ms_mod3_decoder_device::view_latch(unsigned index) const
{
	u8 latch = 0;
	for (unsigned bit = 0, n = 0; bit < 8; bit++)
	{
		if (BIT(m_latch_mask, bit))
			latch |= BIT(index, n++) << bit;
	}
	return latch;
}


void gaelco_ms_mod3_descramble(u8 *rom, offs_t length)
{
	for (offs_t i = 0; i < length; i++)
		rom[i] = bitswap<8>(rom[i], 5, 3, 7, 2, 0, 1, 4, 6);

	std::vector<u8> const buffer(rom, rom + length);
	for (offs_t i = 0; i < length; i++)
		rom[i] = buffer[(i & ~offs_t(0xfff)) | bitswap<12>(i, 10, 8, 6, 4, 0, 2, 7, 3, 1, 9, 11, 5)];
}
