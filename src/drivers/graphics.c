#include <stdint.h>
#include "drivers/graphics.h"

// Source : OSWiki.dev dan StackOverflow
// Woila Penyelamat OSWiki 

void vga_init(void) {
    uint8_t tmp;
    out(VGA_CRTC_INDEX, 0x11);
    tmp = in(VGA_CRTC_DATA);
    out(VGA_CRTC_DATA, tmp & 0x7F);

    out(VGA_MISC_WRITE, 0xE3);  // See VGA mode 12h s

    // Sequencer registers (0x3C4/3C5)
    out(VGA_SEQ_INDEX, 0x00); out(VGA_SEQ_DATA, 0x03);  // Reset (synchronous)
    out(VGA_SEQ_INDEX, 0x01); out(VGA_SEQ_DATA, 0x01);  // Clocking Mode = 1 (no shift)
    out(VGA_SEQ_INDEX, 0x02); out(VGA_SEQ_DATA, 0x0F);  // Map Mask = 0x0F (enable all 4 planes)
    out(VGA_SEQ_INDEX, 0x03); out(VGA_SEQ_DATA, 0x00);  // Character Map Select = 0
    out(VGA_SEQ_INDEX, 0x04); out(VGA_SEQ_DATA, 0x02);  // Memory Mode = 2 (graphics)

    // Graphics Controller registers (0x3CE/3CF)
    out(VGA_GC_INDEX, 0x00); out(VGA_GC_DATA, 0x00);   // Set/Reset = 0
    out(VGA_GC_INDEX, 0x01); out(VGA_GC_DATA, 0x00);   // Enable Set/Reset = 0
    out(VGA_GC_INDEX, 0x02); out(VGA_GC_DATA, 0x00);   // Color Compare = 0
    out(VGA_GC_INDEX, 0x03); out(VGA_GC_DATA, 0x00);   // Data Rotate = 0 (write mode 0)
    out(VGA_GC_INDEX, 0x04); out(VGA_GC_DATA, 0x00);   // Read Map Select = 0 (plane 0)
    out(VGA_GC_INDEX, 0x05); out(VGA_GC_DATA, 0x00);   // Graphics Mode = 0 (graphics)
    out(VGA_GC_INDEX, 0x06); out(VGA_GC_DATA, 0x05);   // Miscellaneous = 0x05 (enable even/odd)
    out(VGA_GC_INDEX, 0x07); out(VGA_GC_DATA, 0x0F);   // Color Don't Care = 0x0F
    out(VGA_GC_INDEX, 0x08); out(VGA_GC_DATA, 0xFF);   // Bit Mask = 0xFF (no mask)

    // CRT Controller registers (0x3D4/3D5) – horizontal timings
    out(VGA_CRTC_INDEX, 0x00); out(VGA_CRTC_DATA, 0x5F); // Horizontal Total = 95
    out(VGA_CRTC_INDEX, 0x01); out(VGA_CRTC_DATA, 0x4F); // H. Display End = 79
    out(VGA_CRTC_INDEX, 0x02); out(VGA_CRTC_DATA, 0x50); // H. Blank Start = 80
    out(VGA_CRTC_INDEX, 0x03); out(VGA_CRTC_DATA, 0x82); // H. Blank End = 130 (bit7=1)
    out(VGA_CRTC_INDEX, 0x04); out(VGA_CRTC_DATA, 0x54); // H. Retrace Start = 84
    out(VGA_CRTC_INDEX, 0x05); out(VGA_CRTC_DATA, 0x80); // H. Retrace End = 128 (bit8=1)
    
    // CRTC – vertical timings
    out(VGA_CRTC_INDEX, 0x06); out(VGA_CRTC_DATA, 0x0B); // Vertical Total = 11
    out(VGA_CRTC_INDEX, 0x07); out(VGA_CRTC_DATA, 0x3E); // Overflow = 0x3E (V total bits)
    out(VGA_CRTC_INDEX, 0x08); out(VGA_CRTC_DATA, 0x00); // Preset Row Scan = 0
    out(VGA_CRTC_INDEX, 0x09); out(VGA_CRTC_DATA, 0x40); // Max Scan Line = 64
    out(VGA_CRTC_INDEX, 0x10); out(VGA_CRTC_DATA, 0xEA); // Vertical Retrace Start = 234
    out(VGA_CRTC_INDEX, 0x11); out(VGA_CRTC_DATA, 0x8C); // Vertical Retrace End = 140
    out(VGA_CRTC_INDEX, 0x12); out(VGA_CRTC_DATA, 0xDF); // Vertical Display End = 223
    out(VGA_CRTC_INDEX, 0x13); out(VGA_CRTC_DATA, 0x28); // Logical Width = 40*8=320 bytes? (unused here)
    out(VGA_CRTC_INDEX, 0x14); out(VGA_CRTC_DATA, 0x00); // Underline = 0
    out(VGA_CRTC_INDEX, 0x15); out(VGA_CRTC_DATA, 0xE7); // Vertical Blank Start = 231
    out(VGA_CRTC_INDEX, 0x16); out(VGA_CRTC_DATA, 0x04); // Vertical Blank End = 4
    out(VGA_CRTC_INDEX, 0x17); out(VGA_CRTC_DATA, 0xE3); // Mode Control = 0xE3
    // (Values above taken from standard VGA mode 12h timing

    // Attribute Controller (0x3C0): reset flip-flop
    (void)in(VGA_INPUT_STATUS);
    out(VGA_ACT_INDEX, 0x10); out(VGA_ACT_INDEX, 0x01);  // Attribute Mode Control = 1
    out(VGA_ACT_INDEX, 0x11); out(VGA_ACT_INDEX, 0x00);  // Overscan = 0
    out(VGA_ACT_INDEX, 0x12); out(VGA_ACT_INDEX, 0x0F);  // Color Plane Enable = 0x0F&#8203
    out(VGA_ACT_INDEX, 0x13); out(VGA_ACT_INDEX, 0x00);  // Horizontal Pan = 0
    out(VGA_ACT_INDEX, 0x14); out(VGA_ACT_INDEX, 0x00);  // Color Select = 0
    out(VGA_ACT_INDEX, 0x20);                            // Finalize: enable video output

    vga_clear(0);
}

