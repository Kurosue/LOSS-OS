#include <stdint.h>
#include "filesystem/ext2.h"

/**
 * Print string or Save string to file
 * @param argc Argument count
 * @param argv Argument vector
 * @param current_inode Current directory inode
 */
void echo(uint32_t current_inode, int argc, char *argv[]);