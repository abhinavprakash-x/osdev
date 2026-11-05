# Bare Minimum Filesystem (BMFS)

This is a simple filesystem for the operating system.
boot.img
LBA 0: Boot sector (512 bytes)
LBA 1 -> KENEL_MAX_SECTORS: Kernel sectors (512 bytes each)
LBA KENEL_MAX_SECTORS + 1 : BMFS Superblock (512 bytes)
LBA KENEL_MAX_SECTORS + 2 onwards: BMFS data sectors (512 bytes each) / elf executables