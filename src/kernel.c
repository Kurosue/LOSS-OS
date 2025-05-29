#include <stdint.h>
#include "cpu/gdt.h"
#include "kernel-entrypoint.h"
#include "drivers/framebuffer.h"
#include "cpu/interrupt.h"
#include "cpu/idt.h"
#include "drivers/keyboard.h"
#include "drivers/console.h"
#include "drivers/disk.h"
#include "filesystem/ext2.h"
#include "memory/paging.h"
#include "process/process.h"
#include "process/scheduler.h"
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
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0);

    // Write shell into memory
    struct EXT2DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .parent_inode          = 2,
        .buffer_size           = 0x100000,
        .name_len              = 5
    };
    read(request);

    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();
    process_create_user_process(request);
    scheduler_init();
    scheduler_switch_to_next_process();
}
