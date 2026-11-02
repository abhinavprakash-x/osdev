/*
 * Ring 3 User-Mode, Syscall & ELF32 Loader Tests
 * Covers BUG-002, BUG-003, BUG-004, BUG-010, BUG-017, BUG-040, BUG-049,
 * Milestone 3 (Exit Statuses), and Milestone 4 (ELF32 Loader).
 */

#include "apps/kernel_tests.h"
#include "tests/test_framework.h"
#include "mm/pmm.h"
#include "mm/paging.h"
#include "mm/heap.h"
#include "task/task.h"
#include "task/scheduler.h"
#include "task/elf.h"
#include "libc/mem.h"
#include "libc/stdlib.h"
#include "drivers/pit.h"

extern const uint8_t user_test_start[];
extern const uint32_t user_test_size;

static void test_elf_task(void)
{
    printf("[ELF32 Validator & Loader]\n");

    const char* empty_image = "";
    test_assert_true(!elf_validate(0, 100), "elf_validate(null pointer rejected)");
    test_assert_true(!elf_validate(empty_image, 0), "elf_validate(empty size rejected)");

    // Build a synthetic 2-segment ELF32 executable in memory:
    // - Segment 0 (.text): RX at 0x00400000, reads initial value (99) from 0x00401000 and calls SYS_EXIT(99)
    // - Segment 1 (.data/.bss): RW at 0x00401000, 4 bytes initialized data (99) + 12 bytes zeroed .bss
    struct {
        Elf32_Ehdr ehdr;
        Elf32_Phdr phdr[2];
        uint8_t    code[16];
        uint32_t   data_val;
    } __attribute__((packed)) img;

    memset(&img, 0, sizeof(img));

    img.ehdr.e_ident[0] = ELFMAG0;
    img.ehdr.e_ident[1] = ELFMAG1;
    img.ehdr.e_ident[2] = ELFMAG2;
    img.ehdr.e_ident[3] = ELFMAG3;
    img.ehdr.e_ident[4] = ELFCLASS32;
    img.ehdr.e_ident[5] = ELFDATA2LSB;
    img.ehdr.e_ident[6] = 1;

    img.ehdr.e_type      = ET_EXEC;
    img.ehdr.e_machine   = EM_386;
    img.ehdr.e_version   = 1;
    img.ehdr.e_entry     = 0x00400000u;
    img.ehdr.e_phoff     = sizeof(Elf32_Ehdr);
    img.ehdr.e_ehsize    = sizeof(Elf32_Ehdr);
    img.ehdr.e_phentsize = sizeof(Elf32_Phdr);
    img.ehdr.e_phnum     = 2;

    // Segment 0: .text (Read + Execute) at 0x00400000
    img.phdr[0].p_type   = PT_LOAD;
    img.phdr[0].p_offset = (uint32_t)((uint8_t*)img.code - (uint8_t*)&img);
    img.phdr[0].p_vaddr  = 0x00400000u;
    img.phdr[0].p_filesz = sizeof(img.code);
    img.phdr[0].p_memsz  = sizeof(img.code);
    img.phdr[0].p_flags  = PF_R | PF_X;
    img.phdr[0].p_align  = PAGE_SIZE;

    // Segment 1: .data + .bss (Read + Write) at 0x00401000
    img.phdr[1].p_type   = PT_LOAD;
    img.phdr[1].p_offset = (uint32_t)((uint8_t*)&img.data_val - (uint8_t*)&img);
    img.phdr[1].p_vaddr  = 0x00401000u;
    img.phdr[1].p_filesz = sizeof(uint32_t); // 4 bytes from file (.data)
    img.phdr[1].p_memsz  = 16;               // + 12 bytes zeroed (.bss)
    img.phdr[1].p_flags  = PF_R | PF_W;
    img.phdr[1].p_align  = PAGE_SIZE;

    // Ring 3 machine code:
    // mov ebx, [0x00401000] (8B 1D 00 10 40 00) -> loads 99 from .data
    // add ebx, [0x00401004] (03 1D 04 10 40 00) -> adds 0 from zeroed .bss
    // mov eax, 2            (B8 02 00 00 00)    -> SYS_EXIT
    // int 0x80              (CD 80)
    static const uint8_t elf_code_payload[16] = {
        0x8B, 0x1D, 0x00, 0x10, 0x40, 0x00,
        0x03, 0x1D, 0x04, 0x10, 0x40, 0x00,
        0xB8, 0x02, 0x00, 0x00,
    };
    memcpy(img.code, elf_code_payload, sizeof(elf_code_payload));
    // Place final bytes of `mov eax, 2; int 0x80` in simpler 14-byte sequence:
    // 8B 1D 00 10 40 00 (mov ebx, [0x401000])
    // 03 1D 04 10 40 00 (add ebx, [0x401004])
    // 31 C0 40 40       (xor eax, eax; inc eax; inc eax -> eax = 2)
    // CD 80             (int 0x80)
    static const uint8_t compact_payload[16] = {
        0x8B, 0x1D, 0x00, 0x10, 0x40, 0x00,
        0x03, 0x1D, 0x04, 0x10, 0x40, 0x00,
        0x31, 0xC0, 0xCD, 0x80
    };
    // Let's set eax = 2 via: B8 02 00 00 00 (5 bytes) + 8B 1D 00 10 40 00 (6 bytes) + 03 1D 04 10 40 00 (6 bytes) -> 17 bytes,
    // or use `mov ebx, [0x401000]; mov eax, 2; int 0x80` (13 bytes):
    static const uint8_t clean_payload[16] = {
        0x8B, 0x1D, 0x00, 0x10, 0x40, 0x00, // mov ebx, [0x00401000] (.data = 99)
        0x03, 0x1D, 0x04, 0x10, 0x40, 0x00, // add ebx, [0x00401004] (.bss  = 0)
        0x6A, 0x02,                         // push 2
        0x58,                               // pop eax (eax = 2 = SYS_EXIT)
        0xCD                                // (see 16-byte array below)
    };
    (void)compact_payload;
    (void)clean_payload;

    // 13-byte exact sequence padded with NOPs to 16 bytes:
    const uint8_t final_code[16] = {
        0x8B, 0x1D, 0x00, 0x10, 0x40, 0x00, // mov ebx, [0x00401000]
        0xB8, 0x02, 0x00, 0x00, 0x00,       // mov eax, 2 (SYS_EXIT)
        0xCD, 0x80,                         // int 0x80
        0x90, 0x90, 0x90
    };
    memcpy(img.code, final_code, sizeof(final_code));
    img.data_val = 99u;

    // 1. Valid ELF32 image must pass
    test_assert_true(elf_validate(&img, sizeof(img)), "elf_validate(valid ELF32 accepted)");

    // 2. Bad Magic must be rejected
    img.ehdr.e_ident[0] = 0x00;
    test_assert_true(!elf_validate(&img, sizeof(img)), "elf_validate(bad magic rejected)");
    img.ehdr.e_ident[0] = ELFMAG0;

    // 3. Wrong Architecture must be rejected
    img.ehdr.e_machine = 62; // EM_X86_64
    test_assert_true(!elf_validate(&img, sizeof(img)), "elf_validate(wrong arch rejected)");
    img.ehdr.e_machine = EM_386;

    // 4. p_filesz > p_memsz must be rejected
    img.phdr[0].p_filesz = img.phdr[0].p_memsz + 1u;
    test_assert_true(!elf_validate(&img, sizeof(img)), "elf_validate(filesz > memsz rejected)");
    img.phdr[0].p_filesz = sizeof(img.code);

    // 5. Kernel-space virtual address (< USER_SPACE_START) must be rejected
    img.phdr[0].p_vaddr = 0x00100000u;
    test_assert_true(!elf_validate(&img, sizeof(img)), "elf_validate(kernel vaddr rejected)");
    img.phdr[0].p_vaddr = 0x00400000u;

    // 6. Kernel heap overlap must be rejected
    img.phdr[1].p_vaddr = HEAP_START;
    test_assert_true(!elf_validate(&img, sizeof(img)), "elf_validate(heap overlap rejected)");
    img.phdr[1].p_vaddr = 0x00401000u;

    // 7. Non-executable entry segment (missing PF_X) must be rejected
    img.phdr[0].p_flags = PF_R;
    test_assert_true(!elf_validate(&img, sizeof(img)), "elf_validate(non-executable entry rejected)");
    img.phdr[0].p_flags = PF_R | PF_X;

    // 8. End-to-end test: Load and execute the 2-segment ELF32 task in Ring 3
    int used_before = get_used_memory();
    task_t* elf_task = create_user_task_elf("elf_test", &img, sizeof(img));
    test_assert_true(elf_task != 0, "create_user_task_elf(created multi-segment task)");
    if (elf_task != 0)
    {
        uint32_t elf_pid = elf_task->pid;
        int32_t exit_status = 0;
        task_add(elf_task);

        bool waited = task_wait_pid(elf_pid, &exit_status);
        test_assert_true(waited, "create_user_task_elf(waited for ELF task)");
        test_assert_equal_int(99, (int)exit_status, "create_user_task_elf(executed .text and read 99 from .data)");
        test_assert_equal_int(used_before, get_used_memory(), "create_user_task_elf(all ELF pages reclaimed on exit)");
    }
}

