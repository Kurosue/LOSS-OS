#include "commands/touch.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void touch(uint32_t current_inode, int argc, char *argv[]) {
    if (argc < 2) {
        syscall(6, (uint32_t)"Usage: touch <filename>\n", 24, 0xF);
        return;
    }

    struct EXT2DriverRequest request = {
        .buf = NULL,
        .name = argv[1],
        .parent_inode = current_inode,
        .buffer_size = 0,
        .name_len = strlen(argv[1]),
        .is_directory = false
    };
    uint32_t retcode;
    syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);
    if(retcode == 1) {
        syscall(6, (uint32_t)"File already exists.\n", 21, 0xC);
    } else if(retcode == 2) {
        syscall(6, (uint32_t)"Parent directory not found.\n", 29, 0xC);
    } else if(retcode == 3) {
        syscall(6, (uint32_t)"Failed to create file.\n", 24, 0xC);
    }
}