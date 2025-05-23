#include "commands/echo.h"


extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void echo(uint32_t currentInode, int argc, char *argv[]) {
    if (argc == 2) {
        syscall(6, (uint32_t)argv[1], strlen(argv[1]), 0xF);
        syscall(5, (uint32_t)'\n', 0xF, 0);
    } else if (argc >= 4){
        if(memcmp(argv[2], ">", 1) == 0 && strlen(argv[3]) > 0) {
            struct EXT2DriverRequest request = {
                .buf                   = argv[1],
                .name                  = argv[3],
                .parent_inode          = currentInode,
                .buffer_size           = strlen(argv[1]),
                .name_len              = strlen(argv[3]),
            };
            syscall(2, (uint32_t) &request, 0, 0);
            syscall(5, (uint32_t)'\n', 0xF, 0);
        } else {
            const char *msg = "Usage: echo <text> [> <file_name>]\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }
    } else {
        const char *msg = "Usage: echo <text> [> <file_name>]\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
    }
}