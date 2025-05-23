#include <stdint.h>

/**
 * Change directory command
 * @param path Path to change to (can be relative or absolute)
 * @param current_inode Pointer to current inode (will be updated)
 */
void cd(uint32_t *current_inode, int argc, char *argv[]);