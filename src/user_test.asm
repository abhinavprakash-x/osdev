[bits 32]

global user_test_start
global user_test_end

user_test_start:
    mov eax, 0xDEADBEEF

user_test_loop:
    jmp user_test_loop

user_test_end: