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
    return s1[i] - s2[i];
}

int strncmp(const char* s1, const char* s2, size_t n)
{
    size_t i = 0;
    while(s1[i] != '\0' && s1[i] == s2[i] && i < n) i++;
    if(i == n) return 0;
    return s1[i] - s2[i];
}