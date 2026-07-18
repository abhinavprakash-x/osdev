/*
 * Physical Memory Manager (PMM)
 * Manages physical RAM via a Bitmap. Keeps track of which 4KB frames 
 * are free and which are in use by the kernel or hardware.
 */

#include "pmm.h"
#include "../libc/mem.h"

// Architecture limits based on a 32-bit OS (Maximum 4GB RAM)
#define PMM_BLOCK_SIZE     4096
#define PMM_MAX_BLOCKS     1048576               // 4GB / 4KB
#define PMM_BITMAP_SIZE    (PMM_MAX_BLOCKS / 32) // 32768 uint32_t entries

// E820 Memory Map Constants
#define E820_MEM_MAP_ADDR  0x5000
#define E820_USABLE        1

static uint32_t mem_map[32768];
static int used_blocks = 0;
static int last_searched_idx = 0;
static int total_blocks = 0;

static void set_bit(int bit)
{
    int idx = bit / 32;
    int bit_offset = bit % 32;
    mem_map[idx] = mem_map[idx] | (1 << bit_offset);
}

static void clear_bit(int bit)
{
    int idx = bit / 32;
    int bit_offset = bit % 32;
    mem_map[idx] &= ~(1 << bit_offset);
}

static int test_bit(int bit)
{
    int idx = bit / 32;
    int bit_offset = bit % 32;
    return (mem_map[idx] & (1 << bit_offset)) != 0;
}

void pmm_init(void)
{
    uint32_t entry_count = *(uint32_t*)E820_MEM_MAP_ADDR;
    last_searched_idx = 0;
    total_blocks = 0;
    used_blocks = 0;

    // Assume All Memory is in use
    memset(mem_map, 0xFF, sizeof(mem_map));

    struct mem_map_entry* mmap = (struct mem_map_entry*)(E820_MEM_MAP_ADDR + 4);

    for(uint32_t i = 0; i < entry_count; ++i)
    {
        // Region Type 1 means standard, usable RAM
        if(mmap[i].region_type == E820_USABLE)
        {
            uint32_t start_frame = mmap[i].base_addr / PMM_BLOCK_SIZE;
            uint32_t num_frames = mmap[i].region_length / PMM_BLOCK_SIZE;

            total_blocks += num_frames;
            // Mark these specific frames as available
            for(uint32_t j = 0; j < num_frames; ++j) clear_bit(start_frame + j);
        }
    }

    // Protect the first 1MB of memory (0x0 to 0xFFFFF).
    // This is strictly reserved for the BIOS, VGA Buffer, and Kernel code.
    // 1MB / 4KB = 256 blocks.
    for(uint32_t i = 0; i < 256; ++i) {
        set_bit(i);
        used_blocks++;
    }
}

void* pmm_alloc_block(void)
{
    // Scan the bitmap looking for a free block
    for(int i = 0; i < PMM_BITMAP_SIZE; ++i)
    {
        int idx = (last_searched_idx + i) % PMM_BITMAP_SIZE;
        // If entire 32-bit chunk is occupied skip it
        if(mem_map[idx] == 0xFFFFFFFF) continue;
        
        // If any one bit is free (0). Find which one and Claim it
        for(int j = 0; j < 32; ++j)
        {
            int bit = (idx * 32) + j;
            if(test_bit(bit) == 0)
            {
                set_bit(bit);
                last_searched_idx = idx;
                uint32_t physical_addr = bit * 4096;
                used_blocks++;
                return (void*)physical_addr;
            }
        }
    }

    // Kernel Panic condition: We are entirely out of physical RAM.
    return 0;
}

void pmm_free_block(void* physical_addr)
{
    uint32_t bit = (uint32_t)physical_addr / PMM_BLOCK_SIZE;
    clear_bit(bit);
    used_blocks--;
}

int get_used_memory(void)
{
    return used_blocks;
}

int get_total_memory(void)
{
    return total_blocks;
}