#include <stdint.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/drivers/framebuffer.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/idt.h"
#include "header/drivers/keyboard.h"
#include "header/drivers/console.h"
#include "header/drivers/disk.h"
#include "header/filesystem/ext2.h"
#include <string.h> // for memset, strlen

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    console_init();

    initialize_filesystem_ext2();

    struct BlockBuffer b;
    for (int i = 0; i < 512; i++) b.buf[i] = 'A' + (i % 26);  // fill with predictable data

    // ===== STEP 1: Create folder1 under root (inode 2) =====
    struct EXT2DriverRequest mkdir_req = {
        .name = "folder1",
        .name_len = 7,
        .parent_inode =1,
        .buf = NULL,
        .buffer_size = 0,
        .is_directory = true
    };

    int8_t mkdir_result = write(&mkdir_req);
    vga_draw_char(0, 0, (mkdir_result == 0) ? 'M' : 'F', 0xF); // M = mkdir ok

    // ===== STEP 2: Resolve inode of /folder1 =====
    uint32_t folder1_inode = get_inode_for_path("/folder1");
    vga_draw_char(0, 2, (folder1_inode != 0) ? 'I' : 'X', 0xF); // I = inode ok

    if (folder1_inode != 0) {
        // ===== STEP 3: Write tpazolite into /folder1 =====
        struct EXT2DriverRequest write_req = {
            .name = "tpazolite",
            .name_len = 9,
            .parent_inode = folder1_inode,
            .buf = &b,
            .buffer_size = 512,
            .is_directory = false
        };

        int8_t write_result = write(&write_req);
        vga_draw_char(0, 4, (write_result == 0) ? 'W' : 'X', 0xF); // W = write ok

        // ===== STEP 4: Clear buffer and read back =====
        memset(b.buf, 0, 512);
        int8_t read_result = read(write_req);
        vga_draw_char(0, 6, (read_result == 0) ? 'R' : 'X', 0xF); // R = read ok

        // ===== STEP 5: Validate content =====
        bool valid = true;
        for (int i = 0; i < 512; i++) {
            if (b.buf[i] != ('A' + (i % 26))) {
                valid = false;
                break;
            }
        }
        vga_draw_char(0, 8, (valid) ? 'V' : 'X', 0xF); // V = valid ok
    }

    while (true) {
        // infinite loop to prevent exit
    }
}
