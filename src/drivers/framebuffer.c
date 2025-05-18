#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../header/drivers/framebuffer.h"
#include "../header/lib/string.h"
#include "../header/cpu/portio.h"

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
    if (c == '\0') {
        return;
    }
    if (c != '\n') {
        framebuffer_write(framebuffer_state.row, framebuffer_state.col, c, color, 0);
    }

    if (framebuffer_state.col == FRAMEBUFFER_WIDTH - 1 || c == '\n') {
        framebuffer_state.row++;
        framebuffer_state.col = 0;
        if (framebuffer_state.row == FRAMEBUFFER_HEIGHT) {
            scrollDown();
        }
        framebuffer_write(framebuffer_state.row, framebuffer_state.col, ' ', color, 0);
    } else {
        framebuffer_state.col++;
    }
    framebuffer_set_cursor(framebuffer_state.row, framebuffer_state.col);
}

void puts(uint32_t string, uint32_t count, uint32_t color) {
    char* temp = (char*) string;
    uint32_t i;
    if (framebuffer_state.row >= FRAMEBUFFER_HEIGHT - 1) scrollDown();
    for (i = 0; i < count; i++) {
        putchar(temp[i], color);
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
