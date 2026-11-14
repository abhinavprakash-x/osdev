#!/usr/bin/env python3
import os
import struct
import sys

FS_MAGIC = 0x53464D42  # "BMFS"
FS_MAX_FILENAME = 20
FS_MAX_FILES = 15
SECTOR_SIZE = 512

FS_TYPE_FILE = 1
FS_ROOT_DIR = 0xFFFF

def main():
    if len(sys.argv) < 3:
        print("Usage: mkfs_bmfs.py <output_fs.img> <kernel_max_sectors> [files...]")
        sys.exit(1)

    out_path = sys.argv[1]
    kernel_max_sectors = int(sys.argv[2])
    files = sys.argv[3:]

    if len(files) > FS_MAX_FILES:
        print(f"Error: Too many files (max {FS_MAX_FILES})")
        sys.exit(1)

    superblock_lba = 1 + kernel_max_sectors
    current_lba = superblock_lba + 1

    entries_blob = bytearray()
    data_blob = bytearray()

    for path in files:
        name = os.path.basename(path)
        name_bytes = name.encode("ascii")[:FS_MAX_FILENAME - 1]
        name_padded = name_bytes.ljust(FS_MAX_FILENAME, b"\x00")

        with open(path, "rb") as f:
            content = f.read()

        size = len(content)
        sectors = (size + SECTOR_SIZE - 1) // SECTOR_SIZE if size > 0 else 1
        padded_content = content.ljust(sectors * SECTOR_SIZE, b"\x00")

        # <20sIIHBB = name(20B), start_lba(4B), size(4B), parent(2B), type(1B), reserved(1B)
        entry = struct.pack("<20sIIHBB", name_padded, current_lba, size, FS_ROOT_DIR, FS_TYPE_FILE, 0)
        entries_blob.extend(entry)
        data_blob.extend(padded_content)

        current_lba += sectors

    remaining = FS_MAX_FILES - len(files)
    entries_blob.extend(b"\x00" * (remaining * 32))

    header = struct.pack("<II24s", FS_MAGIC, len(files), b"\x00" * 24)
    superblock = header + entries_blob
    assert len(superblock) == SECTOR_SIZE

    with open(out_path, "wb") as f:
        f.write(superblock)
        f.write(data_blob)

if __name__ == "__main__":
    main()