[bits 32]

global user_test_start
global user_test_end


; =========================================================
; Syscall numbers
; =========================================================

SYS_TEST    equ 0
SYS_WRITE   equ 1
SYS_EXIT    equ 2
SYS_YIELD   equ 3
SYS_SLEEP   equ 4
SYS_GETPID  equ 5


; =========================================================
; Convert a raw-binary label into a runtime user address.
;
; Example:
;
;     LOAD_ADDRESS msg_header
;
; Result:
;     ESI = 0x00400000 + offset_of(msg_header)
;
; This is necessary because the user program is copied
; to 0x400000 rather than linked as a relocated ELF.
; =========================================================

%macro LOAD_ADDRESS 1
    call %%get_eip

%%get_eip:
    pop esi
    add esi, %1 - %%get_eip
%endmacro


user_test_start:

    ; =====================================================
    ; USER SYSCALL TESTS
    ; =====================================================

    ; -----------------------------------------------------
    ; Header
    ; -----------------------------------------------------
    LOAD_ADDRESS msg_header
    call write_string


    ; =====================================================
    ; TEST 1: SYS_TEST
    ; =====================================================

    mov eax, SYS_TEST
    int 0x80

    cmp eax, 42
    jne .sys_test_failed

    LOAD_ADDRESS msg_test_pass
    call write_string
    jmp .sys_test_done

.sys_test_failed:
    LOAD_ADDRESS msg_test_fail
    call write_string

.sys_test_done:


    ; =====================================================
    ; TEST 2: SYS_GETPID
    ; =====================================================

    mov eax, SYS_GETPID
    int 0x80

    ; PID 0 belongs to kmain.
    cmp eax, 0
    je .getpid_failed

    ; Preserve PID for debugging.
    mov edi, eax

    LOAD_ADDRESS msg_getpid_pass
    call write_string
    jmp .getpid_done

.getpid_failed:
    LOAD_ADDRESS msg_getpid_fail
    call write_string

.getpid_done:


    ; =====================================================
    ; TEST 3: SYS_WRITE - valid pointer
    ; =====================================================

    ; Get runtime address of the message.
    LOAD_ADDRESS msg_ring3

    ; EBX = userspace buffer
    mov ebx, esi

    ; ECX = number of bytes to write
    mov ecx, msg_ring3_len

    mov eax, SYS_WRITE
    int 0x80

    cmp eax, msg_ring3_len
    jne .write_valid_failed

    LOAD_ADDRESS msg_write_valid_pass
    call write_string
    jmp .write_valid_done

.write_valid_failed:
    LOAD_ADDRESS msg_write_valid_fail
    call write_string

.write_valid_done:


    ; =====================================================
    ; TEST 4: SYS_WRITE - invalid pointer
    ; =====================================================

    mov eax, SYS_WRITE
    mov ebx, 0xDEADBEEF
    mov ecx, 10
    int 0x80

    ; Invalid user pointer must return -1.
    cmp eax, -1
    jne .write_invalid_failed

    LOAD_ADDRESS msg_write_invalid_pass
    call write_string
    jmp .write_invalid_done

.write_invalid_failed:
    LOAD_ADDRESS msg_write_invalid_fail
    call write_string

.write_invalid_done:


    ; =====================================================
    ; TEST 5: SYS_YIELD
    ; =====================================================

    mov eax, SYS_YIELD
    int 0x80

    cmp eax, 0
    jne .yield_failed

    LOAD_ADDRESS msg_yield_pass
    call write_string
    jmp .yield_done

.yield_failed:
    LOAD_ADDRESS msg_yield_fail
    call write_string

.yield_done:


    ; =====================================================
    ; TEST 6: SYS_SLEEP
    ; =====================================================

    mov eax, SYS_SLEEP
    mov ebx, 2000               ; 2 seconds
    int 0x80

    cmp eax, 0
    jne .sleep_failed

    LOAD_ADDRESS msg_sleep_pass
    call write_string
    jmp .sleep_done

.sleep_failed:
    LOAD_ADDRESS msg_sleep_fail
    call write_string

.sleep_done:


    ; =====================================================
    ; ALL TESTS PASSED
    ; =====================================================

    LOAD_ADDRESS msg_all_pass
    call write_string


    ; =====================================================
    ; TEST 7: SYS_EXIT
    ; =====================================================

    xor ebx, ebx                ; exit status = 0
    mov eax, SYS_EXIT
    int 0x80

    ; SYS_EXIT should never return.
    LOAD_ADDRESS msg_exit_fail
    call write_string

.exit_returned:
    jmp .exit_returned



; =========================================================
; write_string
;
; Input:
;     ESI = runtime address of NUL-terminated string
;
; Clobbers:
;     EAX, EBX, ECX
;
; Preserves:
;     ESI
; =========================================================

write_string:

    push esi

    mov ebx, esi
    xor ecx, ecx

.count:
    cmp byte [ebx + ecx], 0
    je .count_done

    inc ecx
    jmp .count

.count_done:

    mov eax, SYS_WRITE
    int 0x80

    pop esi
    ret



; =========================================================
; Strings
; =========================================================

msg_header:
    db 10
    db "================================", 10
    db "       USER SYSCALL TESTS", 10
    db "================================", 10
    db 0


msg_test_pass:
    db "[ OK ] SYS_TEST", 10
    db 0

msg_test_fail:
    db "[FAIL] SYS_TEST", 10
    db 0


msg_getpid_pass:
    db "[ OK ] SYS_GETPID", 10
    db 0

msg_getpid_fail:
    db "[FAIL] SYS_GETPID", 10
    db 0


msg_ring3:
    db "Hello from Ring 3!", 10
    msg_ring3_len equ $ - msg_ring3


msg_write_valid_pass:
    db "[ OK ] SYS_WRITE (valid pointer)", 10
    db 0

msg_write_valid_fail:
    db "[FAIL] SYS_WRITE (valid pointer)", 10
    db 0


msg_write_invalid_pass:
    db "[ OK ] SYS_WRITE (invalid pointer)", 10
    db 0

msg_write_invalid_fail:
    db "[FAIL] SYS_WRITE (invalid pointer)", 10
    db 0


msg_yield_pass:
    db "[ OK ] SYS_YIELD", 10
    db 0

msg_yield_fail:
    db "[FAIL] SYS_YIELD", 10
    db 0


msg_sleep_pass:
    db "[ OK ] SYS_SLEEP", 10
    db 0

msg_sleep_fail:
    db "[FAIL] SYS_SLEEP", 10
    db 0


msg_all_pass:
    db "================================", 10
    db "    ALL SYSCALL TESTS PASSED", 10
    db "================================", 10
    db 0


msg_exit_fail:
    db "[FAIL] SYS_EXIT returned!", 10
    db 0


user_test_end: