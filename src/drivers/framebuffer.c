#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "drivers/framebuffer.h"

struct Framebuffer framebuffer_state = {
    .col = 0,
    .row = 0
};

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t pos = r * 80 + c;
	out(CURSOR_PORT_CMD, 0x0F);
	out(CURSOR_PORT_DATA, (uint8_t) (pos & 0xFF));
	out(CURSOR_PORT_CMD, 0x0E);
	out(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));

}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    FRAMEBUFFER_MEMORY_OFFSET[(row * 80 + col) * 2] = c;
    FRAMEBUFFER_MEMORY_OFFSET[(row * 80 + col) * 2 + 1] = fg | bg;
}

void framebuffer_clear(void) {
    for (int i = 0; i < 80 * 25; i++){
        FRAMEBUFFER_MEMORY_OFFSET[i*2] = 0x00;
        FRAMEBUFFER_MEMORY_OFFSET[i*2+1] = 0x07;
    }
}

void putchar(char c, uint32_t color) {
    if (c) {
        if (c == '\b') {
            if (framebuffer_state.col > 0) {
                framebuffer_state.col--;
            } 
            else if (framebuffer_state.row > 0) {
                framebuffer_state.row--;
                framebuffer_state.col = FRAMEBUFFER_WIDTH - 1;
            }

            // Clear the character at current position
            framebuffer_write(framebuffer_state.row, framebuffer_state.col, ' ', color, 0);
        } 

        // Handle newline
        else if (c == '\n') {
            framebuffer_state.row++;
            framebuffer_state.col = 0;
        } 

        // Handle regular characters
        else {
            framebuffer_write(framebuffer_state.row, framebuffer_state.col, c, color, 0);
            framebuffer_state.col++;
            if (framebuffer_state.col >= FRAMEBUFFER_WIDTH) {
                framebuffer_state.row++;
                framebuffer_state.col = 0;
            }
        }
        
        framebuffer_set_cursor(framebuffer_state.row, framebuffer_state.col);
    }
    
    uint8_t special_key = get_special_key();
    if (special_key != KEY_NONE) {
        switch (special_key) {
            case KEY_UP:
                if (framebuffer_state.row > 0) framebuffer_state.row--;
                break;
                
            case KEY_DOWN:
                if (framebuffer_state.row < FRAMEBUFFER_HEIGHT - 1) framebuffer_state.row++;
                break;
                
            case KEY_LEFT:
                if (framebuffer_state.col > 0) {
                    framebuffer_state.col--;
                } else if (framebuffer_state.row > 0) {
                    framebuffer_state.row--;
                    framebuffer_state.col = FRAMEBUFFER_WIDTH - 1;
                }
                break;
                
            case KEY_RIGHT:
                if (framebuffer_state.col < FRAMEBUFFER_WIDTH - 1) {
                    framebuffer_state.col++;
                } else if (framebuffer_state.row < FRAMEBUFFER_HEIGHT - 1) {
                    framebuffer_state.row++;
                    framebuffer_state.col = 0;
                }
                break;
        }
        framebuffer_set_cursor(framebuffer_state.row, framebuffer_state.col);
    }
}

void puts(char* string, uint32_t count, uint32_t color) {
    uint32_t i;
    if (framebuffer_state.row >= FRAMEBUFFER_HEIGHT - 1) scrollDown();
    for (i = 0; i < count; i++) {
        putchar(string[i], color);
    }
}

void scrollDown() {
    memcpy(FRAMEBUFFER_MEMORY_OFFSET, FRAMEBUFFER_MEMORY_OFFSET + FRAMEBUFFER_WIDTH * 2, FRAMEBUFFER_WIDTH * 2 * (FRAMEBUFFER_HEIGHT - 1));
    framebuffer_state.row--;
    framebuffer_state.col = 0;
    for (int i = 0; i < FRAMEBUFFER_WIDTH; i++) {
        framebuffer_write(framebuffer_state.row, i, ' ', 0xF, 0);
    }
    framebuffer_set_cursor(framebuffer_state.row, framebuffer_state.col);
}
