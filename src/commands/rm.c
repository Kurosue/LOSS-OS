#include "commands/rm.h"
#include "filesystem/ext2.h"
#include "lib/string.h"

extern void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

// Helper function to check if directory is empty (only has . and ..)
bool isDirectoryEmpty(uint32_t parent_inode, const char *dirname) {
    char data_buffer[BLOCK_SIZE * 16] = {};
    struct EXT2DriverRequest reqDir = {
        .buf                   = &data_buffer,
        .name                  = (char *)dirname,
        .parent_inode          = parent_inode,
        .buffer_size           = BLOCK_SIZE * 16,
        .name_len              = strlen(dirname),
    };
    
    int32_t retCode;
    syscall(1, (uint32_t) &reqDir, (uint32_t) &retCode, 0);
    
    if (retCode != 0) return false;
    
    uint32_t current_offset = 0;
    int entry_count = 0;
    
    while (current_offset < reqDir.buffer_size) {
        struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(data_buffer + current_offset);
        
        if (entry->inode == 0) {
            if (entry->rec_len == 0) break;
            current_offset += entry->rec_len;
            continue;
        }
        
        if (current_offset + entry->rec_len > reqDir.buffer_size || 
            entry->rec_len < (sizeof(struct EXT2DirectoryEntry) + entry->name_len)) {
            break;
        }
        
        char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);
        
        // Count entries (skip . and ..)
        if (!(entry->name_len == 1 && entry_name[0] == '.') &&
            !(entry->name_len == 2 && entry_name[0] == '.' && entry_name[1] == '.')) {
            entry_count++;
        }
        
        current_offset += entry->rec_len;
    }
    
    return entry_count == 0;
}

// Helper function to get inode of a file/directory by name
uint32_t getInodeByName(uint32_t parent_inode, const char *name) {
    char data_buffer[BLOCK_SIZE * 16] = {};
    struct EXT2DriverRequest reqDir = {
        .buf                   = &data_buffer,
        .name                  = ".",
        .parent_inode          = parent_inode,
        .buffer_size           = BLOCK_SIZE * 16,
        .name_len              = 1,
    };
    
    int32_t retCode;
    syscall(1, (uint32_t) &reqDir, (uint32_t) &retCode, 0);
    
    if (retCode != 0) return 0;
    
    uint32_t current_offset = 0;
    
    while (current_offset < reqDir.buffer_size) {
        struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(data_buffer + current_offset);
        
        if (entry->inode == 0) {
            if (entry->rec_len == 0) break;
            current_offset += entry->rec_len;
            continue;
        }
        
        if (current_offset + entry->rec_len > reqDir.buffer_size || 
            entry->rec_len < (sizeof(struct EXT2DirectoryEntry) + entry->name_len)) {
            break;
        }
        
        char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);
        
        if (entry->name_len == strlen(name) && memcmp(entry_name, name, entry->name_len) == 0) {
            return entry->inode;
        }
        
        current_offset += entry->rec_len;
    }
    
    return 0;
}

// Recursive remove function - improved version
void removeRecursive(uint32_t parent_inode, const char *name) {
    // First, let's try to delete it directly in case it's a file
    struct EXT2DriverRequest deleteReq = {
        .buf                   = NULL,
        .name                  = (char *)name,
        .parent_inode          = parent_inode,
        .buffer_size           = 0,
        .name_len              = strlen(name),
        .is_directory          = false,  // Try as file first
    };
    
    int32_t deleteRetCode;
    syscall(3, (uint32_t) &deleteReq, (uint32_t) &deleteRetCode, 0);
    
    if (deleteRetCode == 0) {
        // Successfully deleted as file
        return;
    }
    
    // If file deletion failed, try as directory
    uint32_t target_inode = getInodeByName(parent_inode, name);
    if (target_inode == 0) {
        const char *msg = "rm: cannot find ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)name, strlen(name), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return;
    }
    
    // Try to read as directory to confirm it's a directory
    char data_buffer[BLOCK_SIZE * 16] = {};
    struct EXT2DriverRequest reqDir = {
        .buf                   = &data_buffer,
        .name                  = ".",
        .parent_inode          = target_inode,
        .buffer_size           = BLOCK_SIZE * 16,
        .name_len              = 1,
    };
    
    int32_t retCode;
    syscall(1, (uint32_t) &reqDir, (uint32_t) &retCode, 0);
    
    if (retCode != 0) {
        // Not a directory, maybe the file delete failed for another reason
        const char *msg = "rm: cannot access ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)name, strlen(name), 0xF);
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
        return;
    }
    
    // It's a directory, remove all contents first
    uint32_t current_offset = 0;
    
    // We need to collect all entries first, then delete them
    // This avoids issues with modifying directory while reading it
    char entry_names[64][256];  // Support up to 64 entries, 256 chars each
    int entry_count = 0;
    
    while (current_offset < reqDir.buffer_size && entry_count < 64) {
        struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)(data_buffer + current_offset);
        
        if (entry->inode == 0) {
            if (entry->rec_len == 0) break;
            current_offset += entry->rec_len;
            continue;
        }
        
        if (current_offset + entry->rec_len > reqDir.buffer_size || 
            entry->rec_len < (sizeof(struct EXT2DirectoryEntry) + entry->name_len)) {
            break;
        }
        
        char *entry_name = (char *)entry + sizeof(struct EXT2DirectoryEntry);
        
        // Skip . and .. entries
        if (!(entry->name_len == 1 && entry_name[0] == '.') &&
            !(entry->name_len == 2 && entry_name[0] == '.' && entry_name[1] == '.')) {
            
            // Store entry name for later deletion
            if (entry->name_len < 255) {
                memcpy(entry_names[entry_count], entry_name, entry->name_len);
                entry_names[entry_count][entry->name_len] = '\0';
                entry_count++;
            }
        }
        
        current_offset += entry->rec_len;
    }
    
    // Now delete all collected entries
    for (int i = 0; i < entry_count; i++) {
        removeRecursive(target_inode, entry_names[i]);
    }
    
    // Now delete the directory itself
    deleteReq.is_directory = true;
    deleteReq.name = (char *)name;
    deleteReq.parent_inode = parent_inode;
    deleteReq.name_len = strlen(name);
    
    syscall(3, (uint32_t) &deleteReq, (uint32_t) &deleteRetCode, 0);
    
    if (deleteRetCode != 0) {
        const char *msg = "rm: failed to remove directory ";
        syscall(6, (uint32_t)msg, strlen(msg), 0x4);
        syscall(6, (uint32_t)name, strlen(name), 0xF);
        
        // Print specific error
        if (deleteRetCode == 1) {
            const char *msg2 = " (not found)";
            syscall(6, (uint32_t)msg2, strlen(msg2), 0x4);
        } else if (deleteRetCode == 2) {
            const char *msg2 = " (not empty)";
            syscall(6, (uint32_t)msg2, strlen(msg2), 0x4);
        } else {
            const char *msg2 = " (unknown error)";
            syscall(6, (uint32_t)msg2, strlen(msg2), 0x4);
        }
        
        const char *newline = "\n";
        syscall(6, (uint32_t)newline, 1, 0xF);
    }
}

