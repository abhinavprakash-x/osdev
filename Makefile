# ============================================================
# Toolchain
# ============================================================

ASM   ?= nasm
CROSS ?= ./cross/bin/i686-elf-

CC      := $(CROSS)gcc
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
QEMU    ?= qemu-system-i386


# ============================================================
# Directories & Outputs
# ============================================================

SRC_DIR     := src
INCLUDE_DIR := include
USER_DIR    := user
BUILD_DIR   := build

BOOTLOADER  := $(SRC_DIR)/bootloader.asm
LINKER      := linker.ld
USER_LINKER := $(USER_DIR)/linker.ld
MKFS_SCRIPT := mkfs_bmfs.py

BOOT_BIN    := $(BUILD_DIR)/bootloader.bin
KERNEL_BIN  := $(BUILD_DIR)/kernel.bin
FS_IMAGE    := $(BUILD_DIR)/fs.img
BOOT_IMAGE  := $(BUILD_DIR)/boot.img

KERNEL_MAX_SECTORS := 150
KERNEL_MAX_SIZE    := $(shell expr $(KERNEL_MAX_SECTORS) \* 512)


# ============================================================
# Compiler / Assembler / Linker flags
# ============================================================

# CFLAGS
#
# -m32                         Build 32-bit code
# -ffreestanding               Build for a freestanding environment
# -std=gnu11                   Use GNU C11
# -fno-pic                     No position-independent code
# -fno-pie                     No position-independent executable
# -fno-stack-protector         Kernel has no normal userspace SSP runtime
# -fno-builtin                 Use our own libc functions rather than
#                              compiler builtins where appropriate
#
# Warnings:
# -Wall                        Basic warnings
# -Wextra                      Extra useful warnings
# -Wshadow                     Warn about variable shadowing
# -Wundef                      Warn about undefined macros in #if
#
# Errors:
# -Werror=implicit-function-declaration
#                              Missing function declarations are errors
# -Werror=return-type          Missing return statements are errors
#
# Dependencies:
# -MMD -MP                    Generate .d dependency files
#
CFLAGS := \
	-m32 \
	-ffreestanding \
	-std=gnu11 \
	-fno-pic \
	-fno-pie \
	-fno-stack-protector \
	-fno-builtin \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wundef \
	-Werror=implicit-function-declaration \
	-Werror=return-type \
	-MMD \
	-MP \
	-I$(INCLUDE_DIR) \
	-DKERNEL_MAX_SECTORS=$(KERNEL_MAX_SECTORS)

# Userspace C flags (use user/ headers instead of kernel include/)
USER_CFLAGS := $(filter-out -I$(INCLUDE_DIR),$(CFLAGS)) -I$(USER_DIR)

# NASM flags for normal kernel assembly
ASFLAGS := -f elf32

# NASM flags for the bootloader
BOOT_ASFLAGS := -f bin -DKERNEL_MAX_SECTORS=$(KERNEL_MAX_SECTORS)

# Kernel linker flags (flat binary)
LDFLAGS := \
	-m elf_i386 \
	-T $(LINKER) \
	--oformat binary

# Userspace linker flags (ELF32 executable)
USER_LDFLAGS := \
	-m elf_i386 \
	-T $(USER_LINKER) \
	-z max-page-size=0x1000 \
	-s


# ============================================================
# Source discovery
# ============================================================

# Find all kernel C files recursively under src/
C_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.c' | sort)

# Find all kernel assembly files recursively under src/
ASM_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.asm' | sort)

# The bootloader is assembled directly into a raw binary.
# Everything else is assembled into ELF32 object files.
ASM_KERNEL := $(filter-out $(BOOTLOADER),$(ASM_SOURCES))

# Automatically discover userspace library, C apps, and text/data files
USER_LIB_SRC     := $(USER_DIR)/ulib.c
USER_LIB_OBJ     := $(BUILD_DIR)/user/ulib.o

USER_APP_SOURCES := $(filter-out $(USER_LIB_SRC),$(shell find $(USER_DIR) -maxdepth 1 -type f -name '*.c' | sort))
USER_APP_OBJECTS := $(patsubst $(USER_DIR)/%.c,$(BUILD_DIR)/user/%.o,$(USER_APP_SOURCES))
USER_ELFS        := $(patsubst $(USER_DIR)/%.c,$(BUILD_DIR)/user/%.elf,$(USER_APP_SOURCES))
USER_DATA_FILES  := $(shell find $(USER_DIR) -maxdepth 1 -type f -name '*.txt' | sort)

FS_FILES         := $(USER_ELFS) $(USER_DATA_FILES)


# ============================================================
# Object mapping
# ============================================================

# Example:
#
# src/mm/pmm.c
#      ↓
# build/mm/pmm.o
#
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

# Example:
#
# src/task/switch.asm
#      ↓
# build/task/switch.o
#
ASM_OBJECTS := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_KERNEL))

OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

