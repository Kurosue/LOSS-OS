#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/drivers/framebuffer.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/idt.h"
#include "header/drivers/keyboard.h"
#include "header/drivers/disk.h"
#include "header/filesystem/ext2.h"

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

    //test ext2
    // I KNOW ITS A FCCKING AI SO DONT JUDGE ME
    initialize_filesystem_ext2();
    struct EXT2DriverRequest request;
    request.buf = &b;
    request.name = "test.txt";
    request.name_len = 9;
    request.parent_inode = 2;
    request.buffer_size = 512;
    request.is_directory = false;

    int8_t result = write(&request);
    if (result != 0) {
        framebuffer_write(0, 0, 'E', 0xF, 0);
        framebuffer_write(0, 1, 'R', 0xF, 0);
        framebuffer_write(0, 2, 'R', 0xF, 0);
        framebuffer_write(0, 3, 'O', 0xF, 0);
        framebuffer_write(0, 4, 'R', 0xF, 0);
    } else {
        framebuffer_write(0, 0, 'W', 0xF, 0);
        framebuffer_write(0, 1, 'R', 0xF, 0);
        framebuffer_write(0, 2, 'I', 0xF, 0);
        framebuffer_write(0, 3, 'T', 0xF, 0);
        framebuffer_write(0, 4, 'E', 0xF, 0);
    }
    // test read
    request.buf = &b;
    request.name = "test.txt";
    request.name_len = 9;
    request.parent_inode = 2;
    request.buffer_size = 512;
    request.is_directory = false;
    result = read(request);
    if (result != 0) {
        framebuffer_write(1, 0, 'E', 0xF, 0);
        framebuffer_write(1, 1, 'R', 0xF, 0);
        framebuffer_write(1, 2, 'R', 0xF, 0);
        framebuffer_write(1, 3, 'O', 0xF, 0);
        framebuffer_write(1, 4, 'R', 0xF, 0);
    } else {
        for (uint32_t i = 0; i < request.buffer_size; i++) {
            framebuffer_write(1, i % FRAMEBUFFER_WIDTH, b.buf[i], 0xF, 0);
        }
    }



    int row = 2, col = 0;
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