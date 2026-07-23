# Kernel Architecture & Design

Bare Minimum OS is a 32-bit x86 operating system using a monolithic
kernel architecture. The kernel is written in C and x86 assembly and
currently executes entirely in Ring 0.

## Physical Memory Map

Because Bare Minimum OS runs directly on bare metal, certain physical
memory regions have fixed purposes and must not be overwritten.

| Address | End | Description |
|---|---|---|
| `0x00000` | `0x003FF` | BIOS Interrupt Vector Table (IVT) |
| `0x00400` | `0x004FF` | BIOS Data Area (BDA) |
| `0x05000` | — | E820 memory map data |
| `0x07C00` | `0x07DFF` | Custom bootloader (512 bytes) |
| `0x08000` | — | Kernel code and data |
| `0x90000` | — | Interrupt Descriptor Table (IDT) |
| `0xA0000` | — | Initial kernel stack top (grows downward) |
| `0xB8000` | `0xBFFFF` | VGA text-mode memory |

The BIOS E820 memory map stores its entry count at `0x5000`, followed
by the memory-map entries beginning at `0x5004`.

The kernel is linked and loaded at physical address `0x8000`.

## Boot Process

The system boots using a custom BIOS bootloader.

The boot process is roughly:

```text
BIOS
  |
  v
Bootloader (0x7C00)
  |
  +-- Load kernel from disk
  +-- Obtain E820 memory map
  +-- Enable A20
  +-- Load GDT
  +-- Enter 32-bit protected mode
  |
  v
Kernel Entry
  |
  v
kmain()
```

The bootloader loads the kernel into memory and transfers execution to
the 32-bit kernel after entering protected mode.

The GDT is currently created by the bootloader and contains the
descriptors required for Ring 0 execution. Moving GDT management into
the kernel is planned as part of the transition to user mode.

## Kernel Initialization

The kernel initializes its major subsystems before starting the
interactive shell.

```text
IDT
 |
PIC
 |
PIT
 |
Physical Memory Manager
 |
Paging
 |
Kernel Heap
 |
Scheduler
 |
Shell
```

Hardware interrupts are enabled after the required interrupt and
scheduler infrastructure has been initialized.

## Interrupts & IRQ Routing

The kernel uses the Interrupt Descriptor Table (IDT) for CPU exceptions
and hardware interrupts.

The legacy 8259 Programmable Interrupt Controller (PIC) is remapped
because its default IRQ vectors overlap with the CPU exception vectors
`0–31`.

After remapping:

| Source | IRQ | IDT Vector |
|---|---:|---:|
| CPU Exceptions | — | `0–31` |
| Master PIC | `IRQ 0–7` | `32–39` |
| Slave PIC | `IRQ 8–15` | `40–47` |

Currently active hardware interrupts are:

| IRQ | Vector | Device |
|---|---:|---|
| IRQ 0 | 32 | Programmable Interval Timer (PIT) |
| IRQ 1 | 33 | PS/2 Keyboard |

All other PIC IRQs are currently masked.

The PIT runs at 100 Hz and is also used to drive the preemptive
scheduler.

## Hardware I/O Ports

Hardware devices are accessed using the x86 `inb` and `outb`
instructions.

| Port | Purpose |
|---|---|
| `0x20` | Master PIC command |
| `0x21` | Master PIC data / interrupt mask |
| `0xA0` | Slave PIC command |
| `0xA1` | Slave PIC data / interrupt mask |
| `0x40` | PIT Channel 0 data |
| `0x43` | PIT command register |
| `0x60` | PS/2 keyboard data |
| `0x3D4` | VGA CRT controller index |
| `0x3D5` | VGA CRT controller data |
| `0x80` | I/O delay used during PIC initialization |

## Memory Management

Memory management is divided into three layers:

```text
kmalloc() / kfree()
        |
        v
   Kernel Heap
        |
        v
      Paging
        |
        v
Physical Memory Manager
        |
        v
   Physical Memory
```

The Physical Memory Manager tracks available 4 KiB physical frames
using a bitmap and allocates them using a next-fit strategy.

Paging provides virtual memory using the 32-bit x86 two-level paging
structure. The kernel also uses recursive page-directory mapping to
access paging structures through virtual memory.

The kernel heap provides dynamic allocation through `kmalloc()` and
`kfree()` and supports heap growth, block splitting and coalescing.

See [Memory Management](memory.md) for more details.

## Drivers

The kernel currently contains drivers for:

- VGA text-mode output
- PS/2 keyboard input
- Programmable Interval Timer
- 8259 PIC

The keyboard driver converts PS/2 Scancode Set 1 input into characters
and stores keyboard input in a circular buffer.

The VGA text driver writes directly to VGA text memory and uses a back
buffer to reduce unnecessary screen updates.

## Multitasking

Bare Minimum OS implements preemptive multitasking using kernel threads.

The PIT periodically interrupts the currently executing task, allowing
the scheduler to select another runnable task.

Tasks contain their execution context and scheduling information in a
task control structure and may transition between states such as
`READY`, `RUNNING`, and `WAITING`.

Tasks can sleep for a period of time and are returned to the runnable
state by the scheduler when their wake time is reached. Finished tasks
are removed and their resources are reclaimed.

See [Multitasking](multitasking.md) for more details.

## Kernel Shell

The interactive shell currently executes as a kernel thread in Ring 0.

It provides commands for interacting with and inspecting kernel
subsystems, including memory management, task information, CPU
information and the kernel test suite.

Because user mode has not yet been implemented, shell commands currently
have the same privileges as the kernel.

## Current Limitations

The OS currently:

- Runs entirely in Ring 0
- Uses BIOS booting
- Targets 32-bit x86
- Has no filesystem or disk driver
- Has no user-space processes or system calls
- Uses VGA text mode for display output

User mode, a kernel-managed GDT, TSS and system calls are the next major
architectural milestones.