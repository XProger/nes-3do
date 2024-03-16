#ifndef APU_H
#define APU_H

void apu_reset(void)
{
    memset(apu_reg, 0, sizeof(apu_reg));
}

uint32 apu_read(uint32 addr)
{
    uint32 bit;

    switch (addr)
    {
        case 0x16:
        case 0x17:
            bit = (apu_reg[addr] & 0x80) > 1;
            apu_reg[addr] <<= 1;
            return bit;
    }
    // TODO
    return 0;
}

void apu_write(uint32 addr, uint32 data)
{
    switch (addr)
    {
        case 0x14:
        {
            uint32 i;
            uint32 dma_addr = data << 8;
            for (i = 0; i < 256; i++)
            {
                ((uint8*)oam)[i] = cpu_read(dma_addr + i);
            }
            break;
        }
        case 0x16:
        case 0x17:
            apu_reg[addr] = joy_state[addr & 1];
            break;
    }
    // TODO
}

#endif
