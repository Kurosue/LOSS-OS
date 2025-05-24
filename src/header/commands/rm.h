#include <stdint.h>
#include <stdbool.h>

/**
 * Helper function to check if directory is empty (only has . and ..)
 * @param parent_inode Parent inode containing the directory
 * @param dirname Name of directory to check
 * @return true if directory is empty, false otherwise
 */
bool isDirectoryEmpty(uint32_t parent_inode, const char *dirname);

/**
 * Helper function to get inode of a file/directory by name
 * @param parent_inode Parent directory inode
 * @param name Name of file/directory to find
 * @return inode number, 0 if not found
 */
uint32_t getInodeByName(uint32_t parent_inode, const char *name);

/**
 * Recursive remove function
 * @param parent_inode Parent directory inode
 * @param name Name of file/directory to remove
 */
void removeRecursive(uint32_t parent_inode, const char *name);

/**
 * Remove files and directories command
 * @param current_inode Current directory inode
 * @param argc Argument count
 * @param argv Argument vector
 */
void rm(uint32_t current_inode, int argc, char *argv[]);