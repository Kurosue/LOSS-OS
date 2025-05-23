#ifndef _STRING_H
#define _STRING_H

#include <stdint.h>
#include <stddef.h>

/**
 * C standard memset, check man memset or
 * https://man7.org/linux/man-pages/man3/memset.3.html for more details
 * 
 * @param s Pointer to memory area to set
 * @param c Constant byte value for filling memory area
 * @param n Memory area size in byte 
 * 
 * @return Pointer s
*/
void* memset(void *s, int c, size_t n);

/**
 * C standard memcpy, check man memcpy or
 * https://man7.org/linux/man-pages/man3/memcpy.3.html for more details
 * 
 * @param dest Starting location for memory area to set
 * @param src Pointer to source memory
 * @param n Memory area size in byte 
 * 
 * @return Pointer dest
*/
void* memcpy(void* restrict dest, const void* restrict src, size_t n);

/**
 * C standard memcmp, check man memcmp or
 * https://man7.org/linux/man-pages/man3/memcmp.3.html for more details
 * 
 * @param s1 Pointer to first memory area
 * @param s2 Pointer to second memory area
 * @param n Memory area size in byte 
 * 
 * @return Integer as error code, zero for equality, non-zero for inequality
*/
int memcmp(const void *s1, const void *s2, size_t n);

/**
 * C standard memmove, check man memmove or
 * https://man7.org/linux/man-pages/man3/memmove.3.html for more details
 * 
 * @param dest Pointer to destination memory
 * @param src Pointer to source memory
 * @param n Memory area size in byte 
 * 
 * @return Pointer dest
*/
void *memmove(void *dest, const void *src, size_t n);

// hell yeah
size_t strlen(const char *str);

/**
 * Split a mutable C-string in-place by replacing delimiter characters with '\0'.
 * Populates the provided output array with pointers to each token.
 *
 * Behavior on size bounds:
 * - If max_tokens is larger than the number of tokens: only actual tokens are written;
 *   out[i] for i>=returned_count are untouched.
 * - If max_tokens is smaller than the number of tokens: delimiters after reaching max_tokens
 *   are not replaced, and the last token pointer will include the remainder of the string
 *   (including any unprocessed delimiters).
 *
 * @param str        Input string to split (will be modified in-place).
 * @param delim      Delimiter character to split on.
 * @param out        Array of char* to receive pointers to each token.
 * @param max_tokens Maximum number of pointers that out can hold.
 *
 * @return Number of tokens written into out (<= max_tokens).
 */
size_t split(char *str, char delim, char **out, size_t max_tokens);

/**
 * Append a source C-string to a destination buffer if space allows.
 *
 * @param dest    Destination buffer, must be null-terminated.
 * @param src     Source C-string to append.
 * @param bufsize Total size of dest buffer in bytes.
 *
 * @return 0 on success (dest updated), -1 if buffer overflow would occur (dest unchanged).
 */
int append(char *dest, const char *src, size_t bufsize);

#endif
