# OS Development Roadmap

## Milestone 1 — Stabilize and Verify (Completed)

- Add regression tests for every fixed bug.
- Run the kernel under QEMU with assertions and logging.
- Test allocation failures, invalid addresses, task exhaustion, and large inputs.
- Verify PMM, paging, heap, scheduler, and syscall behavior.

---

## Milestone 2 — Improve Memory Management (Completed)

- Add physical-frame ownership and reference counting (`pmm_ref_block`, `pmm_get_refcount`).
- Implement page-fault diagnostics (`CR2` and error-code decoding).
- Add virtual-memory allocation helpers (`paging_alloc_pages`, `paging_free_pages`, `paging_temp_map`).
- Separate kernel and user address-space policies (`0x00400000 – 0xBFFFFFFF` user space, `0xC0000000 – 0xE0000000` kernel heap).
- Relocate kernel to `0x00100000` (1 MiB) and move IDT into `.bss` (`BUG-046`).

---

## Milestone 3 — Build a Reliable Process Model (Completed)

- Define task creation and cleanup contracts (`create_task`, `create_user_task`, `destroy_task`).
- Use `PID 0` (`kmain`) as the idle task and halt the CPU (`hlt`) when all tasks are sleeping.
- Implement process termination with exit statuses (`task_exit_with_status`, `SYS_EXIT`) and synchronous waiting (`task_wait_pid`, `SYS_WAITPID`).
- Isolate Ring 3 CPU exceptions (`[USER FAULT]`) so crashing user tasks do not panic the kernel.

---

## Milestone 4 — Create a Real User-Program Loader (Completed)

- Implement `elf_validate()` and `create_user_task_elf()` (`src/task/elf.c`).
- Allocate and map multiple `PT_LOAD` code, rodata, data, and `.bss` pages.
- Enforce segment permissions (`PF_R`, `PF_W`, `PF_X`) with `CR0.WP` enabled.
- Provide a freestanding userspace C runtime (`user/ulib.c`, `user/ulib.h`, `user/linker.ld`).

---

## Milestone 5 — Add Core Kernel Services (Completed)

- Validate system-call arguments and writable user ranges (`SYS_WRITE`, `SYS_READ`, `SYS_WAITPID`, `SYS_EXEC`).
- Add 28-bit LBA ATA PIO disk driver (`ata_read_sectors`, `ata_write_sectors`).
- Add persistent BMFS filesystem with hierarchical directories (`mkdir`, `cd`, `pwd`), file creation/deletion (`write`, `rm`), and first-fit sector reuse.
- Add 200-line VGA scrollback buffer with keyboard arrow/page navigation.

---

## Milestone 6 — Improve Engineering Quality (Completed)

- Maintain automatic header dependency tracking (`-MMD -MP`) and automatic `user/*.c` ELF discovery in `Makefile`.
- Enforce build-time kernel size and `.bss` boundary checks (`linker.ld`).
- Keep architecture, memory, multitasking, syscall, and filesystem documentation synchronized with the codebase.

---

## Milestone 7 — Advanced Subsystems

Goal: expand Bare Minimum OS into a graphical, self-hosting, and networked system.

- **Step 1: Add a GUI** — Switch to a high-resolution VESA/VBE framebuffer, implement a bitmap font renderer, mouse driver,
and windowing/compositor layer with a few simple GUI applications (calculator, text editor, paint, image viewer, etc.).
- **Step 2: Port TCC / GCC** — Expand the userspace C library (`malloc`/`free`, file descriptors)
and port a C compiler to compile programs natively inside the OS.
- **Step 3: Add a Network Stack** — Implement a PCI driver, NIC driver (RTL8139 / E1000), and Ethernet/ARP/IPv4/ICMP/UDP/TCP stack.
(Add a Browser too maybe.)
- **Step 4: Add Audio Support** — Implement PC Speaker / SoundBlaster 16 / AC'97 audio playback.
- **Step 5: UEFI & Real Hardware Support (Optional)** — Add a UEFI bootloader path and test/harden drivers on physical x86 hardware.