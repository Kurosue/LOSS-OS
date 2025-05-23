#include <stdint.h>
#include "filesystem/ext2.h"
#include "lib/string.h"
// #include "commands/commands.h"

#define BLOCK_COUNT 16

// Function declarations
uint32_t get_parent_inode(uint32_t inode);
uint32_t find_child_inode(uint32_t parent_inode, const char *name, uint8_t name_len);
int split_path(const char *path, char components[][64], int max_components);
int strcmp(const char *s1, const char *s2);
void processCommand(char *command);
void terminal(void);

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

// Simple string comparison function
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Helper function to get parent inode of a directory using syscalls
uint32_t get_parent_inode(uint32_t inode) {
    if (inode == 1) return 1; // Root's parent is itself
    
    // Read the directory contents using syscall
    unsigned char data_buffer[BLOCK_SIZE * 16];
    struct EXT2DriverRequest reqDir = {
        .buf                   = &data_buffer,
        .name                  = ".",
        .parent_inode          = inode,
        .buffer_size           = BLOCK_SIZE * 16,
        .name_len              = 1,
    };
    
    int32_t retCode = 0;
    syscall(1, (uint32_t) &reqDir, (uint32_t) &retCode, 0); // read_directory syscall
    
    if (retCode != 0) return 1; // If error, return root
    
    // Find ".." entry (should be second entry)
    uint32_t offset = 0;
    
    // Skip first entry (".")
    struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(data_buffer + offset);
    if (entry->rec_len == 0) return 1;
    offset += entry->rec_len;
    
    // Get second entry ("..")
    if (offset < BLOCK_SIZE * 16) {
        entry = (struct EXT2DirectoryEntry *)(data_buffer + offset);
        if (entry->name_len == 2) {
            char *name = (char *)entry + sizeof(struct EXT2DirectoryEntry);
            if (name[0] == '.' && name[1] == '.') {
                return entry->inode;
            }
        }
    }
    
    return 1; // If error, return root
}

// Helper function to find child inode by name using syscalls
uint32_t find_child_inode(uint32_t parent_inode, const char *name, uint8_t name_len) {
    if (parent_inode == 0) return 0;
    
    // Read the directory contents using syscall
    unsigned char data_buffer[BLOCK_SIZE * 16];
    struct EXT2DriverRequest reqDir = {
        .buf                   = &data_buffer,
        .name                  = ".",
        .parent_inode          = parent_inode,
        .buffer_size           = BLOCK_SIZE * 16,
        .name_len              = 1,
    };
    
    int32_t retCode = 0;
    syscall(1, (uint32_t) &reqDir, (uint32_t) &retCode, 0); // read_directory syscall
    
    if (retCode != 0) return 0; // Error reading directory
    
    // Search through directory entries
    uint32_t offset = 0;
    while (offset < BLOCK_SIZE * 16) {
        struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(data_buffer + offset);
        
        if (entry->rec_len == 0 || offset + entry->rec_len > BLOCK_SIZE * 16) break;
        
        if (entry->inode != 0 && entry->file_type == EXT2_FT_DIR && 
            entry->name_len == name_len) {
            char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);
            
            // Compare names
            bool match = true;
            for (int i = 0; i < name_len; i++) {
                if (entry_name[i] != name[i]) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                return entry->inode;
            }
        }
        
        offset += entry->rec_len;
    }
    
    return 0; // Not found
}

