#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FS_MAX_FILENAME 20u
#define FS_MAX_FILES    15u
#define FS_MAGIC        0x53464D42u // "BMFS" in little-endian
#define FS_SUPERBLOCK_LBA (1u + (uint32_t)KERNEL_MAX_SECTORS)

typedef struct {
    char     name[FS_MAX_FILENAME]; // Null-terminated filename (20 bytes)
    uint32_t start_lba;             // Starting LBA sector on disk (4 bytes)
    uint32_t size;                  // Exact file size in bytes (4 bytes)
    uint32_t reserved;              // Reserved / padding (4 bytes)
} __attribute__((packed)) fs_entry_t; // Total = 32 bytes

typedef struct {
    uint32_t   magic;               // Must be FS_MAGIC (4 bytes)
    uint32_t   file_count;          // Number of valid files (0..15) (4 bytes)
    uint8_t    reserved[24];        // Pad header to 32 bytes
    fs_entry_t entries[FS_MAX_FILES]; // 15 * 32 = 480 bytes
} __attribute__((packed)) fs_superblock_t; // 32 + 480 = 512 bytes!


bool fs_init(void);
const fs_entry_t* fs_find(const char* name);

bool fs_read_file(const char* name, void** out_buffer, uint32_t* out_size);
void fs_list_files(void);