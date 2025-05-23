#include <stdint.h>
#include "filesystem/ext2.h"
#include "lib/string.h"
// #include "commands/commands.h"

#define BLOCK_COUNT 16

// Init buffer untuk menyimpan history sama current buffer comman
char command[10][100];
uint32_t currentInode = 1; // Init root cihuy

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

void processCommand(char *command)
{
    char *argv[10];
    int argc = 0;
    char *p = command;
    /** Cek dan Panggil possible command kita, kalau gabisa error message 
     * 1. cd
     * 2. ls
     * 3. mkdir
     * 4. cat
     * 5. cp
     * 6. rm
     * 7. mv
     * 8. find
     */
    // Split string comman dan args nya
    // buang leading spaces dan newline
    while (*p == ' ' || *p == '\n') p++;

    while (*p && argc < 10) {
        argv[argc++] = p;
        // cari akhir token: space atau newline
        while (*p && *p != ' ' && *p != '\n') p++;

        if (*p == ' ') {
            *p = '\0';
            p++;
            // skip multiple spaces
            while (*p == ' ') p++;
        }
        else if (*p == '\n') {
            *p = '\0';
            break;       // langsung keluar, newline tidak jadi argv
        }
    }

    if (argc == 0) {
        return; // Kosongan alias enter doang
    }

    // Pengecekan command
    char *cmd = argv[0];
    if (memcmp(cmd, "cd", 2) == 0 && strlen(cmd) == 2) {
        if (argc >= 2) {
            // TODO: panggil fungsi_cd(argv[1]);
        } else {
            const char *msg = "Usage: cd <dir>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }

    } else if (memcmp(cmd, "ls", 2) == 0 && strlen(cmd) == 2) {
        // Construct EXT2DriverReq buat currInode
        int32_t retCode = 0;
        unsigned char data_buffer[BLOCK_SIZE * 16];
        struct EXT2DriverRequest reqDir = {
            .buf                   = &data_buffer,
            .name                  = ".",
            .parent_inode          = 1,
            .buffer_size           = BLOCK_SIZE * 16,
            .name_len              = 1,
        };
        syscall(1, (uint32_t) &reqDir,(uint32_t) &retCode, 0);
        if (retCode == 0) {
            uint32_t current_offset = 0;
            struct EXT2DirectoryEntry *entry;

            while (current_offset < reqDir.buffer_size) {
                entry = (struct EXT2DirectoryEntry *)(data_buffer + current_offset);
                if (entry->inode == 0) {
                    if (entry->rec_len == 0) {
                        break;
                    }
                    current_offset += entry->rec_len;
                    if (current_offset >= reqDir.buffer_size && entry->rec_len < sizeof(struct EXT2DirectoryEntry)) break;
                    continue;
                }

                if (current_offset + entry->rec_len > reqDir.buffer_size || entry->rec_len < (sizeof(struct EXT2DirectoryEntry) + entry->name_len)) {
                    break;
                }
                char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);

                syscall(6, (uint32_t)entry_name, entry->name_len, 0xF);

                if (entry->file_type == EXT2_FT_DIR) {
                    const char *dir_suffix = "/ "; 
                    syscall(6, (uint32_t)dir_suffix, strlen(dir_suffix), 0xA); 
                } else {
                    const char *file_suffix = "  "; 
                    syscall(6, (uint32_t)file_suffix, strlen(file_suffix), 0xF);
                }
                current_offset += entry->rec_len; 
            }
            const char *final_newline = "\n";
            syscall(6, (uint32_t)final_newline, strlen(final_newline), 0xF);

        } else {
            const char *errMsg = "ls: cannot access directory\n";
            syscall(6, (uint32_t)errMsg, strlen(errMsg), 0x7);
        }
    } else if (memcmp(cmd, "mkdir", 5) == 0 && strlen(cmd) == 5) {
        if (argc >= 2) {
            int32_t retcode = 0;

            struct EXT2DriverRequest request = {
                .buf                   = NULL,
                .name                  = argv[1],
                .parent_inode          = currentInode,
                .buffer_size           = 0,
                .name_len              = strlen(argv[1]),
                .is_directory          = true,
            };

            syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);
            
            if (retcode == -1)
            {
                const char *msg = "mkdir failed\n";
                syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            }
        } else {
            const char *msg = "Usage: mkdir <dir>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }

    } else if (memcmp(cmd, "cat", 3) == 0 && strlen(cmd) == 3) {
        if (argc >= 2) {
            struct BlockBuffer bf;
            int32_t retcode = 0;

            struct EXT2DriverRequest request = {
                .buf                   = &bf,
                .name                  = argv[1],
                .parent_inode          = currentInode,
                .buffer_size           = BLOCK_SIZE * 16,
                .name_len              = strlen(argv[1]),
            };

            syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);

            if (retcode == 0)
            {
                const char *msg = (char *) bf.buf;
                syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            }
            else
            {
                const char *msg = "cat failed\n";
                syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            }
        } else {
            const char *msg = "Usage: cat <file>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }

    } else if (memcmp(cmd, "cp", 2) == 0 && strlen(cmd) == 2) {
        if (argc >= 3) {
            // TODO: panggil fungsi_cp(argv[1], argv[2]);
        } else {
            const char *msg = "Usage: cp <src> <dst>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }

    } else if (memcmp(cmd, "rm", 2) == 0 && strlen(cmd) == 2) {
        if (argc >= 2) {
            // TODO: panggil fungsi_rm(argv[1]);
        } else {
            const char *msg = "Usage: rm <file>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }

    } else if (memcmp(cmd, "mv", 2) == 0 && strlen(cmd) == 2) {
        if (argc >= 3) {
            // TODO: panggil fungsi_mv(argv[1], argv[2]);
        } else {
            const char *msg = "Usage: mv <src> <dst>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }

    } else if (memcmp(cmd, "find", 4) == 0 && strlen(cmd) == 4) {
        if (argc >= 2) {
            // TODO: panggil fungsi_find(argv[1]);
        } else {
            const char *msg = "Usage: find <name>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        }

    } else {
        const char *msg = "Error: Command ";
        const char *nmsg = " is not available !!\n";
        syscall(6, (uint32_t) msg, strlen(msg), 0x4);
        syscall(6, (uint32_t) cmd, strlen(cmd), 0xF);
        syscall(6, (uint32_t) nmsg, strlen(nmsg), 0x4);
    }
}

void terminal()
{
    // Pemanis sahaja biar keren
    char *user = "kiwz";
    char *OSname = "@LOSS-2025:";
    syscall(6, (uint32_t) user, strlen(user), 0xA);
    syscall(6, (uint32_t) OSname, strlen(OSname), 0xF);

    // Input keyboard
    char buf = 0;
    int indexCommand = 0;
    while(buf != '\n')
    {
        syscall(4, (uint32_t) &buf, 0, 0);
        if(buf == '\b')
        {
            if(indexCommand > 0)
            {
                // Hapus dari buffer index kurangin
                syscall(5, (uint32_t) buf, 0xF, 0);
                command[0][indexCommand] = 0;
                indexCommand--;
            }
        } else if(buf){
            syscall(5, (uint32_t) buf, 0xF, 0); 
            command[0][indexCommand] = buf;
            indexCommand++;
        }
    }
    syscall(5, (uint32_t) '\n', 0xF, 0);
    processCommand(command[0]); 
    for(int i = 0; i < 100; i++)
    {
        command[0][i] = 0;
    }
}

int main(void) {
    struct BlockBuffer      bl[2]   = {0};
    struct EXT2DriverRequest request = {
        .buf                   = &bl,
        .name                  = "shell",
        .parent_inode          = 1,
        .buffer_size           = BLOCK_SIZE * BLOCK_COUNT,
        .name_len = 5,
    };
    int32_t retcode;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    syscall(7, 0, 0, 0);
    while (true)
    {
        /* code */
        terminal();
    }
    


    return 0;
}
