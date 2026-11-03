#include "apps/kernel_tests.h"
#include "tests/test_framework.h"
#include "drivers/ata.h"
#include <stdint.h>

void test_fs(void)
{
    uint8_t sector0[512];
    test_assert_true(ata_read_sectors(0, 1, sector0), "ata(read boot sector LBA 0)");
    test_assert_true(sector0[510] == 0x55 && sector0[511] == 0xAA, "ata(boot signature 0xAA55 verified)");
}