# Automatically generated dependency files from GCC (kernel + userspace)
DEP_FILES := $(C_OBJECTS:.o=.d) $(USER_LIB_OBJ:.o=.d) $(USER_APP_OBJECTS:.o=.d)


# ============================================================
# Default target
# ============================================================

.PHONY: all
all: $(BOOT_IMAGE)


# ============================================================
# Bootloader
# ============================================================

$(BOOT_BIN): $(BOOTLOADER) Makefile
	@mkdir -p $(dir $@)
	$(ASM) $(BOOT_ASFLAGS) $< -o $@


# ============================================================
# Kernel C compilation
# ============================================================

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# ============================================================
# Kernel assembly
# ============================================================

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASFLAGS) $< -o $@


# ============================================================
# Kernel linking
# ============================================================

$(KERNEL_BIN): $(OBJECTS) $(LINKER)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)


# ============================================================
# Userspace ELF32 applications & BMFS image
# ============================================================

$(BUILD_DIR)/user/%.o: $(USER_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/%.elf: $(BUILD_DIR)/user/%.o $(USER_LIB_OBJ) $(USER_LINKER)
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) -o $@ $(USER_LIB_OBJ) $<

# Shortcut target: `make hello.elf` builds `build/user/hello.elf`
%.elf: $(BUILD_DIR)/user/%.elf
	@echo "Built $<"

$(FS_IMAGE): $(FS_FILES) $(MKFS_SCRIPT) Makefile
	@mkdir -p $(dir $@)
	python3 $(MKFS_SCRIPT) $@ $(KERNEL_MAX_SECTORS) $(FS_FILES)


# ============================================================
# Final boot image
# ============================================================

$(BOOT_IMAGE): $(BOOT_BIN) $(KERNEL_BIN) $(FS_IMAGE)
	@mkdir -p $(dir $@)
	@if [ "$$(stat -c %s $(KERNEL_BIN))" -gt $(KERNEL_MAX_SIZE) ]; then \
		echo "Error: kernel.bin exceeds $(KERNEL_MAX_SECTORS) Sectors!"; \
		exit 1; \
	fi
	truncate -s $(KERNEL_MAX_SIZE) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) $(FS_IMAGE) > $@


# ============================================================
# Run
# ============================================================

.PHONY: run
run: all
	$(QEMU) -drive format=raw,file=$(BOOT_IMAGE)


# ============================================================
# Debug
# ============================================================

.PHONY: debug
debug: all
	$(QEMU) \
		-drive format=raw,file=$(BOOT_IMAGE) \
		-d int \
		-D qemu-int.log


# ============================================================
# Build check
# ============================================================

# Compile kernel objects and link userspace ELFs without building
# the final disk image.

.PHONY: check
check: $(OBJECTS) $(USER_ELFS)
	@echo "Build check passed."


# ============================================================
# Print build information
# ============================================================

.PHONY: print
print:
	@echo "CC          = $(CC)"
	@echo "ASM         = $(ASM)"
	@echo "LD          = $(LD)"
	@echo "OBJCOPY     = $(OBJCOPY)"
	@echo "QEMU        = $(QEMU)"
	@echo
	@echo "SRC_DIR     = $(SRC_DIR)"
	@echo "INCLUDE_DIR = $(INCLUDE_DIR)"
	@echo "USER_DIR    = $(USER_DIR)"
	@echo "BUILD_DIR   = $(BUILD_DIR)"
	@echo
	@echo "C_SOURCES   ="
	@$(foreach f,$(C_SOURCES),echo "  $(f)";)
	@echo
	@echo "ASM_SOURCES ="
	@$(foreach f,$(ASM_SOURCES),echo "  $(f)";)
	@echo
	@echo "OBJECTS     ="
	@$(foreach f,$(OBJECTS),echo "  $(f)";)
	@echo
	@echo "USER_ELFS   ="
	@$(foreach f,$(USER_ELFS),echo "  $(f)";)
	@echo
	@echo "FS_FILES    ="
	@$(foreach f,$(FS_FILES),echo "  $(f)";)


# ============================================================
# Clean
# ============================================================

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)


# ============================================================
# Rebuild
# ============================================================

.PHONY: rebuild
rebuild: clean all


# ============================================================
# Help
# ============================================================

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make            Build boot image (bootloader + kernel + BMFS)"
	@echo "  make all        Build boot image"
	@echo "  make <app>.elf  Build a single userspace ELF (e.g. make hello.elf)"
	@echo "  make check      Compile all kernel & userspace sources"
	@echo "  make run        Build and run in QEMU"
	@echo "  make debug      Build and run QEMU with interrupt logging"
	@echo "  make print      Print discovered sources, objects, and BMFS files"
	@echo "  make clean      Remove build directory"
	@echo "  make rebuild    Clean and rebuild everything"
	@echo "  make help       Show this help"


# ============================================================
# Include generated dependency files
# ============================================================

-include $(DEP_FILES)