void rm(uint32_t current_inode, int argc, char *argv[]) {
    bool recursive = false;
    int start_index = 1;
    
    if (argc < 2) {
        const char *msg = "Usage: rm [-r] <file|directory>\n";
        syscall(6, (uint32_t)msg, strlen(msg), 0x7);
        return;
    }
    
    // Check for -r flag
    if (argc >= 2 && memcmp(argv[1], "-r", 2) == 0 && strlen(argv[1]) == 2) {
        recursive = true;
        start_index = 2;
        
        if (argc < 3) {
            const char *msg = "Usage: rm [-r] <file|directory>\n";
            syscall(6, (uint32_t)msg, strlen(msg), 0x7);
            return;
        }
    }
    
    for (int i = start_index; i < argc; i++) {
        char *target = argv[i];
        
        if (recursive) {
            removeRecursive(current_inode, target);
        } else {
            // Non-recursive delete - files or empty directories
            struct EXT2DriverRequest deleteReq = {
                .buf                   = NULL,
                .name                  = target,
                .parent_inode          = current_inode,
                .buffer_size           = 0,
                .name_len              = strlen(target),
                .is_directory          = false,
            };
            
            int32_t retCode;
            syscall(3, (uint32_t) &deleteReq, (uint32_t) &retCode, 0);
            
            if (retCode != 0) {
                // File deletion failed, try as directory (might be empty directory)
                deleteReq.is_directory = true;
                syscall(3, (uint32_t) &deleteReq, (uint32_t) &retCode, 0);
                
                if (retCode == 2) {
                    // Directory not empty
                    const char *msg = "rm: directory not empty (use -r for recursive removal): ";
                    syscall(6, (uint32_t)msg, strlen(msg), 0x4);
                    syscall(6, (uint32_t)target, strlen(target), 0xF);
                    const char *newline = "\n";
                    syscall(6, (uint32_t)newline, 1, 0xF);
                } else if (retCode == 1) {
                    // Not found
                    const char *msg = "rm: cannot remove '";
                    syscall(6, (uint32_t)msg, strlen(msg), 0x4);
                    syscall(6, (uint32_t)target, strlen(target), 0xF);
                    const char *msg2 = "': No such file or directory\n";
                    syscall(6, (uint32_t)msg2, strlen(msg2), 0x4);
                } else if (retCode == -1) {
                    // Other error
                    const char *msg = "rm: failed to remove ";
                    syscall(6, (uint32_t)msg, strlen(msg), 0x4);
                    syscall(6, (uint32_t)target, strlen(target), 0xF);
                    const char *newline = "\n";
                    syscall(6, (uint32_t)newline, 1, 0xF);
                }
                else if (retCode == 0) {
                    // Successfully deleted directory
                    const char *msg = "Removed directory: ";
                    syscall(6, (uint32_t)msg, strlen(msg), 0x7);
                    syscall(6, (uint32_t)target, strlen(target), 0xF);
                    const char *newline = "\n";
                    syscall(6, (uint32_t)newline, 1, 0xF);
                }
            }
            else {
                // Successfully deleted file
                const char *msg = "Removed: ";
                syscall(6, (uint32_t)msg, strlen(msg), 0x7);
                syscall(6, (uint32_t)target, strlen(target), 0xF);
                const char *newline = "\n";
                syscall(6, (uint32_t)newline, 1, 0xF);
            }
        }
    }
}