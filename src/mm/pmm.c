#include "pmm.h"

/*
Assume 4 GB Ram
So 4 * 1024 * 1024 = 4194304 Kilo Bytes
Since it is distributed in pages of size 4KB each
Hence 4194304 / 4 = 1048576 blocks (each block of 4KB)
Each block requires 1 bit to say if it's free or not
So, array of 1048576 size is required, but one element
can store 32 bits (if array type uint32_t) So, 1048576 / 32 = 32768
*/
uint32_t mem_map[32768];

void pmm_init()
{
    int entry_count = *(uint32_t*)0x5000;

    for(int i = 0; i < 32768; ++i)
        mem_map[i] = 0xFFFFFFFF;

    struct mem_map_entry* mmap = (struct mem_map_entry*)0x5004;
    for(uint32_t i = 0; i < entry_count; ++i)
    {
        if(mmap[i].region_type == 1)
        {
            uint32_t start_frame = mmap[i].base_addr / 4096;
            uint32_t num_frames = mmap[i].region_length / 4096;
            for(uint32_t j = 0; j < num_frames; ++j) clear_bit(start_frame + j);
        }
    }

    //Protect the Kernel (mark starting 1 MegaByte as used (for kernel and vga memory etc.))
    for(uint32_t i = 0; i < 256; ++i)
        set_bit(i);
}

void set_bit(int bit)
{
    int idx = bit / 32;
    int bit_offset = bit % 32;
    mem_map[idx] = mem_map[idx] | (1 << bit_offset);
}

void clear_bit(int bit)
{
    int idx = bit / 32;
    int bit_offset = bit % 32;
    mem_map[idx] &= ~(1 << bit_offset);
}

int test_bit(int bit)
{
    int idx = bit / 32;
    int bit_offset = bit % 32;
    return (mem_map[idx] & (1 << bit_offset)) != 0;
}