void test_user_suite(void)
{
    printf("[User Payload & Task Creation]\n");

    test_assert_true(user_test_size > 0, "user_test_size(non-empty)");
    test_assert_true(user_test_size <= PAGE_SIZE, "user_test_size(fits in single 4KB page)");

    int used_before = get_used_memory();

    // BUG-010: Reject null buffer, zero size, or size > PAGE_SIZE
    test_assert_true(create_user_task("u_null", 0, user_test_size, USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR) == 0,
                     "user task BUG-010(null code rejected)");
    test_assert_true(create_user_task("u_zero", user_test_start, 0, USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR) == 0,
                     "user task BUG-010(zero size rejected)");
    test_assert_true(create_user_task("u_oversize", user_test_start, PAGE_SIZE + 1, USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR) == 0,
                     "user task BUG-010(>4KB payload rejected)");

    // BUG-003 & BUG-004: Force mapping failure (stack in shared kernel PDE 0 at 0x00100000, or colliding with code page)
    // and verify complete resource rollback without leaking PMM frames or heap blocks.
    task_t* bad_code_map = create_user_task("u_bad_code", user_test_start, user_test_size, 0x00100000u, USER_STACK_TOP_ADDR);
    test_assert_true(bad_code_map == 0, "user task BUG-004(shared kernel PDE code map rejected)");

    task_t* bad_stack_map = create_user_task("u_bad_stack", user_test_start, user_test_size, USER_CODE_LOAD_ADDR, USER_CODE_LOAD_ADDR + PAGE_SIZE);
    test_assert_true(bad_stack_map == 0, "user task BUG-004(colliding code/stack map rejected)");
    test_assert_equal_int(used_before, get_used_memory(), "user task BUG-003(failed creation leaks 0 frames)");

    // BUG-002 & BUG-017: Verify created user task has mapped code & stack and copied payload bytes
    task_t* utask = create_user_task("u_valid", user_test_start, user_test_size, USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR);
    test_assert_true(utask != 0, "user task BUG-002(create valid user task)");
    if (utask != 0)
    {
        uint32_t eflags;
        __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

        paging_switch_directory(utask->page_directory);
        bool code_mapped  = user_range_valid(USER_CODE_LOAD_ADDR, user_test_size);
        bool stack_mapped = user_range_valid(USER_STACK_TOP_ADDR - PAGE_SIZE, PAGE_SIZE);
        bool code_copied  = (*(const uint32_t*)USER_CODE_LOAD_ADDR == *(const uint32_t*)user_test_start);
        paging_switch_directory(paging_get_kernel_directory());

        __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");

        test_assert_true(code_mapped, "user task BUG-002(code page mapped in user space)");
        test_assert_true(stack_mapped, "user task BUG-002(stack page mapped in user space)");
        test_assert_true(code_copied, "user task BUG-017(payload copied via temp mapping)");

        destroy_task(utask);
    }

    test_assert_equal_int(used_before, get_used_memory(), "user task(cleanup restores PMM frames)");

    static const uint8_t crashing_user_code[4] = { 0xFA, 0xEB, 0xFE, 0x90 }; // cli; jmp $
    task_t* crash_task = create_user_task("u_crash", crashing_user_code, sizeof(crashing_user_code),
                                          USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR);
    test_assert_true(crash_task != 0, "user task BUG-049(create crashing user task)");
    if (crash_task != 0)
    {
        uint32_t crash_pid = crash_task->pid;
        task_add(crash_task);

        uint32_t start_ticks = get_ticks();
        uint32_t timeout = get_timer_freq();
        if (timeout == 0) timeout = 100;

        while (scheduler_pid_exists(crash_pid) && (get_ticks() - start_ticks) < timeout)
        {
            yield();
        }
        yield(); // Ensure reap_dead_tasks() frees the terminated task

        test_assert_true(!scheduler_pid_exists(crash_pid), "user task BUG-049(crashing Ring 3 task terminated & reaped)");
        test_assert_equal_int(used_before, get_used_memory(), "user task BUG-049(crashing task frames reclaimed)");

        int32_t crash_status = 0;
        test_assert_true(task_get_exit_status(crash_pid, &crash_status) && crash_status == -141,
                         "user task Milestone3(crash exit status recorded as -141)");
    }

    // Verify Ring 3 SYS_EXIT records exit status from EBX and task_wait_pid() retrieves it
    // Machine code: mov ebx, 77 (BB 4D 00 00 00); mov eax, 2 (B8 02 00 00 00); int 0x80 (CD 80)
    static const uint8_t exit77_user_code[12] = {
        0xBB, 0x4D, 0x00, 0x00, 0x00,
        0xB8, 0x02, 0x00, 0x00, 0x00,
        0xCD, 0x80
    };
    task_t* exit_task = create_user_task("u_exit77", exit77_user_code, sizeof(exit77_user_code),
                                         USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR);
    test_assert_true(exit_task != 0, "user task Milestone3(create exit77 user task)");
    if (exit_task != 0)
    {
        uint32_t exit_pid = exit_task->pid;
        int32_t status = 0;
        task_add(exit_task);

        bool waited = task_wait_pid(exit_pid, &status);
        test_assert_true(waited, "user task Milestone3(task_wait_pid succeeded)");
        test_assert_equal_int(77, (int)status, "user task Milestone3(SYS_EXIT status 77 retrieved)");
        test_assert_equal_int(used_before, get_used_memory(), "user task Milestone3(PMM frames restored)");
    }

    test_elf_task();
}

void launch_user_test(void)
{
    printf("[User Mode / Syscalls]\n");

    task_t* user_task = create_user_task(
        "user_test",
        user_test_start,
        user_test_size,
        USER_CODE_LOAD_ADDR,
        USER_STACK_TOP_ADDR
    );

    if (user_task == 0)
    {
        printf("[FAIL] Could not create user task\n");
        return;
    }

    task_add(user_task);
    printf("[ OK ] User test task created\n");
}