#include "ulib.h"

int main(void)
{
    uprintf("\n[fib.elf] First 12 Fibonacci numbers (PID %d):\n  ", (int)sys_getpid());

    int a = 0, b = 1;
    for (int i = 0; i < 12; ++i)
    {
        uprintf("%d ", a);
        int next = a + b;
        a = b;
        b = next;
    }
    uprintf("\n[fib.elf] Done!\n");
    return 12;
}