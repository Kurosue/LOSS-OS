#include "commands/cd.h"
#include "filesystem/ext2.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static uint32_t get_parent_inode(uint32_t inode) {
    if (inode == 1) return 1;
    
    unsigned char data_buffer[BLOCK_SIZE * 16];
    struct EXT2DriverRequest reqDir = {
        .buf                   = &data_buffer,
        .name                  = ".",
        .parent_inode          = inode,
        .buffer_size           = BLOCK_SIZE * 16,
        .name_len              = 1,
    };
    
    int32_t retCode = 0;
    syscall(1, (uint32_t) &reqDir, (uint32_t) &retCode, 0);
    
    if (retCode != 0) return 1;
    
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
    
    return 1;
}

static uint32_t find_child_inode(uint32_t parent_inode, const char *name, uint8_t name_len) {
    if (parent_inode == 0) return 0;
    
    unsigned char data_buffer[BLOCK_SIZE * 16];
    struct EXT2DriverRequest reqDir = {
        .buf                   = &data_buffer,
        .name                  = ".",
        .parent_inode          = parent_inode,
        .buffer_size           = BLOCK_SIZE * 16,
        .name_len              = 1,
    };
    
    int32_t retCode = 0;
    syscall(1, (uint32_t) &reqDir, (uint32_t) &retCode, 0);
    
    if (retCode != 0) return 0;
    
    uint32_t offset = 0;
    while (offset < BLOCK_SIZE * 16) {
        struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(data_buffer + offset);
        
        if (entry->rec_len == 0 || offset + entry->rec_len > BLOCK_SIZE * 16) break;
        
        if (entry->inode != 0 && entry->file_type == EXT2_FT_DIR && 
            entry->name_len == name_len) {
            char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);
            
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
    
    return 0;
}

static int split_path(const char *path, char components[][64], int max_components) {
    int count = 0;
    int i = 0;
    int comp_start = 0;
    int comp_len = 0;

    while (path[i] == '/') i++;
    
    comp_start = i;
    
    while (path[i] && count < max_components) {
        if (path[i] == '/') {
            if (comp_len > 0) {
                if (comp_len < 63) {
                    for (int j = 0; j < comp_len; j++) {
                        components[count][j] = path[comp_start + j];
                    }
                    components[count][comp_len] = '\0';
                    count++;
                }
                comp_len = 0;
            }
            while (path[i] == '/') i++;
            comp_start = i;
        } else {
            comp_len++;
            i++;
        }
    }
    
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

void cd(uint32_t *current_inode, int argc, char *argv[]) {

    const char *path = (argc >= 2) ? argv[1] : NULL;

    if (argc >= 2) {

        if (!path) {
            *current_inode = 1;
            return;
        }
        
        uint32_t target_inode = *current_inode;
        
        if (path[0] == '/') {
            target_inode = 1;
        }
        
        char components[16][64]; 
        int num_components = split_path(path, components, 16);
        
        bool success = true;
        for (int i = 0; i < num_components && success; i++) {
            if (strlen(components[i]) == 0) {
                continue;
            }
            
            if (strcmp(components[i], ".") == 0) {
                continue;
            } 
            else if (strcmp(components[i], "..") == 0) {
                uint32_t parent = get_parent_inode(target_inode);
                if (parent != 0) {
                    target_inode = parent;
                }
            } 
            else {
                uint32_t child = find_child_inode(target_inode, components[i], strlen(components[i]));
                if (child == 0) {
                    const char *msg1 = "cd: ";
                    const char *msg2 = ": No such directory\n";
                    syscall(6, (uint32_t)msg1, strlen(msg1), 0x4);
                    syscall(6, (uint32_t)components[i], strlen(components[i]), 0xF);
                    syscall(6, (uint32_t)msg2, strlen(msg2), 0x4);
                    success = false;
                } else {
                    target_inode = child;
                }
            }
        }
        
        if (success) {
            *current_inode = target_inode;
        }
    }
    else
    {
        const char *msg = "Usage: cd <path>\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
    }
}