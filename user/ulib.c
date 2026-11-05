#include "ulib.h"

typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)

extern int main(void);

__attribute__((section(".text.start"), noreturn))
void _start(void)
{
    int status = main();
    sys_exit(status);
}

int32_t sys_test(void)
{
    int32_t ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(SYS_TEST) : "memory");
    return ret;
}

int32_t sys_write(const char* buf, uint32_t len)
{
    int32_t ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(SYS_WRITE), "b"(buf), "c"(len) : "memory");
    return ret;
}

void sys_exit(int32_t status)
{
    __asm__ volatile ("int $0x80" : : "a"(SYS_EXIT), "b"(status) : "memory");
    while (1) { }
}

int32_t sys_yield(void)
{
    int32_t ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(SYS_YIELD) : "memory");
    return ret;
}

int32_t sys_sleep(uint32_t ms)
{
    int32_t ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(SYS_SLEEP), "b"(ms) : "memory");
    return ret;
}

uint32_t sys_getpid(void)
{
    uint32_t ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(SYS_GETPID) : "memory");
    return ret;
}

int32_t sys_waitpid(uint32_t pid, int32_t* out_status)
{
    int32_t ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(SYS_WAITPID), "b"(pid), "c"(out_status) : "memory");
    return ret;
}

size_t strlen(const char* s)
{
    size_t i = 0;
    while (s[i] != '\0') i++;
    return i;
}

void uprint(const char* s)
{
    sys_write(s, (uint32_t)strlen(s));
}

static void uitoa(int num, char* str)
{
    int i = 0;
    int neg = 0;

    if (num == -2147483648)
    {
        const char* min_s = "-2147483648";
        for (i = 0; min_s[i] != '\0'; ++i) str[i] = min_s[i];
        str[i] = '\0';
        return;
    }
    if (num == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    if (num < 0)
    {
        neg = 1;
        num = -num;
    }
    while (num != 0)
    {
        str[i++] = (char)((num % 10) + '0');
        num /= 10;
    }
    if (neg) str[i++] = '-';
    str[i] = '\0';

    for (int l = 0, r = i - 1; l < r; ++l, --r)
    {
        char tmp = str[l];
        str[l] = str[r];
        str[r] = tmp;
    }
}

void uprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    for (size_t i = 0; format[i] != '\0'; ++i)
    {
        if (format[i] == '%')
        {
            ++i;
            if (format[i] == '\0') break;
            if (format[i] == 'd')
            {
                char buf[32];
                uitoa(va_arg(args, int), buf);
                uprint(buf);
            }
            else if (format[i] == 's')
            {
                const char* s = va_arg(args, const char*);
                uprint(s ? s : "(null)");
            }
            else if (format[i] == 'c')
            {
                char c = (char)va_arg(args, int);
                sys_write(&c, 1);
            }
            else if (format[i] == '%')
            {
                sys_write("%", 1);
            }
        }
        else
        {
            sys_write(&format[i], 1);
        }
    }

    va_end(args);
}