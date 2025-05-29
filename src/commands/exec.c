#include "commands/exec.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void exec(int argc, char *argv[]) {

    if (argc < 2) {
        const char *msg = "Usage: exec <program> [args...]\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        return;
    }

    struct EXT2DriverRequest req = {
        .buf = (uint8_t *)0, // Virtual memory address di 0
        .name = argv[1],
        .parent_inode = 2, // Asumsi folder bin ada di inode 2
        .buffer_size = 0x100000,
        .name_len = strlen(argv[1]),
        .is_directory = false};

    int32_t ret_code;

    syscall(9, (uint32_t)&req, (uint32_t)&ret_code, 0);
    if (ret_code == 1) {
        const char *msg = "exec: max process\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    }
    else if (ret_code == 2) {
        const char *msg = "exec: invalid entrypoint\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    }
    else if (ret_code == 3) {
        const char *msg = "exec: not enough memory\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    }
    else if (ret_code == 4) {
        const char *msg = "exec: read failure\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    }
}