/*
 * Kernel Heap Unit Tests
 * Validates allocation, alignment, block splitting, block reuse, and coalescing.
 */

#include "tests/test_framework.h"
#include "mm/heap.h"
#include "libc/mem.h"
#include "libc/stdlib.h"

void test_heap_suite(void)
{
    printf("[Kernel Heap]\n");

    // 1. Basic Allocation & Overlap
    void *a = kmalloc(32);
    void *b = kmalloc(64);
    test_assert_true(a != 0, "kmalloc(32)");
    test_assert_true(b != 0, "kmalloc(64)");
    test_assert_true(a != b, "kmalloc(unique)");

    memset(a, 0xAA, 32);
    memset(b, 0xBB, 64);
    test_assert_equal_int(0xAA, ((uint8_t*)a)[0], "kmalloc(no overlap)");

    kfree(a);
    kfree(b);

    // 2. Alignment Test
    void *c = kmalloc(3);
    heap_block_t *block_c = (heap_block_t*)((uint8_t*)c - sizeof(heap_block_t));
    test_assert_true(block_c->size >= 8 && (block_c->size % 8 == 0), "heap(8-byte alignment)");

    // 3. Block Splitting Test
    void *d = kmalloc(16);
    heap_block_t *block_d = (heap_block_t*)((uint8_t*)d - sizeof(heap_block_t));
    test_assert_true(block_d->next != 0, "heap(block split exists)");
    test_assert_true(block_d->next->is_free == 1, "heap(split block is free)");

    // 4. Block Reuse Test
    void *e1 = kmalloc(128);
    kfree(e1);
    void *e2 = kmalloc(128);
    test_assert_true(e1 == e2, "heap(block reuse)");
    kfree(e2);

    // 5. Block Coalescing Test
    void *f1 = kmalloc(64);
    void *f2 = kmalloc(64);
    void *f3 = kmalloc(64);

    heap_block_t *block_f1 = (heap_block_t*)((uint8_t*)f1 - sizeof(heap_block_t));
    heap_block_t *block_f2 = (heap_block_t*)((uint8_t*)f2 - sizeof(heap_block_t));

    kfree(f1);
    kfree(f2);

    size_t expected_merged_size = 64 + sizeof(heap_block_t) + 64;
    test_assert_true(block_f1->size >= expected_merged_size, "heap(block coalescing sizes)");
    test_assert_true(block_f1->next == block_f2->next, "heap(linked list merged)");

    kfree(c);
    kfree(d);
    kfree(f3);

    // 6. BUG-018 & BUG-031: Overflow and Ceiling Rejection
    test_assert_true(kmalloc(0) == 0, "heap BUG-018(kmalloc 0 rejected)");
    test_assert_true(kmalloc((size_t)-1) == 0, "heap BUG-018(kmalloc -1 wraparound rejected)");
    test_assert_true(kmalloc(0xFFFFFFF0u) == 0, "heap BUG-018(kmalloc near-UINT_MAX rejected)");
    test_assert_true(kmalloc(HEAP_MAX_SIZE) == 0, "heap BUG-031(kmalloc >= ceiling rejected)");

    // 7. BUG-030: kfree Validation (invalid pointers, interior pointers, double-free)
    void *g = kmalloc(32);
    test_assert_true(g != 0, "heap BUG-030(alloc for kfree validation)");
    if (g != 0)
    {
        heap_block_t *block_g = (heap_block_t*)((uint8_t*)g - sizeof(heap_block_t));

        // Fake a magic header inside the payload and pass an interior pointer
        uint32_t *fake = (uint32_t*)g;
        fake[0] = HEAP_MAGIC;
        fake[1] = 16;
        fake[2] = 0;
        fake[3] = 0;
        kfree((uint8_t*)g + sizeof(heap_block_t));
        test_assert_equal_int(0, block_g->is_free, "heap BUG-030(interior fake block rejected)");

        // Arbitrary non-heap address and unaligned pointer
        kfree((void*)0x00100000);
        kfree((uint8_t*)g + 1);
        test_assert_equal_int(0, block_g->is_free, "heap BUG-030(out-of-bounds/unaligned rejected)");

        // Valid free followed by double free
        kfree(g);
        test_assert_equal_int(1, block_g->is_free, "heap BUG-030(valid kfree marks free)");
        kfree(g);
        test_assert_equal_int(1, block_g->is_free, "heap BUG-030(double kfree safely ignored)");
    }

    printf("Heap tests complete.\n");
}