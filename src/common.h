#ifndef COMMON_H
#define COMMON_H

#include <string.h>

#ifdef _DEBUG
    #define DBG_BREAK __debugbreak()
    #define LOG(...) { printf(__VA_ARGS__); printf("\n"); }
    #define ASSERT(expr) if (!(expr)) { LOG("ASSERT:\n  %s:%d\n  %s => %s", __FILE__, __LINE__, __FUNCTION__, #expr); DBG_BREAK; }
#else
    #define LOG(...)
    #define ASSERT(x)
#endif

#define TODO ASSERT(0);

#define FRAME_WIDTH     256
#define FRAME_HEIGHT    240

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;
typedef signed char     sint8;
typedef signed short    sint16;
typedef signed int      sint32;

static const uint32 screen_pal[64] = {
    0xFF626262, 0xFF001FB2, 0xFF2404C8, 0xFF5200B2, 0xFF730076, 0xFF800024, 0xFF730B00, 0xFF522800, 0xFF244400, 0xFF005700, 0xFF005C00, 0xFF005324, 0xFF003C76, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFABABAB, 0xFF0D57FF, 0xFF4B30FF, 0xFF8A13FF, 0xFFBC08D6, 0xFFD21269, 0xFFC72E00, 0xFF9D5400, 0xFF607B00, 0xFF209800, 0xFF00A300, 0xFF009942, 0xFF007DB4, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFF53AEFF, 0xFF9085FF, 0xFFD365FF, 0xFFFF57FF, 0xFFFF5DCF, 0xFFFF7757, 0xFFFA9E00, 0xFFBDC700, 0xFF7AE700, 0xFF43F611, 0xFF26EF7E, 0xFF2CD5F6, 0xFF4E4E4E, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFFB6E1FF, 0xFFCED1FF, 0xFFE9C3FF, 0xFFFFBCFF, 0xFFFFBDF4, 0xFFFFC6C3, 0xFFFFD59A, 0xFFE9E681, 0xFFCEF481, 0xFFB6FB9A, 0xFFA9FAC3, 0xFFA9F0F4, 0xFFB8B8B8, 0xFF000000, 0xFF000000
};

uint8 joy_state[2];

typedef struct
{
    uint8 y;
    uint8 id;
    uint8 attr;
    uint8 x;
} PPU_SPRITE;

uint8 prg_rom[8 * 1024 * 16];
uint8 chr_rom[2 * 1024 * 8];
uint8 ram[2048];

uint32 table_mirror;
uint8 table_name[2][1024];
uint8 table_pal[32];

uint8 apu_reg[24];

PPU_SPRITE oam[64];

uint32 prg_banks_cur;
uint32 prg_banks;
uint32 chr_banks;
uint32 mapper;

uint32 cpu_read(uint32 addr);
void cpu_write(uint32 addr, uint8 data);
void cpu_nmi(void);

#endif
