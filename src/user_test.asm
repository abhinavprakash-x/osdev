[bits 32]

global user_test_start
global user_test_end

user_test_start:

    ; SYS_TEST
    mov eax, 0
    int 0x80

    ; Kernel should return 42 in EAX.
    ; Spin here so we can inspect it with QEMU.
user_test_loop:
    jmp user_test_loop

user_test_end: