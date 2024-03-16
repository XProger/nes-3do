#ifndef CART_H
#define CART_H

#include "common.h"

typedef struct
{
    uint32 magic;
    uint8 prg_banks;
    uint8 chr_banks;
    uint8 flags6;
    uint8 flags7;
    uint8 flags8;
    uint8 prg_chr_rom;
    uint8 prg_ram;
    uint8 chr_ram;
    uint8 timing;
    uint8 sys_type;
    uint8 ext_type;
    uint8 def_device;
} NES_HEADER;

void cart_load(const uint8 *data)
{
    NES_HEADER *header = (NES_HEADER*)data;
    
    data += sizeof(NES_HEADER);
    if (header->flags6 & 0x04)
    {
        data += 512;
    }

    ASSERT(!((header->flags7 & 0x0C) == 0x08)); //  TODO file type

    prg_banks_cur = 0;
    prg_banks = header->prg_banks;
    chr_banks = header->chr_banks;

    table_mirror = header->flags6 & 1;
    mapper = (header->flags7 & 0xF0) | (header->flags6 >> 4);

    LOG("mirror: %d", table_mirror);
    LOG("mapper: %d", mapper);

    memcpy(prg_rom, data, prg_banks * 1024 * 16);
    data += prg_banks * 1024 * 16;
    memcpy(chr_rom, data, (chr_banks ? chr_banks : 1) * 1024 * 8);
}

#endif
