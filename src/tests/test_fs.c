#include "apps/kernel_tests.h"
#include "tests/test_framework.h"
#include "drivers/ata.h"
#include "drivers/fs.h"
#include "task/elf.h"
#include "task/scheduler.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "libc/stdlib.h"
#include "libc/string.h"
#include <stdint.h>

void test_fs(void)
{
    printf("[ATA & BMFS Filesystem]\n");

    uint8_t sector0[512];
    test_assert_true(ata_read_sectors(0, 1, sector0), "ata(read boot sector LBA 0)");
    test_assert_true(sector0[510] == 0x55 && sector0[511] == 0xAA, "ata(boot signature 0xAA55 verified)");

    test_assert_true(fs_init(), "bmfs(fs_init mounts superblock)");
    test_assert_true(fs_find("hello.elf") != 0, "bmfs(fs_find locates hello.elf)");
    test_assert_true(fs_find("fib.elf") != 0, "bmfs(fs_find locates fib.elf)");
    test_assert_true(fs_find("test.txt") != 0, "bmfs(fs_find locates test.txt)");
    test_assert_true(fs_find("nonexistent.elf") == 0, "bmfs(fs_find rejects missing file)");

    // Test directory creation, chdir, pwd, file write, readback, and cleanup
    test_assert_true(fs_mkdir("sub"), "bmfs(fs_mkdir creates /sub)");
    test_assert_true(!fs_mkdir("sub"), "bmfs(fs_mkdir rejects duplicate /sub)");
    test_assert_true(fs_chdir("sub"), "bmfs(fs_chdir enters /sub)");

    char cwd[64];
    fs_get_pwd(cwd, sizeof(cwd));
    test_assert_true(strcmp(cwd, "/sub") == 0, "bmfs(fs_get_pwd returns /sub)");

    const char* msg = "BMFS v2 test payload";
    test_assert_true(fs_write_file("note.txt", msg, (uint32_t)strlen(msg) + 1u), "bmfs(fs_write_file creates /sub/note.txt)");

    void* read_buf = 0;
    uint32_t read_sz = 0;
    test_assert_true(fs_read_file("/sub/note.txt", &read_buf, &read_sz), "bmfs(fs_read_file resolves /sub/note.txt)");
    if (read_buf != 0)
    {
        test_assert_true(strcmp((const char*)read_buf, msg) == 0, "bmfs(written payload matches on readback)");
        kfree(read_buf);
    }

    test_assert_true(fs_chdir(".."), "bmfs(fs_chdir .. returns to /)");
    test_assert_true(!fs_delete_file("sub"), "bmfs(fs_delete_file rejects non-empty directory)");
    test_assert_true(fs_delete_file("sub/note.txt"), "bmfs(fs_delete_file removes sub/note.txt)");
    test_assert_true(fs_delete_file("sub"), "bmfs(fs_delete_file removes empty /sub)");

    int used_before = get_used_memory();

    void* elf_buf = 0;
    uint32_t elf_size = 0;
    bool read_ok = fs_read_file("fib.elf", &elf_buf, &elf_size);
    test_assert_true(read_ok && elf_buf != 0 && elf_size > 0, "bmfs(fs_read_file reads fib.elf from disk)");

    if (read_ok && elf_buf != 0)
    {
        const fs_entry_t* entry = fs_find("fib.elf");
        task_t* task = create_user_task_elf(entry->name, elf_buf, elf_size);
        kfree(elf_buf);

        test_assert_true(task != 0, "bmfs+elf(create_user_task_elf loads fib.elf)");
        if (task != 0)
        {
            uint32_t pid = task->pid;
            int32_t status = 0;
            task_add(task);

            bool waited = task_wait_pid(pid, &status);
            test_assert_true(waited, "bmfs+elf(task_wait_pid on disk ELF)");
            test_assert_equal_int(12, (int)status, "bmfs+elf(fib.elf returned exit status 12)");
        }
    }

    test_assert_equal_int(used_before, get_used_memory(), "bmfs+elf(PMM frames restored after disk ELF run)");
}