void vga_draw_pixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
    int byteIndex = y * VGA_PITCH + (x >> 3);
    uint8_t mask = 1 << (7 - (x & 7));
    uint8_t *vga = (uint8_t*)VGA_MEMORY;
    
    for (int plane = 0; plane < 4; plane++) {
        // Pilih Plane-nya
        out(VGA_SEQ_INDEX, 0x02);
        out(VGA_SEQ_DATA, (1 << plane));
        
        // Read map Select
        out(VGA_GC_INDEX, 0x04);
        out(VGA_GC_DATA, plane);
        
        // Ambil data dari planhe nya
        uint8_t current = vga[byteIndex];
        
        // Ganti bit warnanya
        if (color & (1 << plane)) {
            vga[byteIndex] = current | mask;
        } else {
            vga[byteIndex] = current & ~mask;
        }
    }
}

void vga_clear(uint8_t color) {
    uint8_t *vga = (uint8_t*)VGA_MEMORY;
    for (int plane = 0; plane < 4; plane++) {
        out(VGA_SEQ_INDEX, 0x02);
        out(VGA_SEQ_DATA, (1 << plane));    
        if (color & (1 << plane)) {
            for (int i = 0; i < VGA_PLANE_SIZE; i++) vga[i] = 0xFF;
        } else {
            for (int i = 0; i < VGA_PLANE_SIZE; i++) vga[i] = 0x00;
        }
    }
}

void vga_draw_cursor(int cx, int cy, uint8_t color) {
    int px = cx * 8;
    int py = cy * 8;
    for (int dy = 7; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            vga_draw_pixel(px + dx, py + dy, color);
        }
    }
}

void vga_clear_cursor(int cx, int cy) {
    int px = cx * 8;
    int py = cy * 8;
    for (int dy = 7; dy < 16; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            vga_draw_pixel(px + dx, py + dy, 0x00);
        }
    }
}

void vga_draw_char(int x, int y, char c, uint8_t color) {
    x = x * 8;
    y = y * 8;
    if (c < 32 || c > 126) return;
    // Bersihin dulu 8x8 pixel yang baka dipake
    for(int dy=0; dy<8; dy++) {
        for(int dx=0; dx<8; dx++) {
            vga_draw_pixel(x+dx, y+dy, 0x00);
        }
    }

    const uint8_t *glyph = font8x8[c - 32];
    for(int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for(int col = 0; col < 8; col++) {
            // The font has LSB as the leftmost pixel (bit 0 = leftmost)
            if (bits & (1 << col)) {  // Changed back to original
                vga_draw_pixel(x + col, y + row, color);
            }
        }
    }
}