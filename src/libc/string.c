/*
 * Custom String Library
 * Provides standard C-style string manipulation and comparison tools.
 */

#include "string.h"

size_t strlen(const char* s)
{
    size_t i = 0;
    while(s[i] != '\0') i++;
    return i;
}

int strcmp(const char* s1, const char* s2)
{
    size_t i = 0;
    while(s1[i] != '\0' && s1[i] == s2[i]) i++;
    return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}

int strncmp(const char* s1, const char* s2, size_t n)
{
    if(n == 0) return 0;

    size_t i = 0;
    while(i < n && s1[i] != '\0' && s1[i] == s2[i]) i++;
    if(i == n) return 0;

    return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}