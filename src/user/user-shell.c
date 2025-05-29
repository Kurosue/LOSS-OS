#include <stdint.h>
#include "filesystem/ext2.h"
#include "lib/string.h"
#include "commands/commands.h"

#define BLOCK_COUNT 16

void processCommand(char *command);
void terminal(void);
void update_path_display(void);

// Init buffer untuk menyimpan history sama current buffer command
char command[10][100];
uint32_t currentInode = 1; // Init root cihuy
char current_path[256] = "/"; // Track current path

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

// Import the helper functions from cd.c
uint32_t getParentInode(uint32_t inode);
uint32_t findChildInode(uint32_t parent_inode, const char *name, uint8_t name_len);

void update_path_display(void) {
    if (currentInode == 1) {
        strcpy(current_path, "/");
        return;
    }
    
    // Build path by traversing up to root and then reversing
    char path_parts[32][64];  // Store path components
    int depth = 0;
    uint32_t inode = currentInode;
    
    // Traverse up to root, collecting directory names
    while (inode != 1 && depth < 31) {
        uint32_t parent = getParentInode(inode);
        if (parent == inode) break; // Safety check
        
        // Read parent directory to find our name
        unsigned char data_buffer[BLOCK_SIZE * 16];
        struct EXT2DriverRequest reqDir = {
            .buf = &data_buffer,
            .name = ".",
            .parent_inode = parent,
            .buffer_size = BLOCK_SIZE * 16,
            .name_len = 1,
        };
        
        int32_t ret_code = 0;
        syscall(1, (uint32_t) &reqDir, (uint32_t) &ret_code, 0);
        
        if (ret_code != 0) break;
        
        // Find entry that matches our inode
        uint32_t offset = 0;
        bool found = false;
        while (offset < BLOCK_SIZE * 16 && !found) {
            struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(data_buffer + offset);
            if (entry->rec_len == 0) break;
            
            if (entry->inode == inode && entry->name_len > 0 && entry->name_len < 63) {
                char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);
                // Skip "." and ".." entries
                if (!(entry->name_len == 1 && entry_name[0] == '.') &&
                    !(entry->name_len == 2 && entry_name[0] == '.' && entry_name[1] == '.')) {
                    
                    // Copy directory name
                    for (int i = 0; i < entry->name_len; i++) {
                        path_parts[depth][i] = entry_name[i];
                    }
                    path_parts[depth][entry->name_len] = '\0';
                    depth++;
                    found = true;
                }
            }
            offset += entry->rec_len;
        }
        
        if (!found) break;
        inode = parent;
    }
    
    // Build the path string from root down
    strcpy(current_path, "/");
    for (int i = depth - 1; i >= 0; i--) {
        if (strlen(current_path) > 1) {
            strcat(current_path, "/");
        }
        strcat(current_path, path_parts[i]);
    }
}

void processCommand(char *command)
{
    char *argv[10];
    int argc = 0;
    char *p = command;
    
    // Split string command dan args nya
    // buang leading spaces dan newline
    while (*p == ' ' || *p == '\n') p++;
    while (*p && argc < 10) {
        while (*p == ' ') p++;
        if (!*p) break;
    
        if (*p == '"') {    
            p++;
            argv[argc++] = p;
        
            while (*p && *p != '"') p++;
                if (*p == '"') {
                    *p = '\0';
                    p++;
                }
        
            while (*p == ' ') p++;
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\n') p++;
            if (*p) {
                *p = '\0';
                p++;
            }
        }
    }

    if (argc == 0) {
        return; // Kosongan alias enter doang
    }

    // Pengecekan command
    char *cmd = argv[0];

    if (memcmp(cmd, "cd", 2) == 0 && strlen(cmd) == 2) {
        uint32_t old_inode = currentInode;
        cd(&currentInode, argc, argv);
        // Update path if cd was successful (inode changed)
        if (currentInode != old_inode) {
            update_path_display();
        }
    }
    
    else if (memcmp(cmd, "ls", 2) == 0 && strlen(cmd) == 2)
        ls(currentInode);
    
    else if (memcmp(cmd, "mkdir", 5) == 0 && strlen(cmd) == 5)
        mkdir(currentInode, argc, argv);
        
    else if (memcmp(cmd, "cat", 3) == 0 && strlen(cmd) == 3)
        cat(currentInode, argc, argv);

    else if (memcmp(cmd, "cp", 2) == 0 && strlen(cmd) == 2)
        cp(currentInode, argc, argv);

    else if (memcmp(cmd, "rm", 2) == 0 && strlen(cmd) == 2)
        rm(currentInode, argc, argv);
    
    else if (memcmp(cmd, "mv", 2) == 0 && strlen(cmd) == 2)
        mv(currentInode, argc, argv);
    
    else if (memcmp(cmd, "find", 4) == 0 && strlen(cmd) == 4)
        find(argc, argv);
    
    else if (memcmp(cmd, "echo", 4) == 0 && strlen(cmd) == 4)
        echo(currentInode, argc, argv);
    
    else if (memcmp(cmd, "flex", 4) == 0 && strlen(cmd) == 4)
        flex(argc, argv);

    else if (memcmp(cmd, "clear", 5) == 0 && strlen(cmd) == 5)
        syscall(8,0,0,0);

    else if (memcmp(cmd, "touch", 5) == 0 && strlen(cmd) == 5)
        touch(currentInode, argc, argv);    

    else if (memcmp(cmd, "kill", 4) == 0 && strlen(cmd) == 4)
        kill(argc, argv);

    else if (memcmp(cmd, "ps", 2) == 0 && strlen(cmd) == 2) {
        syscall(12,0,0,0);
        syscall(5, (uint32_t) '\n', 0xF, 0);
        syscall(5, (uint32_t) '\n', 0xF, 0);
    }
    else if (memcmp(cmd, "exec", 4) == 0 && strlen(cmd) == 4)
        exec(argc, argv);

    else {
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
    char *OSname = "@LOSS-2025";
    syscall(6, (uint32_t) user, strlen(user), 0xA);
    syscall(6, (uint32_t) OSname, strlen(OSname), 0xF);
    syscall(6, (uint32_t) ":", 1, 0xF);
    syscall(6, (uint32_t) current_path, strlen(current_path), 0xB); // Cyan color for path
    syscall(6, (uint32_t) " $ ", 3, 0xF);

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
    syscall(7, 0, 0, 0);
    update_path_display(); // Initialize path display
    while (true)
    {
        /* code */
        terminal();
    }
    
    return 0;
}