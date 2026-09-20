#pragma once

#include <stdint.h>

void itoa(int num, char* str);
int atoi(const char* str);

void itox(uint32_t num, char* str);
uint32_t xtoi(const char* str);

void printf(const char* format, ...);