#include "ulib.h"

int main(void)
{
    int pid = (int)sys_getpid();
    uprintf("\n[counter.elf] Starting 5-second background counter (PID %d)...\n", pid);

    for (int i = 1; i <= 5; ++i)
    {
        sys_sleep(1000);
        uprintf("[counter.elf] PID %d tick %d/5\n", pid, i);
    }

    uprintf("[counter.elf] PID %d finished!\n", pid);
    return 0;
}