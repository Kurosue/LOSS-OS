#include "drivers/console.h"

void console_init() {
    vga_init();
    keyboard_state_activate();
}

void update_cursor(uint8_t text_color) {
    vga_draw_cursor(framebuffer_state.col, framebuffer_state.row, text_color);
}

void putchar(char c, uint8_t text_color) {
    if (c) {
        if (c == '\b') {
            vga_clear_cursor(framebuffer_state.col, framebuffer_state.row);
            if (framebuffer_state.col > 0) {
                framebuffer_state.col--;
            } 
            else if (framebuffer_state.row > 0) {
                framebuffer_state.row--;
                framebuffer_state.col = MAX_COLS - 1;
            }

            // Draw char " " ae
            vga_draw_char(framebuffer_state.col, framebuffer_state.row, ' ', text_color);
        } 
        else if (c == '\n') {
            vga_clear_cursor(framebuffer_state.col, framebuffer_state.row);
            framebuffer_state.row++;
            framebuffer_state.col = 0;
            if (framebuffer_state.row >= MAX_ROWS) {
                framebuffer_state.row = MAX_ROWS - 1;
            }
        } 
        else {
            vga_draw_char(framebuffer_state.col, framebuffer_state.row, c, text_color);
            framebuffer_state.col++;
            if (framebuffer_state.col >= MAX_COLS) {
                framebuffer_state.row++;
                framebuffer_state.col = 0;
                if (framebuffer_state.row >= MAX_ROWS) {
                    framebuffer_state.row = MAX_ROWS - 1;
                    // TODO: implement scroll-up here if desired
                }
            }
        }
        update_cursor(text_color);
    }
}

void puts(char* string, uint32_t count, uint8_t text_color) {
    uint32_t i;
    if (string == NULL) return;
    
    for (i = 0; i < count && string[i] != '\0'; i++) {
        putchar(string[i], text_color);
    }
}