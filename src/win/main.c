#include <stdio.h>
#include <windows.h>

#include "common.h"
#include "cart.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"

uint8 mem[256 * 1024];

sint32 nes_load(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    fread(mem, 1, sizeof(mem), f);
    fclose(f);
    return 1;
}

#define WND_SCALE       3
#define WND_WIDTH       (FRAME_WIDTH * WND_SCALE)
#define WND_HEIGHT      (FRAME_HEIGHT * WND_SCALE)

uint32 SCREEN[FRAME_WIDTH * FRAME_HEIGHT];

HWND hWnd;
HDC hDC;
uint32 quit;

LRESULT CALLBACK app_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_ACTIVATE:
        {
            joy_state[0] = joy_state[1] = 0;
            break;
        }

        case WM_DESTROY :
        {
            PostQuitMessage(0);
            quit = 1;
            break;
        }

        case WM_KEYDOWN    :
        case WM_KEYUP      :
        case WM_SYSKEYUP   :
        case WM_SYSKEYDOWN :
        {
            uint8 key = 0;
            switch (wParam)
            {
                case VK_RIGHT  : key = (1 << 0); break; // R
                case VK_LEFT   : key = (1 << 1); break; // L
                case VK_DOWN   : key = (1 << 2); break; // D
                case VK_UP     : key = (1 << 3); break; // U
                case VK_RETURN : key = (1 << 4); break; // START
                case VK_SPACE  : key = (1 << 5); break; // SELECT
                case 'Z'       : key = (1 << 6); break; // B
                case 'X'       : key = (1 << 7); break; // A
            }

            if (msg != WM_KEYUP && msg != WM_SYSKEYUP) {
                joy_state[0] |= key;
            } else {
                joy_state[0] &= ~key;
            }

            break;
        }

        default :
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void app_init(void)
{
    WNDCLASSEX wcex;
    RECT r = { 0, 0, WND_WIDTH, WND_HEIGHT };

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
    int wx = (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2;

    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.hInstance      = GetModuleHandle(NULL);
    wcex.hIcon          = LoadIcon(wcex.hInstance, "MAINICON");
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszClassName  = "NES3DO";
    wcex.hIconSm        = wcex.hIcon;
    wcex.lpfnWndProc    = &app_proc;
    RegisterClassEx(&wcex);

    hWnd = CreateWindow("NES3DO", "NES-3DO", WS_OVERLAPPEDWINDOW, wx + r.left, wy + r.top, r.right - r.left, r.bottom - r.top, 0, 0, wcex.hInstance, 0);
    hDC = GetDC(hWnd);

    ShowWindow(hWnd, SW_SHOWDEFAULT);
}

void app_messages(void)
{
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void pal_update()
{
    table_pal[0x10] =
    table_pal[0x14] =
    table_pal[0x18] =
    table_pal[0x1C] = table_pal[0x00];
}

void draw_sprite(sint32 index, uint32 chr, uint32 pal, uint32 flip, uint32 trans, sint32 x, sint32 y)
{
    const uint8 *ptr = (chr_rom + index * 16) + chr * (16 * 16 * 16);
    sint32 ix, iy, i;

    for (iy = 0; iy < 8; iy++)
    {
        uint8 a = ptr[iy];
        uint8 b = ptr[iy + 8];

        for (ix = 0; ix < 8; ix++)
        {
            i = (a >> 7) | ((b >> 6) & 2);

            a <<= 1;
            b <<= 1;

            if (trans && i == 0)
                continue;

            if (i != 0)
            {
                i |= pal;
            }

            sint32 sx = x + ((flip & 1) ? (7 - ix) : ix);
            sint32 sy = y + ((flip & 2) ? (7 - iy) : iy);

            if (sx >= 0 && sx <= 255 && sy >= 0 && sy <= 239)
            {
                SCREEN[sy * FRAME_WIDTH + sx] = screen_pal[table_pal[i]];
            }
        }
    }
}

void draw_bg_row(row)
{
    uint32 table_addr = 0x2000 + (PPU_CTRL & 3) * 0x400;
    
    uint32 chr = (PPU_CTRL & PPU_CTRL_PAT_BG) >> 4;
    sint32 scroll_x = PPU_SCROLL_X(PPU_SCROLL);
    sint32 scroll_y = PPU_SCROLL_Y(PPU_SCROLL);

    sint32 cx = (scroll_x >> 3);
    sint32 cy = (scroll_y >> 3);
    sint32 fx = (scroll_x & 7);
    sint32 fy = (scroll_y & 7);

    sint32 y, x;
    sint32 w = 32 + ((PPU_MASK & PPU_MASK_BG_TRIM) ? 1 : 0);

    pal_update();

    y = cy + row;
    if (y >= 30)
    {
        table_addr += 0x800;
        y -= 30;
    }

    for (x = 0; x < w; x++, cx++)
    {
        if (cx >= 32)
        {
            table_addr += 0x400;
            cx -= 32;
        }

        uint8 *table = get_vram_ptr(table_addr);

        uint32 spr = table[y * 32 + cx];
        uint32 pal = table[30 * 32 + ((y >> 2) << 3) + (cx >> 2)];

        uint32 sx = (cx & 2);
        uint32 sy = (y & 2) << 1;

        pal = (pal >> (sx | sy)) & 3;

        draw_sprite(spr, chr, (pal << 2), 0, 0, x * 8 - fx, row * 8 - fy);
    }
}

void draw_spr(void)
{
    uint32 i, id;
    PPU_SPRITE *spr = oam;
    uint32 chr = (PPU_CTRL & PPU_CTRL_PAT_SP) >> 4;

    pal_update();

    for (i = 0; i < 64; i++, spr++)
    {
        if (spr->y >= 0xEF)
            continue;

        if (PPU_CTRL & PPU_CTRL_SIZE)
        {
            chr = (spr->id & 1);
            id = spr->id & ~1;
            draw_sprite(id, chr, ((spr->attr & PPU_SPR_PAL) << 2) | (1 << 4), spr->attr >> 6, 1, spr->x, spr->y);
            draw_sprite(id + 1, chr, ((spr->attr & PPU_SPR_PAL) << 2) | (1 << 4), spr->attr >> 6, 1, spr->x, spr->y + 8);
        }
        else
        {
            id = spr->id;
            draw_sprite(id, chr, ((spr->attr & PPU_SPR_PAL) << 2) | (1 << 4), spr->attr >> 6, 1, spr->x, spr->y);
        }
    }
}

void app_blit(void)
{
    static const BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), FRAME_WIDTH, -FRAME_HEIGHT, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
    StretchDIBits(hDC, 0, 0, WND_WIDTH, WND_HEIGHT, 0, 0, FRAME_WIDTH, FRAME_HEIGHT, SCREEN, &bmi, DIB_RGB_COLORS, SRCCOPY);
    Sleep(1);
}

int main()
{
    app_init();

    if (!nes_load("roms\\smb.nes"))
        return -1;

    cart_load(mem);

    cpu_reset();
    ppu_reset();
    apu_reset();

    while (!quit)
    {
        app_messages();

        cpu_clock(48);
        ppu_scan();

        if ((scanline <= 240) && ((scanline & 7) == 0))
        {
            if (PPU_MASK & PPU_MASK_BG_EN)
            {
                draw_bg_row(scanline >> 3);
            }
        }

        if (scanline == 241)
        {
            if (PPU_MASK & PPU_MASK_SP_EN)
            {
                draw_spr();
            }
            app_blit();
        }
    }

    return 0;
}
