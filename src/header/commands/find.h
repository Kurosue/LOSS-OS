#include <stdint.h>

/**
 * Helper function to find with recursion
 * @param target Target filename to find
 * @param parent_inode Parent inode to search in
 * @param currentPath Current path being searched
 */
void findRecursive(char* target, uint32_t parent_inode, char* currentPath);

/**
 * Print file contents command
 * @param argc Argument count
 * @param argv Argument vector
 * @return void
 */
void find(int argc, char *argv[]);
