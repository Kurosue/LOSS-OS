#include "commands/touch.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void touch(uint32_t current_inode, int argc, char *argv[]) {
    if(current_inode == 2) {
        const char *msg = "touch: cannot add new file in /bin directory\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
        return;
    }

    if (argc < 2) {
        const char *msg = "Usage: touch <filename>\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xF);
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

    uint32_t ret_code;
    syscall(2, (uint32_t)&request, (uint32_t)&ret_code, 0);

    if (ret_code == 1) {
        const char *msg = "File already exists.\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    }
    else if (ret_code == 2) {
        const char *msg = "Parent directory not found.\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    }
    else if (ret_code == 3) {
        const char *msg = "Failed to create file.\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    }
}