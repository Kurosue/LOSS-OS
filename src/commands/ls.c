#include "commands/ls.h"
#include "filesystem/ext2.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void ls(uint32_t current_inode) {

    int32_t ret_code = 0;
    unsigned char data_buffer[BLOCK_SIZE * 16];

    struct EXT2DriverRequest reqDir = {
        .buf = &data_buffer,
        .name = ".",
        .parent_inode = current_inode,
        .buffer_size = BLOCK_SIZE * 16,
        .name_len = 1,
    };

    syscall(1, (uint32_t)&reqDir, (uint32_t)&ret_code, 0);

    if (ret_code == 0) {
        uint32_t current_offset = 0;
        struct EXT2DirectoryEntry *entry;

        while (current_offset < BLOCK_SIZE * 16) {
            entry = (struct EXT2DirectoryEntry *)(data_buffer + current_offset);

            if (entry->rec_len == 0 || current_offset + entry->rec_len > BLOCK_SIZE * 16)
                break;

            if (entry->inode == 0) {
                current_offset += entry->rec_len;
                continue;
            }

            // Skip "." and ".." entries for cleaner output
            char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);
            if (entry->name_len == 1 && entry_name[0] == '.') {
                current_offset += entry->rec_len;
                continue;
            }

            if (entry->name_len == 2 && entry_name[0] == '.' && entry_name[1] == '.')
            {
                current_offset += entry->rec_len;
                continue;
            }

            syscall(6, (uint32_t)entry_name, entry->name_len, 0xF);

            if (entry->file_type == EXT2_FT_DIR) {
                const char *dir_suffix = "/ ";
                syscall(6, (uint32_t)dir_suffix, strlen(dir_suffix), 0xA);
            }
            else {
                const char *file_suffix = "  ";
                syscall(6, (uint32_t)file_suffix, strlen(file_suffix), 0xF);
            }

            current_offset += entry->rec_len;
        }

        const char *final_newline = "\n\n";
        syscall(6, (uint32_t)final_newline, strlen(final_newline), 0xF);
    }
    else {
        const char *errMsg = "ls: cannot access directory\n\n";
        syscall(6, (uint32_t)errMsg, strlen(errMsg), 0x7);
    }
}