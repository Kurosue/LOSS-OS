#include "commands/exec.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void exec(int argc, char *argv[]) {
    if (argc < 2) {
        const char *msg = "Usage: exec <program> [args...]\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        return;
    }

    char *program_name = argv[1];


    struct EXT2DriverRequest req = {
        .buf = (uint8_t*) 0, // Virtual memory address di 0
        .name = program_name,
        .parent_inode = 2, // Asumsi folder bin ada di inode 2
        .buffer_size = 0,
        .name_len = strlen(program_name),
        .is_directory = false
    };

    int32_t retCode;
    syscall(0, (uint32_t)&req, (uint32_t)&retCode, 0);

    if (retCode != 0) {
        const char *msg = "exec: program not found: ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)program_name, strlen(program_name), 0xF);
        syscall(5, (uint32_t)'\n', 0, 0);
        return;
    }

    syscall(9, (uint32_t) &req, (uint32_t) &retCode, 0);
    if(retCode == 1)
    {
        const char *msg = "exec: max process\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    } else if(retCode == 2) {
        const char *msg = "exec: invalid entrypoint\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    } else if(retCode == 3) {
        const char *msg = "exec: not enough memory\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    } else if(retCode == 4) {
        const char *msg = "exec: read failure\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
    }


}