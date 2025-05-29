#include "commands/mkdir.h"
#include "filesystem/ext2.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void mkdir(uint32_t current_inode, int argc, char *argv[]) {
    if(current_inode == 2) {
        const char *msg = "mkdir: cannot add new folder in /bin directory\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0xC);
        return;
    }

    if (argc >= 2) {

        const char *dirname = (argc >= 2) ? argv[1] : NULL;

        if (!dirname) {
            const char *msg = "Usage: mkdir <dir>\n\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            return;
        }

        int32_t ret_code = 0;

        struct EXT2DriverRequest request = {
            .buf = NULL,
            .name = (char *)dirname,
            .parent_inode = current_inode,
            .buffer_size = 0,
            .name_len = strlen(dirname),
            .is_directory = true,
        };

        syscall(2, (uint32_t)&request, (uint32_t)&ret_code, 0);

        if (ret_code == -1) {
            const char *msg = "mkdir failed\n\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }
        else if (ret_code == 1) {
            const char *msg = "mkdir: directory already exists\n\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }
        else {
            const char *msg = "mkdir success\n\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }
    }
    else {
        const char *msg = "Usage: mkdir <dir>\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
    }
}