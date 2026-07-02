#include "stdlib.h"

typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)

static void reverse(char str[], int n)
{
    int left = 0, right = n - 1;
    while(left < right) 
    {
        char temp = str[left];
        str[left] = str[right];
        str[right] = temp;
        left++;
        right--;
    }
}

void itoa(int num, char* str)
{
    int i = 0;
    int is_negative = 0;

    if(num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    if(num < 0)
    {
        is_negative = 1;
        num = -num;
    }
    while(num != 0)
    {
        int digit = num % 10;
        num /= 10;
        str[i++] = digit + '0';
    }

    if(is_negative) str[i++] = '-';
    str[i] = '\0';
    reverse(str, i);
}

int atoi(const char* str)
{
    int result = 0;
    int sign = 1;
    int i = 0;

    while (str[i] == ' ') i++;

    if (str[i] == '-' || str[i] == '+')
    {
        if (str[i] == '-') sign = -1;
        i++;
    }
    while (str[i] >= '0' && str[i] <= '9')
    {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return sign * result;
}

void itox(uint32_t num, char* str)
{
    int i = 0;
    if(num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    while(num != 0)
    {
        int digit = num % 16;
        num /= 16;

        if(digit < 10) str[i++] = digit + '0';
        else str[i++] = 'A' + (digit - 10);
    }

    str[i] = '\0';
    reverse(str, i);
}

uint32_t xtoi(const char* str)
{
    uint32_t result = 0;
    int i = 0;

    if (str[i] == '0' && (str[i+1] == 'x' || str[i+1] == 'X')) {
        i += 2;
    }

    while (str[i] != '\0')
    {
        int digit = 0;
        if (str[i] >= '0' && str[i] <= '9') digit = str[i] - '0';
        else if (str[i] >= 'a' && str[i] <= 'f') digit = str[i] - 'a' + 10;
        else if (str[i] >= 'A' && str[i] <= 'F') digit = str[i] - 'A' + 10;
        else break;

        result = (result * 16) + digit;
        i++;
    }
    return result;
}

void printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    for(int i = 0; format[i] != '\0'; ++i)
    {
        if(format[i] == '%')
        {
            i++;
            if(format[i] == '\0') break;
            switch(format[i])
            {
                case 'd':
                {
                    int val = va_arg(args, int);
                    char str[32];
                    itoa(val, str);
                    print(str);
                    break;
                }
                case 'x':
                {
                    uint32_t val = va_arg(args, uint32_t);
                    char str[32];
                    itox(val, str);
                    print(str);
                    break;
                }
                case 's':
                {
                    char* val = va_arg(args, char*);
                    print(val);
                    break;
                }
                case '%':
                {
                    putchar('%');
                    break;
                }
            }
        }
        else
        {
            putchar(format[i]);
        }
    }
    va_end(args);
}