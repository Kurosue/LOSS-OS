#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "drivers/framebuffer.h"

struct Framebuffer framebuffer_state = {
    .row = 1,
    .col = 0,
    .start_row = 0,
    .start_col = 0
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