#include "../header/drivers/console.h"

int cursor_row = 0, cursor_col = 0;
static const uint8_t text_color = 0x0F; 

// Initialize graphics console: set mode and clear screen.
void console_init() {
    vga_init();
    keyboard_state_activate();
    vga_draw_cursor(cursor_row, cursor_col, text_color);
}

// Draw one character at the given console position (row, col).
void console_putc(char c) {
    if (c == '\n') {
        // Newline: move to next line
        cursor_col = 0;
        if (++cursor_row >= MAX_ROWS) {
            cursor_row = MAX_ROWS-1;
            // TODO: implement scroll-up here if desired
        }
        return;
    }
    if (c == '\b') {
        // Backspace: move back one and clear
        if (cursor_col > 0) {
            cursor_col--;
            // Erase character by drawing a blank (we draw space glyph or clear pixels)
            for(int dy=0; dy<8; dy++) {
                for(int dx=0; dx<8; dx++) {
                    vga_draw_pixel(cursor_col*8+dx, cursor_row*8+dy, 0x00);
                }
            }
        }
        return;
    }
    // Printable char: draw it and advance cursor
    int x = cursor_col * 8;
    int y = cursor_row * 8;
    vga_draw_char(x, y, c, text_color);
    if (++cursor_col >= MAX_COLS) {
        cursor_col = 0;
        if (++cursor_row >= MAX_ROWS) {
            cursor_row = MAX_ROWS-1;
            // TODO: scroll if needed
        }
    }
}

// Update cursor (underline) visibility. Should be called after cursor move.
void update_cursor(void) {
    vga_draw_cursor(cursor_col, cursor_row, text_color);
}

// Main loop to poll keyboard and update console. Call this repeatedly.
void console_poll_input(void) {
    // Erase the cursor shape at old position
    vga_clear_cursor(cursor_col, cursor_row);

    // Handle any special key
    uint8_t sk = get_special_key();
    if (sk == KEY_UP && cursor_row > 0) cursor_row--;
    if (sk == KEY_DOWN && cursor_row < MAX_ROWS-1) cursor_row++;
    if (sk == KEY_LEFT && cursor_col > 0) cursor_col--;
    if (sk == KEY_RIGHT && cursor_col < MAX_COLS-1) cursor_col++;

    // Handle any character key
    char c;
    get_keyboard_buffer(&c);
    if (c) {
        console_putc(c);
    }

    // Draw the cursor at (possibly) new position
    update_cursor();
}