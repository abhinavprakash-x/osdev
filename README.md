# Bare Minimum OS
32 bit x86 Operating System Project.
Everything written from Scratch in C and x86 assembly.

## Requirements:
- binutils
- Cross Compiler (elf-i386 gcc)
- NASM
- linker
- qemu (or other VM Software)

## Setup:

### 1. Download Tools
```bash
sudo apt update
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo wget git nasm qemu-system-x86

wget https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.gz
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz
```

### 2. Extract and Build
```bash
tar -xvf binutils-2.41.tar.gz
tar -xvf gcc-13.2.0.tar.gz
mkdir build-binutils
mkdir build-gcc
```

```bash
cd ~/osdev/build-binutils
../binutils-2.41/configure \
  --target=i686-elf --prefix="$HOME/osdev/cross" \
  --disable-nls --disable-werror
make -j$(nproc)
make install

cd ~/osdev/build-gcc
../gcc-13.2.0/configure \
  --target=i686-elf --prefix="$HOME/osdev/cross" \
  --disable-nls --enable-languages=c --without-headers
make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
```

```bash
make install-gcc
make install-target-libgcc
```

### 3. Add to Path
```bash
echo 'export PATH="$HOME/osdev/cross/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

## Usage:
```bash
make all    -> Build
make run    -> Run in Qemu
make clean  -> Delete Build Files
make print  -> For Debug
```

## Folder Structure:
```
osdev/
|----- build/ compiled binaries
|----- cross/ i686-elf-gcc toolchain
|----- src/
|       |----- bootloader.asm The Custom Bootloader
|       |----- kernel_entry.asm
|       |----- kernel.c Custom C Kernel
|       |----- apps/ Contains apps for OS (shell, etc.)
|       |----- core/ Contains core OS code (interrupts, etc.)
|       |----- drivers/ Contains drivers (keyboard, vga, pit, etc.)
|       |----- libc/ Custom C library for OS
|       |----- mm/ Memory Manager
|
|----- linker.ld Linker file to link all binaries
|----- Makefile Easy Build Commands
|----- README.md Documentation
|----- LICENSE
```

## Memory Mapping:
```bash
0x0000 BIOS
0x5000 Memory Mapping (E820 interrupt)
0x7C00 Bootloader
0x8000 OS Code (Kernel, Memory Manager, Shell, etc.)
0x90000 Interrupt Descriptor Table entries (IDT)
0xA0000 Stack (grows downwards)
0xB8000 VGA Text Mode Buffer
```

## Interrupts:
ISR0 (DIvide by Zero)
...
(standard)
...
IRQ0 (PIT) -> ISR 32
IRQ1 (PS/2 Keyboard) -> ISR 33

## Ports:
- `0x20`, `0x21` Command and Data Port for Master PIC
- `0xA0`, `0xA1` Command and Data Port for Slave PIC
- `0x40`, `0x43` Data Channel 0 and Command Register Port for PIT
- `0x60` PS/2 Keyboard Port
- `0x3D4`, `0x3D5` VGA CRT Control Register Port (Blinking Cursor Port)

## Features Added:
- Bootloader
- Custom Kernel
- Shell
- Makefile build toolchain
- Interrupt Descriptor Table (IDT)
- Interrupt Service Routine (ISR)
- Programmable Interrupt Controller (8259 PIC)
- IRQ (Custom ISR)
- PS/2 Keyboard Input Driver
- VGA Text Buffer Mode Driver
- Programmable Interval Timer (100Hz frequency)
- Custom libc
- Physical Memory Manager

## Add Next:
- printf()
- paging
- malloc and free
- multitasking? sleep?
- file system?
- ring 3 user mode
- fix documentation

## References:
- [OSdev Wiki](https://wiki.osdev.org/)