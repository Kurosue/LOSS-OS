#include "commands/rm.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);


void rm(uint32_t current_inode, int argc,char *argv[]) {
    if (argc < 2) {
        const char *msg = "Usage: rm <file>\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        return;
    }

    const char *filename = argv[1];
    int32_t retcode = 0;

    struct EXT2DriverRequest request = {
        .buf                   = NULL,
        .name                  = (char *)filename,
        .parent_inode          = current_inode,
        .buffer_size           = 0,
        .name_len              = strlen(filename),
        .is_directory          = false,
    };

    syscall(3, (uint32_t) &request, (uint32_t) &retcode, 0);

    if (retcode != 0)
    {
       struct EXT2DriverRequest request = {
            .buf                   = NULL,
            .name                  = (char *)filename,
            .parent_inode          = current_inode,
            .buffer_size           = 0,
            .name_len              = strlen(filename),
            .is_directory          = true,
        };
        syscall(3, (uint32_t) &request, (uint32_t) &retcode, 0);
        if (retcode == 0) {
            const char *msg = "rm success\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        } else {
            const char *msg = "rm failed\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }
    }
}