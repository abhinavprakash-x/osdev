#include "ulib.h"

static int initialized_data = 1337;
static int uninitialized_bss;

int main(void)
{
    uint32_t pid = sys_getpid();
    uninitialized_bss += 42;

    uprintf("\n[hello.elf] Hello from a real C ELF32 program in Ring 3!\n");
    uprintf("[hello.elf] My PID is %d | .data = %d | .bss = %d\n",
            (int)pid, initialized_data, uninitialized_bss);
    return 0;
}