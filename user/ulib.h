#pragma once
#include <stdint.h>
#include <stddef.h>

#define SYS_TEST    0
#define SYS_WRITE   1
#define SYS_EXIT    2
#define SYS_YIELD   3
#define SYS_SLEEP   4
#define SYS_GETPID  5
#define SYS_WAITPID 6
#define SYS_READ    7
#define SYS_EXEC    8

int32_t  sys_test(void);
int32_t  sys_write(const char* buf, uint32_t len);
void     sys_exit(int32_t status) __attribute__((noreturn));
int32_t  sys_yield(void);
int32_t  sys_sleep(uint32_t ms);
uint32_t sys_getpid(void);
int32_t  sys_waitpid(uint32_t pid, int32_t* out_status);
int32_t  sys_read(char* buf, uint32_t max_len);

size_t strlen(const char* s);
void   uprint(const char* s);
void   uprintf(const char* format, ...);

int32_t readline(char* buf, uint32_t max_len);
int uatoi(const char* str);
void uitoa(int num, char* str);
int strcmp(const char* s1, const char* s2);