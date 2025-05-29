#include "commands/cat.h"
#include "filesystem/ext2.h"
#include "drivers/disk.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void cat(uint32_t current_inode, int argc, char *argv[]) {
    // File di bin ga boleh di akses
    if(current_inode == 2) {
        const char *msg = "cat: cannot access /bin directory\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
        return;
    }

    if (argc >= 2) {

        const char *filename = argv[1];

        if (!filename) {
            const char *msg = "Usage: cat <file>\n\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            return;
        }

        struct BlockBuffer bf;
        int32_t ret_code = 0;

        struct EXT2DriverRequest request = {
            .buf = &bf,
            .name = (char *)filename,
            .parent_inode = current_inode,
            .buffer_size = 0x100000,
            .name_len = strlen(filename),
        };

        syscall(0, (uint32_t)&request, (uint32_t)&ret_code, 0);

        if (ret_code == 0) {
            const char *msg = (char *)bf.buf;
            syscall(6, (uint32_t)msg, strlen(msg), 0xF);
            syscall(6, (uint32_t)"\n\n", 2, 0);
        }
        else if (ret_code == 1) {
            const char *msg = "cat: read failure\n\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0xC);
        }
        else if (ret_code == 2) {
            const char *msg = "cat: file not found: ";
            syscall(6, (uint32_t)msg, strlen(msg), 0x4);
            syscall(6, (uint32_t)filename, strlen(filename), 0xF);
            syscall(6, (uint32_t)"\n\n", 2, 0);
        }
        else if (ret_code == -1) {
            const char *msg = "cat: invalid error\n\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0xC);
        }
    }
    else {
        const char *msg = "Usage: cat <file>\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xF);
    }
}