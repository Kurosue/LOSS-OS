#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include "cpu/portio.h"
#include "lib/font.h"

// Source : OSWiki.dev dan StackOverflow
// Woila Penyelamat OSWiki 

// VGA text dimensions (for cursor)
#define VGA_WIDTH     640
#define VGA_HEIGHT    480
#define VGA_PITCH     (VGA_WIDTH/8)
#define VGA_PLANE_SIZE (VGA_PITCH * VGA_HEIGHT)

// I/O port addresses (for clarity)
#define VGA_MISC_WRITE   0x3C2
#define VGA_SEQ_INDEX    0x3C4
#define VGA_SEQ_DATA     0x3C5
#define VGA_CRTC_INDEX   0x3D4
#define VGA_CRTC_DATA    0x3D5
#define VGA_GC_INDEX     0x3CE
#define VGA_GC_DATA      0x3CF
#define VGA_ACT_INDEX    0x3C0
#define VGA_INPUT_STATUS 0x3DA
#define VGA_MEMORY       0xC0000000 + 0xA0000 // Tambah 0xC0000000 karena paging virtual memori

void vga_init(void);
void vga_draw_pixel(int x, int y, uint8_t color);
void vga_clear(uint8_t color);
void vga_draw_cursor(int cx, int cy, uint8_t color);
void vga_clear_cursor(int cx, int cy);
void vga_draw_char(int x, int y, char c, uint8_t color);

#endif // GRAPHICS_H
