#include "commands/cat.h"
#include "filesystem/ext2.h"
#include "drivers/disk.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void cat(uint32_t current_inode, int argc, char *argv[]) {

    if (argc >= 2) {
        const char *filename = argv[1];
        if (!filename) {
            const char *msg = "Usage: cat <file>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            return;
        }
        
        struct BlockBuffer bf;
        int32_t retcode = 0;

        struct EXT2DriverRequest request = {
            .buf                   = &bf,
            .name                  = (char *)filename,
            .parent_inode          = current_inode,
            .buffer_size           = BLOCK_SIZE * 16,
            .name_len              = strlen(filename),
        };

        syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);

        if (retcode == 0) {
            const char *msg = (char *) bf.buf;
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        } else {
            const char *msg = "cat failed\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }
    }
    else
    {
        const char *msg = "Usage: cat <file>\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
    }
}