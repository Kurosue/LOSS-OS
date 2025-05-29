#include <stdint.h>

/**
 * Get parent inode of a given inode
 * @param inode The inode to get the parent of
 * @return Parent inode number, or 1 if the inode is root or an error occurs
 */
uint32_t get_parent_inode(uint32_t inode);

/**
 * Find child inode by name under a given parent inode
 * @param parent_inode The parent inode to search under
 * @param name The name of the child directory or file
 * @param name_len Length of the name
 * @return Child inode number, or 0 if not found
 */
uint32_t find_child_inode(uint32_t parent_inode, const char *name, uint8_t name_len);

/**
 * Split a path into components
 * @param path The path to split
 * @param components Array to store the components
 * @param max_components Maximum number of components to store
 * @return Number of components found
 */
int split_path(const char *path, char components[][64], int max_components);

/**
 * Change directory command
 * @param path Path to change to (can be relative or absolute)
 * @param current_inode Pointer to current inode (will be updated)
 */
void cd(uint32_t *current_inode, int argc, char *argv[]);