[bits 32]

global user_test_start
global user_test_end

user_test_start:

    ; SYS_TEST
    mov eax, 0
    int 0x80
    mov esi, eax             ; Expected 42


    ; SYS_GETPID
    mov eax, 5
    int 0x80
    mov edi, eax             ; Expected 1


    ; SYS_YIELD
    mov eax, 3
    int 0x80
    mov edx, 0x5949454C      ; "YIEL"


    ; SYS_SLEEP
    mov eax, 4
    mov ebx, 5000
    int 0x80
    mov ecx, 0x534C4545      ; "SLEE"


    ; SYS_EXIT
    mov eax, 2
    xor ebx, ebx
    int 0x80

    ; Should never execute
    jmp $


user_test_end: