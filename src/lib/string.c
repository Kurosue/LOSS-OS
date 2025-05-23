#include <stdint.h>
#include <stddef.h>
#include "../header/lib/string.h"

void* memset(void *s, int c, size_t n) {
    uint8_t *buf = (uint8_t*) s;
    for (size_t i = 0; i < n; i++)
        buf[i] = (uint8_t) c;
    return s;
}

void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
    uint8_t *dstbuf       = (uint8_t*) dest;
    const uint8_t *srcbuf = (const uint8_t*) src;
    for (size_t i = 0; i < n; i++)
        dstbuf[i] = srcbuf[i];
    return dstbuf;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *buf1 = (const uint8_t*) s1;
    const uint8_t *buf2 = (const uint8_t*) s2;
    for (size_t i = 0; i < n; i++) {
        if (buf1[i] < buf2[i])
            return -1;
        else if (buf1[i] > buf2[i])
            return 1;
    }

    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *dstbuf       = (uint8_t*) dest;
    const uint8_t *srcbuf = (const uint8_t*) src;
    if (dstbuf < srcbuf) {
        for (size_t i = 0; i < n; i++)
            dstbuf[i]   = srcbuf[i];
    } else {
        for (size_t i = n; i != 0; i--)
            dstbuf[i-1] = srcbuf[i-1];
    }

    return dest;
}

// yep
size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

size_t split(char *str, char delim, char **out, size_t max_tokens) {
    if (!str || !out || max_tokens == 0) return 0;
    size_t count = 0;
    char *p = str;
    out[count++] = p;
    while (*p && count < max_tokens) {
        if (*p == delim) {
            *p = '\0';
            if (*(p + 1) != '\0') {
                out[count++] = p + 1;
            }
        }
        p++;
    }
    return count;
}

int append(char *dest, const char *src, size_t bufsize) {
    size_t dlen = strlen(dest);
    size_t slen = strlen(src);
    if (dlen + slen + 1 > bufsize) return -1;
    memcpy(dest + dlen, src, slen + 1);
    return 0;
}
