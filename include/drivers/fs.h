#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef KERNEL_MAX_SECTORS
#define KERNEL_MAX_SECTORS 150
#endif

#ifndef STAGE2_SECTORS
#define STAGE2_SECTORS 4
#endif

#define FS_MAX_FILENAME   20u
#define FS_MAX_FILES      15u
#define FS_MAGIC          0x53464D42u // "BMFS" in little-endian
#define FS_SUPERBLOCK_LBA (1u + (uint32_t)STAGE2_SECTORS + (uint32_t)KERNEL_MAX_SECTORS)

#define FS_TYPE_FREE      0u
#define FS_TYPE_FILE      1u
#define FS_TYPE_DIR       2u

#define FS_ROOT_DIR       0xFFFFu

typedef struct {
    char     name[FS_MAX_FILENAME]; // Null-terminated name (20 bytes)
    uint32_t start_lba;             // Starting LBA (0 for directories) (4 bytes)
    uint32_t size;                  // Size in bytes (0 for directories) (4 bytes)
    uint16_t parent;                // Parent slot index, or FS_ROOT_DIR (0xFFFF) (2 bytes)
    uint8_t  type;                  // FS_TYPE_FREE (0), FS_TYPE_FILE (1), FS_TYPE_DIR (2) (1 byte)
    uint8_t  reserved;              // Padding (1 byte)
} __attribute__((packed)) fs_entry_t; // 32 bytes

typedef struct {
    uint32_t   magic;                 // Must be FS_MAGIC (4 bytes)
    uint32_t   file_count;            // Number of active entries (0..15) (4 bytes)
    uint8_t    reserved[24];          // Pad header to 32 bytes
    fs_entry_t entries[FS_MAX_FILES]; // 15 * 32 = 480 bytes
} __attribute__((packed)) fs_superblock_t; // 512 bytes

bool fs_init(void);
const fs_entry_t* fs_find(const char* path);

bool fs_read_file(const char* path, void** out_buffer, uint32_t* out_size);
void fs_list_files(void);

bool fs_write_file(const char* path, const void* data, uint32_t size);
bool fs_delete_file(const char* path);

bool fs_mkdir(const char* path);
bool fs_chdir(const char* path);
void fs_get_pwd(char* buf, size_t max_len);