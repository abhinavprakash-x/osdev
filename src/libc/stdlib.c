#include "stdlib.h"

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

    while (str[i] == ' ') {
        i++;
    }

    if (str[i] == '-' || str[i] == '+') {
        if (str[i] == '-') sign = -1;
        i++;
    }
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return sign * result;
}