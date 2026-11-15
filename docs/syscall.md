# System Calls & Userspace Runtime

Bare Minimum OS provides a protected system-call interface between Ring 3 user programs and the Ring 0 kernel via software interrupt `int 0x80` (IDT vector `128`, DPL 3).

## Syscall ABI

System calls pass arguments in registers and receive the return value in `EAX`:

| Register | Role |
|---|---|
| `EAX` | Syscall number on entry; return value on exit (`-1` on error) |
| `EBX` | Argument 1 |
| `ECX` | Argument 2 |
| `EDX` | Argument 3 (reserved) |

## Syscall Table

Defined in `include/syscall/syscall.h` and `user/ulib.h`:

| Number | Name | Arguments | Return Value | Description |
|---:|---|---|---|---|
| `0` | `SYS_TEST` | None | `42` | Sanity-check syscall path |
| `1` | `SYS_WRITE` | `EBX = const char* buf`<br>`ECX = uint32_t len` | Bytes written, or `-1` | Write buffer to the VGA console |
| `2` | `SYS_EXIT` | `EBX = int32_t status` | Does not return | Terminate current process and record `status` |
| `3` | `SYS_YIELD` | None | `0` | Voluntarily yield the CPU to the scheduler |
| `4` | `SYS_SLEEP` | `EBX = uint32_t ms` | `0` | Put current task to sleep for `ms` milliseconds |
| `5` | `SYS_GETPID` | None | `uint32_t pid` | Return the calling task's PID |
| `6` | `SYS_WAITPID` | `EBX = uint32_t pid`<br>`ECX = int32_t* out_status` | Child exit status, or `-1` | Wait for task `pid` to terminate and optionally write status to `*out_status` |
| `7` | `SYS_READ` | `EBX = char* buf`<br>`ECX = uint32_t max_len` | Bytes read, or `-1` | Read line-buffered keyboard input (until `\n` or `max_len`) with live echo and `\b` handling |
| `8` | `SYS_EXEC` | `EBX = const char* path` | New child PID, or `-1` | Load and launch an ELF32 program from BMFS |

## User Pointer Validation

All pointer arguments passed from Ring 3 are validated before the kernel reads or writes them:

1. **Read Validation (`user_range_valid`):**
   - Rejects 32-bit address wraparound (`end < virtual_addr`).
   - Requires the entire range to lie within `[USER_SPACE_START, USER_SPACE_END]` (`0x00400000 – 0xBFFFFFFF`).
   - Walks the active page directory/page tables and verifies that every page in the range has `PTE_PRESENT | PTE_USER` set on both its PDE and PTE.
2. **Write Validation (`paging_is_mapped_range(..., true)`):**
   - For syscalls that write into user memory (`SYS_READ` and `SYS_WAITPID` when `out_status != NULL`), the kernel additionally requires `PTE_RW` on every touched PDE and PTE so a user program cannot trick the kernel into overwriting read-only `.text`/`.rodata` pages.
3. **Bounded String Copy (`SYS_EXEC`):**
   - `sys_exec()` validates `user_filename` byte-by-byte into a kernel buffer (`kfilename[FS_MAX_FILENAME]`), rejecting unmapped pointers, empty strings, or strings not null-terminated within `FS_MAX_FILENAME` (`20`) bytes.

## Userspace C Runtime (`user/`)

Standalone C programs in `user/` are compiled with `-ffreestanding` and linked against `user/ulib.o` using `user/linker.ld`.

### Userspace Linker Script (`user/linker.ld`)

- Entry point: `ENTRY(_start)` at virtual address `0x00400000`.
- Emits two page-aligned (`0x1000`) `PT_LOAD` program headers:
  1. `text` (`FLAGS(5)` = `PF_R | PF_X`): `.text.start`, `.text*`, and `.rodata*`.
  2. `data` (`FLAGS(6)` = `PF_R | PF_W`): `.data*` and `.bss*` (`COMMON`).

### Userspace Library (`user/ulib.h` & `user/ulib.c`)

- `_start()`: Calls `int status = main();` and passes `status` to `sys_exit(status)`.
- Syscall wrappers: `sys_test()`, `sys_write()`, `sys_exit()`, `sys_yield()`, `sys_sleep()`, `sys_getpid()`, `sys_waitpid()`, `sys_read()`, and `sys_exec()`.
- String & formatting utilities: `strlen()`, `strcmp()`, `uatoi()`, `uitoa()`, `uprint()`, `uprintf()` (`%d`, `%s`, `%c`, `%%`), and `readline(buf, max_len)`.

### Included Userspace Applications

All `.c` files in `user/` (except `ulib.c`) are automatically discovered by the `Makefile`, compiled to `build/user/<name>.elf`, and packed into the BMFS disk image:

- `hello.elf` (`user/hello.c`): Verifies Ring 3 ELF execution, `SYS_GETPID`, `.data`, and `.bss` initialization.
- `fib.elf` (`user/fib.c`): Computes and prints the first 12 Fibonacci numbers and exits with status `12`.
- `counter.elf` (`user/counter.c`): Background multitasking demo that sleeps 1 second per tick for 5 seconds.
- `calc.elf` (`user/calc.c`): Interactive calculator using `readline()` (`SYS_READ`) and `uatoi()`.
- `sleep.elf` (`user/sleep.c`): Sleeps for 20 seconds to test background process inspection via `ps`.