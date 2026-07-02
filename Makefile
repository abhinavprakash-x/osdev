ASM = nasm
CC  = i686-elf-gcc
LD  = ld

SRC_DIR = src
BUILD_DIR = build

# Flags
CFLAGS = -m32 -ffreestanding -fno-pic -Wall -Wextra -I$(SRC_DIR)
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

# --------------------------
# Source discovery
# --------------------------

# C files
C_SOURCES := $(shell find $(SRC_DIR) -type f -name "*.c")

# ASM files
ASM_ALL := $(shell find $(SRC_DIR) -type f -name "*.asm")

# Bootloader (special case)
BOOTLOADER := $(SRC_DIR)/bootloader.asm

# Kernel ASM (exclude bootloader)
ASM_KERNEL := $(filter-out $(BOOTLOADER), $(ASM_ALL))

# --------------------------
# Object mapping
# --------------------------

C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_KERNEL))

OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

# --------------------------
# Targets
# --------------------------

all: $(BUILD_DIR)/boot.img

# --------------------------
# Bootloader (raw binary)
# --------------------------
$(BUILD_DIR)/bootloader.bin: $(BOOTLOADER)
	mkdir -p $(BUILD_DIR)
	$(ASM) -f bin $< -o $@

# --------------------------
# Compile C
# --------------------------
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# --------------------------
# Compile ASM (ELF)
# --------------------------
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	mkdir -p $(dir $@)
	$(ASM) -f elf32 $< -o $@

# --------------------------
# Link kernel
# --------------------------
$(BUILD_DIR)/kernel.bin: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# --------------------------
# Final image
# --------------------------
$(BUILD_DIR)/boot.img: $(BUILD_DIR)/bootloader.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $@
	dd if=/dev/zero bs=512 count=30 >> $@

# --------------------------
# Run
# --------------------------
run: all
	qemu-system-i386 -drive format=raw,file=$(BUILD_DIR)/boot.img

# --------------------------
# Debug (VERY useful)
# --------------------------
print:
	@echo "C_SOURCES = $(C_SOURCES)"
	@echo "ASM_ALL   = $(ASM_ALL)"
	@echo "ASM_KERNEL= $(ASM_KERNEL)"
	@echo "OBJECTS   = $(OBJECTS)"

# --------------------------
# Clean
# --------------------------
clean:
	rm -rf $(BUILD_DIR)