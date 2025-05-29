#include "commands/find.h"
#include "filesystem/ext2.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void find_recursive(char *target, uint32_t parent_inode, char *currentPath) {

    // here is where the fun begins, no ai involved btw, written on nvim btw, i'm single btw
    char data_buffer[BLOCK_SIZE * 16] = {};
    struct EXT2DriverRequest reqDir = {
        .buf = &data_buffer,
        .name = ".",
        .parent_inode = parent_inode,
        .buffer_size = BLOCK_SIZE * 16,
        .name_len = 1,
    };

    int32_t ret_code;
    syscall(1, (uint32_t)&reqDir, (uint32_t)&ret_code, 0);
    const char *final_newline = "\n\n";

    if (ret_code == 0) {
        uint32_t current_offset = 0;
        struct EXT2DirectoryEntry *entry;

        while (current_offset < reqDir.buffer_size) {
            entry = (struct EXT2DirectoryEntry *)(data_buffer + current_offset);
            if (entry->inode == 0) {
                if (entry->rec_len == 0) 
                    break;

                current_offset += entry->rec_len;
                if (current_offset >= reqDir.buffer_size && entry->rec_len < sizeof(struct EXT2DirectoryEntry))
                    break;

                continue;
            }

            if (current_offset + entry->rec_len > reqDir.buffer_size || entry->rec_len < (sizeof(struct EXT2DirectoryEntry) + entry->name_len))
                break;

            char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);

            if (entry->file_type == EXT2_FT_DIR && 
                memcmp(entry_name, ".", 1) != 0 && 
                memcmp(entry_name, "..", 2) != 0) {

                char newPath[strlen(currentPath) + entry->name_len + 5];
                newPath[0] = '\0';

                if (append(newPath, currentPath, sizeof(newPath)) == 0 && 
                    append(newPath, entry_name, sizeof(newPath)) == 0) {

                    append(newPath, "/", sizeof(newPath));

                    if (memcmp(target, entry_name, entry->name_len) == 0) {
                        syscall(6, (uint32_t)newPath, strlen(newPath), 0xF);
                        syscall(6, (uint32_t)final_newline, strlen(final_newline), 0xF);

                        // still calls, let's say i have folder /acep/acep/
                        find_recursive(target, entry->inode, newPath);
                    }
                    else {
                        find_recursive(target, entry->inode, newPath);
                    }
                }
                else {
                    const char *error_message = "Error when appending folder name to path\n\n";
                    syscall(6, (uint32_t)error_message, strlen(error_message), 0xC);
                }
            }
            else if (entry->file_type == EXT2_FT_REG_FILE) {

                char newPath[strlen(currentPath) + entry->name_len + 5];
                newPath[0] = '\0';

                if (append(newPath, currentPath, sizeof(newPath)) == 0 && 
                    append(newPath, entry_name, sizeof(newPath)) == 0) {
                        
                    if (memcmp(target, entry_name, entry->name_len) == 0) {
                        syscall(6, (uint32_t)newPath, strlen(newPath), 0xF);
                        syscall(6, (uint32_t)final_newline, strlen(final_newline), 0xF);
                    }
                }
                else {
                    const char *error_message = "Error when appending file name to path\n\n";
                    syscall(6, (uint32_t)error_message, strlen(error_message), 0xC);
                }
            }
            current_offset += entry->rec_len;
        }
    }
    else {
        const char *errMsg = "find: cannot access directory\n\n";
        syscall(6, (uint32_t)errMsg, strlen(errMsg), 0x7);
    }
}

void find(int argc, char *argv[]) {

    if (argc >= 2) {

        char *arg = argv[1];

        if ((memcmp(arg, "-h", 2) == 0 && strlen(arg) == 2) || 
            (memcmp(arg, "--help", 6) == 0 && strlen(arg) == 6)) {

            const char *msg = "Find any file/folder with name specified.\n";
            const char *msg2 = "\n\nUsage: find <name>\n\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0xF);
            syscall(6, (uint32_t)msg2, strlen(msg), 0xB);
        }
        else {
            find_recursive(arg, 1, "/");
        }
    }
    else {
        const char *msg = "Usage: find <name>\n\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
    }
}