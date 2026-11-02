extern int main(void);

__attribute__((section(".text.start")))
void _start(void)
{
    int status = main();
    sys_exit(status);
}