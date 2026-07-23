# Memory Management

Bare Minimum OS implements physical memory management, virtual memory
through x86 paging, and a dynamically growing kernel heap.

The memory system is divided into three layers:

```text
                 Kernel
                   |
            kmalloc / kfree
                   |
                   v
              Kernel Heap
                   |
              Virtual Pages
                   |
                   v
                 Paging
                   |
             Physical Frames
                   |
                   v
       Physical Memory Manager
                   |
                   v
                  RAM
```

## BIOS Memory Map

During boot, the BIOS E820 interface is used to obtain the physical
memory map.

The bootloader stores the memory map so that the kernel can determine
which regions of physical memory are usable and which are reserved.

The Physical Memory Manager is initialized using this information.

## Physical Memory Manager

Physical memory is divided into 4 KiB frames.

The Physical Memory Manager (PMM) tracks these frames using a bitmap,
where each bit represents the allocation state of one physical frame.

```text
Physical Memory

+--------+--------+--------+--------+--------+
| Frame  | Frame  | Frame  | Frame  | Frame  |
|   0    |   1    |   2    |   3    |  ...   |
+--------+--------+--------+--------+--------+
    |        |        |        |
    v        v        v        v

Bitmap:
   1        1        0        1       ...

1 = unavailable / allocated
0 = available
```

The PMM provides operations for allocating and freeing physical frames.

A next-fit allocation strategy is used. Instead of beginning every
search at the start of the bitmap, allocation continues from the
location of the previous search.

## Paging

The kernel uses the standard 32-bit x86 two-level paging structure.

A 32-bit virtual address is divided into:

```text
31                 22 21                 12 11              0
+--------------------+---------------------+------------------+
| Page Directory     | Page Table          | Offset           |
| Index (10 bits)    | Index (10 bits)     | (12 bits)        |
+--------------------+---------------------+------------------+
```

Each page is 4 KiB.

The Page Directory contains up to 1024 Page Directory Entries, with
each entry referring to a Page Table containing up to 1024 Page Table
Entries.

This allows the 32-bit virtual address space to map up to 4 GiB.

The paging subsystem supports:

- Mapping virtual pages to physical frames
- Unmapping pages
- Translating virtual addresses to physical addresses
- Allocating frames for mappings
- Page fault handling

## Recursive Page Directory Mapping

The kernel uses recursive page-directory mapping to make the active page
directory and its page tables accessible through virtual addresses.

One Page Directory Entry points back to the Page Directory itself.

Conceptually:

```text
Page Directory
     |
     +----> Page Table
     |
     +----> Page Table
     |
     ...
     |
     +----> Page Directory
             (recursive)
```

This allows the kernel to manipulate paging structures without requiring
separate temporary mappings for each page table.

## Kernel Heap

The kernel heap provides dynamic memory allocation through:

```c
kmalloc(size);
kfree(ptr);
```

Heap allocations are stored in blocks containing allocation metadata.

When an allocation is requested, the heap searches for a sufficiently
large free block.

If a block is larger than required, it may be split:

```text
Before:

+-------------------------------+
|          Free Block           |
+-------------------------------+

After allocation:

+----------------+--------------+
|     Used       |     Free     |
+----------------+--------------+
```

When adjacent blocks are freed, they are coalesced to reduce
fragmentation:

```text
+---------+---------+
|  Free   |  Free   |
+---------+---------+

          |
          v

+-------------------+
|       Free        |
+-------------------+
```

If the heap does not contain enough space for an allocation, it can
grow by requesting additional pages and physical frames from the lower
memory-management layers.

## Memory Management Tests

The kernel test suite contains tests for the memory-management
subsystems, including:

- PMM allocation and freeing
- Physical frame uniqueness and alignment
- Page mapping
- Address translation
- Page unmapping
- Heap allocation and freeing
- Heap splitting and coalescing

These tests can be run through the kernel shell.