# Bare Minimum OS

A 32-bit i686 operating system written from scratch in C and x86 Assembly.

The project currently boots through a custom BIOS bootloader, enters 32-bit protected mode, initializes the kernel's GDT/TSS/IDT/PIC/PIT infrastructure, manages physical and virtual memory, provides a kernel heap, runs scheduled tasks, and contains a small Ring 3 syscall path.

## Current Features

### Boot and CPU

- Custom BIOS bootloader using BIOS disk services and LBA reads
- 32-bit x86 protected mode
- Kernel GDT with kernel and user segments
- Task State Segment (TSS)
- Interrupt Descriptor Table (IDT)
- 8259 PIC remapping and IRQ handling
- CPU exception handling
- Ring 0 to Ring 3 transition

### Memory Management

- BIOS E820 memory-map detection
- Bitmap-based physical memory manager
- 4 KiB physical-frame allocation
- Next-fit physical-frame allocation
- 32-bit two-level paging
- Recursive page-directory mapping
- Per-task page directories for user tasks
- Kernel heap with `kmalloc()` / `kfree()`
- Heap block splitting and coalescing

### Multitasking

- Preemptive scheduling driven by the PIT
- Task control blocks
- `READY`, `RUNNING`, `WAITING`, and `DEAD` task states
- Task sleeping and waking
- Context switching
- Kernel tasks
- User-task creation path

### Drivers

- VGA text-mode driver
- PS/2 keyboard driver
- Programmable Interval Timer (PIT)
- 8259 PIC support

### Kernel Utilities

- Interactive kernel shell
- Freestanding C library routines used by the kernel
- Kernel test suite
- CPU information command
- Kernel memory-management inspection commands
- Exception/page-fault crash commands

### System Calls and Userspace

- Ring 3 user tasks
- System call ABI
- User-pointer validation
- Userspace test program

## Build and Run

### Requirements

- `i686-elf-gcc` cross-compiler and binutils
- NASM
- QEMU x86 system emulator

See [Toolchain Setup](docs/toolchain_setup.md) for the cross-compiler setup used by this project.

From the repository root:

```bash
make all        # Build the boot image
make check      # Compile all source files
make run        # Build and run in QEMU
make debug      # Build and run QEMU with interrupt logging
make print      # Print discovered sources and objects
make clean      # Remove build files
make rebuild    # Clean and rebuild everything
make help       # Show all Makefile targets
```

## Project Structure

Generated build output and the local cross-toolchain are kept outside the maintained source/documentation areas.

```text
osdev/
├── bugs/
│   ├── bugs_summary.md
│   ├── bugs_001-025.md
│   ├── bugs_026-050.md
│   └── roadmap.md
│
├── docs/
│   ├── architecture.md
│   ├── memory.md
│   ├── multitasking.md
│   ├── syscall.md
│   ├── toolchain_setup.md
│   └── images/
│
├── include/
│   ├── cpu/
│   ├── drivers/
│   ├── interrupts/
│   ├── libc/
│   ├── mm/
│   ├── syscall/
│   └── task/
│
├── src/
│   ├── apps/
│   ├── cpu/
│   ├── drivers/
│   ├── interrupts/
│   ├── libc/
│   ├── mm/
│   ├── syscall/
│   ├── task/
│   ├── bootloader.asm
│   ├── kernel_entry.asm
│   └── kernel.c
│
├── .gitattributes
├── .gitignore
├── LICENSE
├── linker.ld
├── Makefile
└── README.md
```

`build/` contains generated object files, binaries, and the boot image. `cross/` contains the locally installed `i686-elf` cross-toolchain.

## Documentation

- [Architecture](docs/architecture.md) — boot flow, CPU/interrupt setup, hardware I/O, and kernel subsystem layout
- [Memory Management](docs/memory.md) — E820, PMM, paging, recursive mapping, and heap
- [Multitasking](docs/multitasking.md) — task structure, scheduler, task states, context switching, and Ring 3 task setup
- [System Calls](docs/syscall.md) — syscall ABI, syscall table, user-pointer validation, and userspace test program
- [Toolchain Setup](docs/toolchain_setup.md) — building the `i686-elf` toolchain used by the project
- [Bug Tracker](bugs/bugs_summary.md) — current bug index and detailed bug reports
- [Development Plan](bugs/roadmap.md) — planned development milestones

## Screenshots

### Boot

![Bare Minimum OS boot](docs/images/boot.png)

### Kernel Shell

![Bare Minimum OS shell](docs/images/shell.png)

### Kernel Test Suite

![Kernel test suite](docs/images/tests.png)

## License

This project is open-source and licensed under the terms of the `LICENSE` file.
