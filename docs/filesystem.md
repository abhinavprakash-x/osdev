# Bare Minimum Filesystem & ATA PIO Driver

Bare Minimum OS includes a 28-bit LBA ATA PIO disk driver (`src/drivers/ata.c`) and a persistent, hierarchical on-disk filesystem (**BMFS**, `src/drivers/fs.c`).

## Disk Image Layout (`build/boot.img`)

The `Makefile` constructs a 4 MiB raw disk image (`boot.img`) with the following sector layout (512 bytes per LBA sector):

| LBA Sector(s) | Size | Contents |
|---|---|---|
| `0` | 512 B (1 sector) | `bootloader.bin` (Master Boot Record / 16-bit bootloader) |
| `1 .. 150` | 75 KiB (`KERNEL_MAX_SECTORS = 150`) | `kernel.bin` (padded to exactly 150 sectors) |
| `151` (`FS_SUPERBLOCK_LBA`) | 512 B (1 sector) | **BMFS Superblock & Directory Table** |
| `152 .. 8191` | ~3.92 MiB | **BMFS Data Sectors** (ELF executables, text files, and dynamically created files) |

## ATA PIO Driver (`include/drivers/ata.h`, `src/drivers/ata.c`)

The ATA driver communicates with the primary ATA bus (`0x1F0–0x1F7`) in 28-bit LBA Programmed I/O (PIO) mode:

- `ata_read_sectors(uint32_t lba, uint8_t count, void* buffer)`:
  - Selects Master drive (`0xE0 | ((lba >> 24) & 0x0F)`), programs sector count and LBA bits `0–23`, and issues `0x20` (`ATA_CMD_READ_PIO`).
  - Waits 400 ns (4 status reads), polls until `BSY == 0` and `DRQ == 1` (checking `ERR` and `DF`), and reads 256 16-bit words (`inw(0x1F0)`) per sector.
- `ata_write_sectors(uint32_t lba, uint8_t count, const void* buffer)`:
  - Programs LBA/count, issues `0x30` (`ATA_CMD_WRITE_PIO`), polls `BSY == 0` and `DRQ == 1`, writes 256 words (`outw(0x1F0)`) per sector, and issues `0xE7` (`ATA_CMD_CACHE_FLUSH`) to persist data to disk.
- Both operations save `EFLAGS` and disable interrupts (`cli`) so timer preemption cannot interrupt a multi-word PIO transfer.

## BMFS On-Disk Format (`include/drivers/fs.h`)

The entire BMFS superblock and directory table fit inside a single 512-byte sector at `FS_SUPERBLOCK_LBA` (`151`):

```c
#define FS_MAX_FILENAME   20u
#define FS_MAX_FILES      15u
#define FS_MAGIC          0x53464D42u // "BMFS" in little-endian
#define FS_SUPERBLOCK_LBA (1u + (uint32_t)KERNEL_MAX_SECTORS)

#define FS_TYPE_FREE      0u
#define FS_TYPE_FILE      1u
#define FS_TYPE_DIR       2u
#define FS_ROOT_DIR       0xFFFFu

typedef struct {
    char     name[FS_MAX_FILENAME]; // Null-terminated name (20 bytes)
    uint32_t start_lba;             // Starting LBA (0 for directories) (4 bytes)
    uint32_t size;                  // Exact file size in bytes (0 for directories) (4 bytes)
    uint16_t parent;                // Parent slot index (0..14), or FS_ROOT_DIR (0xFFFF) (2 bytes)
    uint8_t  type;                  // FS_TYPE_FREE (0), FS_TYPE_FILE (1), FS_TYPE_DIR (2) (1 byte)
    uint8_t  reserved;              // Padding (1 byte)
} __attribute__((packed)) fs_entry_t; // 32 bytes

typedef struct {
    uint32_t   magic;                 // FS_MAGIC (0x53464D42) (4 bytes)
    uint32_t   file_count;            // Number of active entries (0..15) (4 bytes)
    uint8_t    reserved[24];          // Reserved header padding (24 bytes)
    fs_entry_t entries[FS_MAX_FILES]; // 15 * 32 = 480 bytes
} __attribute__((packed)) fs_superblock_t; // 32 + 480 = 512 bytes
```

### Key Filesystem Features

1. **Inode Slot Stability:** Deleting a file (`fs_delete_file`) marks its slot as `FS_TYPE_FREE` (`0`) without shifting other entries, preserving parent-directory slot indices for subdirectories.
2. **Hierarchical Path Resolution (`fs_resolve_path`):** Supports absolute paths (`/sub/note.txt`), relative paths (`sub/note.txt`), current directory (`.`), and parent directory (`..`).
3. **First-Fit Sector Allocation (`fs_alloc_lba`):** Scans from `FS_SUPERBLOCK_LBA + 1` (`152`) upward to find the first contiguous range of sectors that does not overlap any active `FS_TYPE_FILE` entry, reclaiming space left by deleted files.
4. **Build-Time Packing (`mkfs_bmfs.py`):** Automatically packs all compiled `build/user/*.elf` binaries and `user/*.txt` files into `build/fs.img` as root-level (`parent = 0xFFFF`, `type = 1`) files during `make`.

## Shell Filesystem Commands

| Command | Description |
|---|---|
| `ls` | List files and directories inside the current working directory |
| `pwd` | Print the full path of the current working directory |
| `cd <path>` | Change the current working directory (supports `/`, `..`, and relative/absolute paths) |
| `mkdir <path>` | Create a new directory on disk and persist the superblock |
| `cat <path>` | Read a file from disk and print its contents |
| `write <path> <text>` | Create or overwrite a file on disk and persist changes across reboots |
| `rm <path>` | Delete a file or an empty directory from disk |
| `run <path> [&]` | Load and execute a Ring 3 ELF32 binary (waits in foreground by default, or runs in background with `&`) |