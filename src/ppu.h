#ifndef PPU_H
#define PPU_H

#include "common.h"

#define PPU_CTRL_TBL_X      (1 << 0)
#define PPU_CTRL_TBL_Y      (1 << 0)
#define PPU_CTRL_INC        (1 << 2)
#define PPU_CTRL_PAT_SP     (1 << 3)
#define PPU_CTRL_PAT_BG     (1 << 4)
#define PPU_CTRL_SIZE       (1 << 5)
#define PPU_CTRL_SLAVE      (1 << 6)
#define PPU_CTRL_NMI        (1 << 7)

#define PPU_MASK_GRAY       (1 << 0)
#define PPU_MASK_BG_TRIM    (1 << 1)
#define PPU_MASK_SP_TRIM    (1 << 2)
#define PPU_MASK_BG_EN      (1 << 3)
#define PPU_MASK_SP_EN      (1 << 4)
#define PPU_MASK_EN         (PPU_MASK_BG_EN | PPU_MASK_SP_EN)

#define PPU_STATUS_SP_OV    (1 << 5)
#define PPU_STATUS_SP_HIT   (1 << 6)
#define PPU_STATUS_VBLANK   (1 << 7)

#define PPU_SPR_PAL         ((1 << 0) | (1 << 1))
#define PPU_SPR_PRIO        (1 << 5)
#define PPU_SPR_FLIP_H      (1 << 6)
#define PPU_SPR_FLIP_V      (1 << 7)

#define PPU_SCROLL_X(reg)   (reg & 0x00FF)
#define PPU_SCROLL_Y(reg)   ((reg & 0xFF00) >> 8)

#define TBL_MIRROR_H        0
#define TBL_MIRROR_V        1

uint32 PPU_CTRL;
uint32 PPU_MASK;
uint32 PPU_STATUS;
uint32 OAM_ADDR;
uint32 OAM_DATA;
uint32 PPU_SCROLL;
uint32 PPU_ADDR;
uint32 PPU_DATA;
uint32 OAM_DMA;

sint32 scanline;
uint32 latch;
uint32 latch_data;

void ppu_reset(void)
{
    scanline = -1;
    latch = 0;
}

uint8* get_vram_ptr(uint32 addr)
{
    if (addr >= 0x0000 && addr <= 0x1FFF)
    {
        return chr_rom + addr;
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        uint32 tile = addr & 0x03FF;
        addr &= 0x0FFF;

        if (table_mirror == TBL_MIRROR_V)
        {
            if (addr >= 0x0000 && addr <= 0x03FF)
                return table_name[0] + tile;
            if (addr >= 0x0400 && addr <= 0x07FF)
                return table_name[1] + tile;
            if (addr >= 0x0800 && addr <= 0x0BFF)
                return table_name[0] + tile;
            if (addr >= 0x0C00 && addr <= 0x0FFF)
                return table_name[1] + tile;
        }
        else
        {
            if (addr >= 0x0000 && addr <= 0x03FF)
                return table_name[0] + tile;
            if (addr >= 0x0400 && addr <= 0x07FF)
                return table_name[0] + tile;
            if (addr >= 0x0800 && addr <= 0x0BFF)
                return table_name[1] + tile;
            if (addr >= 0x0C00 && addr <= 0x0FFF)
                return table_name[1] + tile;
        }
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr &= 0x1F;
        if (addr == 0x10 || addr == 0x14 || addr == 0x18 || addr == 0x1C)
        {
            addr = 0x00;
        }
        return table_pal + addr;
    }

    ASSERT(0);
    return NULL;
}

uint32 vram_read(uint32 addr)
{
    uint8 *ptr = get_vram_ptr(addr);
    return ptr ? *ptr : 0;
}

void vram_write(uint32 addr, uint32 data)
{
    uint8 *ptr = get_vram_ptr(addr);
    if (ptr)
    {
        *ptr = data;
    }
}

uint32 ppu_read(uint32 addr)
{
    switch (addr)
    {
        case 0:
        case 1:
            break;
        case 2:
        {
            uint32 t = (PPU_STATUS & 0xE0) | (PPU_DATA & 0x1F);
            PPU_STATUS &= ~PPU_STATUS_VBLANK;
            latch = 0;
            return t;
        }
        case 3:
            break;
        case 4:
            return ((uint8*)oam)[OAM_ADDR];
        case 5:
        case 6:
            break;
        case 7:
        {
            uint32 t = PPU_DATA;
            PPU_DATA = vram_read(PPU_ADDR);
            if (PPU_ADDR >= 0x3F00)
            {
                t = PPU_DATA;
            }
            PPU_ADDR += (PPU_CTRL & PPU_CTRL_INC) ? 32 : 1;
            return t;
        }
    }
    return 0;
}

void ppu_write(uint32 addr, uint32 data)
{
    switch (addr)
    {
        case 0:
            PPU_CTRL = data;
            break;
        case 1:
            PPU_MASK = data;
            break;
        case 2:
            break;
        case 3:
            OAM_ADDR = data;
            break;
        case 4:
            ((uint8*)oam)[OAM_ADDR] = data;
            break;
        case 5:
            if (latch == 0)
            {
                latch_data = data;
            }
            else
            {
                PPU_SCROLL = latch_data | (data << 8);
            }
            latch ^= 1;
            break;
        case 6:
            if (latch == 0)
            {
                latch_data = (data << 8);
            }
            else
            {
                PPU_ADDR = latch_data | data;
            }
            latch ^= 1;
            break;
        case 7:
            vram_write(PPU_ADDR, data);
            PPU_ADDR += (PPU_CTRL & PPU_CTRL_INC) ? 32 : 1;
            break;
    }
}

void ppu_scan(void)
{
    if (scanline == -1)
    {
        PPU_STATUS &= ~(PPU_STATUS_VBLANK | PPU_STATUS_SP_OV | PPU_STATUS_SP_HIT);
    }

    scanline++;

    if (scanline <= 240)
    {
        if (((PPU_MASK & PPU_MASK_EN) == PPU_MASK_EN) && (scanline == oam[0].y)) // TODO
        {
            PPU_STATUS |= PPU_STATUS_SP_HIT;
        }
    }
    else if (scanline == 241)
    {
        PPU_STATUS |= PPU_STATUS_VBLANK;
        if (PPU_CTRL & PPU_CTRL_NMI)
        {
            cpu_nmi();
        }
    }
    else if (scanline == 261)
    {
        scanline = -1;
    }
}

#endif
