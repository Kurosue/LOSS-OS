#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/drivers/framebuffer.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/idt.h"
#include "header/drivers/keyboard.h"
#include "header/drivers/disk.h"

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    // test disk driver   
    struct BlockBuffer b;
    for (int i = 0; i < 512; i++) b.buf[i] = i % 16;
    write_blocks(&b, 17, 1);

    int row = 0, col = 0;
    keyboard_state_activate();
    
    while (true) {

        // Check for regular character input
        char c;
        get_keyboard_buffer(&c);

        if (c) {

            // Handle special keys like backspace
            if (c == '\b') {

                // Handle backspace - move cursor back and clear character
                if (col > 0) {
                    col--;
                } 
                else if (row > 0) {
                    row--;
                    col = FRAMEBUFFER_WIDTH - 1;
                }

                // Clear the character at current position
                framebuffer_write(row, col, ' ', 0xF, 0);
            } 

            // Handle newline
            else if (c == '\n') {
                row++;
                col = 0;
            } 

            // Handle regular characters
            else {
                framebuffer_write(row, col, c, 0xF, 0);
                col++;
                if (col >= FRAMEBUFFER_WIDTH) {
                    row++;
                    col = 0;
                }
            }
            
            framebuffer_set_cursor(row, col);
        }
        
        // Check for special keys (arrow keys, etc.)
        uint8_t special_key = get_special_key();
        if (special_key != KEY_NONE) {
            switch (special_key) {
                case KEY_UP:
                    if (row > 0) row--;
                    break;
                    
                case KEY_DOWN:
                    if (row < FRAMEBUFFER_HEIGHT - 1) row++;
                    break;
                    
                case KEY_LEFT:
                    if (col > 0) {
                        col--;
                    } else if (row > 0) {
                        row--;
                        col = FRAMEBUFFER_WIDTH - 1;
                    }
                    break;
                    
                case KEY_RIGHT:
                    if (col < FRAMEBUFFER_WIDTH - 1) {
                        col++;
                    } else if (row < FRAMEBUFFER_HEIGHT - 1) {
                        row++;
                        col = 0;
                    }
                    break;
            }
            framebuffer_set_cursor(row, col);
        }
    }
}