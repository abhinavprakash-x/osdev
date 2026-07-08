# Bare Minimum OS
A 32-bit x86 Operating System Project. Everything is written from scratch in C and x86 assembly.
![OS Screenshot](docs/vm1.png)

## Requirements:
- binutils
- Cross Compiler (`i686-elf-gcc`)
- NASM
- linker (`ld`)
- QEMU (or other VM Software)

## 📖 Documentation and Setup:
Detailed Setup instructions and Architecture Documentation is stored in `docs/` directory.

* **[Toolchain Setup](docs/toolchain_setup.md):** Step-by-step instructions for downloading, compiling, and installing the required `i686-elf` cross-compiler. **(Start here if you are building for the first time!)**
* **[Architecture & Design](docs/architecture.md):** Details on the boot sequence, physical memory map, interrupt routing (IDT/PIC), and hardware I/O ports.

## 🚀 Usage:
This project uses a standard Makefile for easy building and testing.
```bash
make all        # Build the kernel and create boot.img
make run        # Run the OS in QEMU
make clean      # Delete Build Files
make print      # For Debugging the Makefile
```

## 📂 Folder Structure:
```
osdev/
|----- build/       # Compiled binaries and final boot.img
|----- cross/       # i686-elf-gcc toolchain location
|----- docs/        # Detailed project documentation and images
|----- src/
|       |----- bootloader.asm  # The Custom Bootloader
|       |----- kernel_entry.asm 
|       |----- kernel.c        # Custom C Kernel entry point
|       |----- apps/           # Contains apps for OS (shell, etc.)
|       |----- interrupts/     # Contains code to handle Interrupts (idt, isr, pic)
|       |----- drivers/        # Contains drivers (keyboard, vga, pit, etc.)
|       |----- libc/           # Custom C library for the OS
|       |----- mm/             # Memory Manager
|
|----- linker.ld    # Linker file to link all binaries
|----- Makefile     # Easy Build Commands
|----- README.md    # This file
|----- LICENSE
```

## ✨ Features Added:
- Custom Bootloader
- Custom C Kernel
- Interactive Shell
- Makefile build toolchain
- Interrupt Descriptor Table (IDT)
- Interrupt Service Routines (ISR)
- Programmable Interrupt Controller (8259 PIC)
- Hardware IRQ Handling
- PS/2 Keyboard Input Driver
- VGA Text Buffer Mode Driver
- Programmable Interval Timer (100Hz frequency)
- Custom `libc`
- Physical Memory Manager (Bitmap Allocator)
- Paging (Virtual Memory)
- `malloc()` and `free()` (Kernel Heap)

## Add Next:
- Shell text Colour Change
- Multitasking
- File System
- Ring 3 User Mode
- More Apps
- (Maybe) GUI

## References:
- [OSdev Wiki](https://wiki.osdev.org/)

This project is open-source and licensed under the terms of the `LICENSE` file.