// Helper function to split path into components
int split_path(const char *path, char components[][64], int max_components) {
    int count = 0;
    int i = 0;
    int comp_start = 0;
    int comp_len = 0;
    
    // Skip leading slashes
    while (path[i] == '/') i++;
    
    comp_start = i;
    
    while (path[i] && count < max_components) {
        if (path[i] == '/') {
            if (comp_len > 0) {
                // Copy component
                if (comp_len < 63) {
                    for (int j = 0; j < comp_len; j++) {
                        components[count][j] = path[comp_start + j];
                    }
                    components[count][comp_len] = '\0';
                    count++;
                }
                comp_len = 0;
            }
            // Skip multiple slashes
            while (path[i] == '/') i++;
            comp_start = i;
        } else {
            comp_len++;
            i++;
        }
    }
    
    // Handle last component
    if (comp_len > 0 && count < max_components) {
        if (comp_len < 63) {
            for (int j = 0; j < comp_len; j++) {
                components[count][j] = path[comp_start + j];
            }
            components[count][comp_len] = '\0';
            count++;
        }
    }
    
    return count;
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
            const char *path = argv[1];
            uint32_t target_inode = currentInode;
            
            // Handle absolute paths (starting with /)
            if (path[0] == '/') {
                target_inode = 1; // Start from root
            }
            
            // Split path into components
            char components[16][64]; // Support up to 16 path components
            int num_components = split_path(path, components, 16);
            
            // Navigate through each component
            bool success = true;
            for (int i = 0; i < num_components && success; i++) {
                if (strlen(components[i]) == 0) {
                    continue; // Skip empty components
                }
                
                if (strcmp(components[i], ".") == 0) {
                    // Stay in current directory
                    continue;
                } else if (strcmp(components[i], "..") == 0) {
                    // Go to parent directory
                    uint32_t parent = get_parent_inode(target_inode);
                    if (parent != 0) {
                        target_inode = parent;
                    }
                    // If we can't find parent or we're already at root, stay where we are
                } else {
                    // Find child directory
                    uint32_t child = find_child_inode(target_inode, components[i], strlen(components[i]));
                    if (child == 0) {
                        const char *msg1 = "cd: ";
                        const char *msg2 = ": No such directory\n";
                        syscall(6, (uint32_t)msg1, strlen(msg1), 0x4);
                        syscall(6, (uint32_t)components[i], strlen(components[i]), 0xF);
                        syscall(6, (uint32_t)msg2, strlen(msg2), 0x4);
                        success = false; // Failed to find directory
                    } else {
                        target_inode = child;
                    }
                }
            }
            
            // Update current inode if successful
            if (success) {
                currentInode = target_inode;
            }
        } else {
            // If no argument provided, go to root directory
            currentInode = 1;
        }
    } else if (memcmp(cmd, "ls", 2) == 0 && strlen(cmd) == 2) {
        // Debug: Print current inode
        const char *debug_msg = "Current inode: ";
        syscall(6, (uint32_t)debug_msg, strlen(debug_msg), 0x7);
        
        // Simple way to print the number (convert to string manually)
        uint32_t inode_num = currentInode;
        char inode_str[16];
        int inode_len = 0;
        if (inode_num == 0) {
            inode_str[0] = '0';
            inode_len = 1;
        } else {
            // Convert number to string (reverse order first)
            while (inode_num > 0) {
                inode_str[inode_len++] = '0' + (inode_num % 10);
                inode_num /= 10;
            }
            // Reverse the string
            for (int i = 0; i < inode_len / 2; i++) {
                char temp = inode_str[i];
                inode_str[i] = inode_str[inode_len - 1 - i];
                inode_str[inode_len - 1 - i] = temp;
            }
        }
        syscall(6, (uint32_t)inode_str, inode_len, 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        
        // Read the current directory contents using read_directory syscall
        int32_t retCode = 0;
        unsigned char data_buffer[BLOCK_SIZE * 16];
        
        struct EXT2DriverRequest reqDir = {
            .buf                   = &data_buffer,
            .name                  = ".",
            .parent_inode          = currentInode,
            .buffer_size           = BLOCK_SIZE * 16,
            .name_len              = 1,
        };
        
        syscall(1, (uint32_t) &reqDir, (uint32_t) &retCode, 0); // read_directory syscall
        
        const char *retcode_msg = "Return code: ";
        syscall(6, (uint32_t)retcode_msg, strlen(retcode_msg), 0x7);
        char ret_str[4];
        if (retCode == 0) ret_str[0] = '0';
        else if (retCode == 1) ret_str[0] = '1';
        else if (retCode == 2) ret_str[0] = '2';
        else ret_str[0] = '?';
        syscall(6, (uint32_t)ret_str, 1, 0xF);
        syscall(6, (uint32_t)newline, 1, 0xF);
        
        if (retCode == 0) {
            uint32_t current_offset = 0;
            struct EXT2DirectoryEntry *entry;

            while (current_offset < BLOCK_SIZE * 16) {
                entry = (struct EXT2DirectoryEntry *)(data_buffer + current_offset);
                
                // Check for end of entries
                if (entry->rec_len == 0 || current_offset + entry->rec_len > BLOCK_SIZE * 16) {
                    break;
                }
                
                // Skip empty entries
                if (entry->inode == 0) {
                    current_offset += entry->rec_len;
                    continue;
                }
                
                // Print ALL entries for debugging (including . and ..)
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
            
            syscall(6, (uint32_t)newline, 1, 0xF);

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
            else if (retcode == 1)
            {
                const char *msg = "mkdir: directory already exists\n";
                syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            }
            else
            {
                const char *msg = "mkdir success\n";
                syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            }
        } 
        else {
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
            struct BlockBuffer bf;
            int32_t retcode = 0;

            struct EXT2DriverRequest read_request = {
                .buf                   = &bf,
                .name                  = argv[1],
                .parent_inode          = currentInode,
                .buffer_size           = BLOCK_SIZE * 16,
                .name_len              = strlen(argv[1]),
            };

            syscall(0, (uint32_t) &read_request, (uint32_t) &retcode, 0);

            if (retcode != 0)
            {
                const char *msg = "failed to read from source file\n";
                syscall(6, (uint32_t)msg, strlen(msg), 0x7);
                return;
            }
            struct EXT2DriverRequest write_request = {
                .buf                   = &bf,
                .name                  = argv[2],
                .parent_inode          = currentInode,
                .buffer_size           = BLOCK_SIZE * 16,
                .name_len              = strlen(argv[2]),
            };
            syscall(2, (uint32_t) &write_request, (uint32_t) &retcode, 0x0);
            if (retcode != 0)
            {
                const char *msg = "failed to write to destination file\n";
                syscall(6, (uint32_t)msg, strlen(msg), 0x7);
                return;
            }
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