# System Calls

System calls provide the interface between Ring 3 user programs and the kernel. A user program requests a kernel service with software interrupt `int 0x80`. The CPU enters the kernel through ISR 128, the syscall dispatcher examines the syscall number, executes the requested operation, and returns the result in `EAX`.

The current syscall interface is intentionally small and is used by the userspace regression program in `src/apps/user_test.asm`.

## Syscall ABI

The syscall ABI defines how a user program communicates with the kernel. This ABI is part of the userspace/kernel contract and should remain stable as the kernel grows.

### Calling convention

| Register | Purpose |
|---|---|
| `EAX` | Syscall number on entry; return value on exit |
| `EBX` | First syscall argument |
| `ECX` | Second syscall argument |
| `EDX` | Third syscall argument / general-purpose register |
| `ESI` | General-purpose register |
| `EDI` | General-purpose register |

A syscall is invoked with:

```asm
mov eax, <syscall_number>
; set EBX/ECX/EDX/... as required
int 0x80
```

The kernel returns the syscall result in `EAX`.

Negative error values are represented as 32-bit two's-complement values. For example, `-1` is returned as `0xFFFFFFFF`.

## Syscall Table

| Number | Name | Arguments | Return value |
|---:|---|---|---|
| `0` | `SYS_TEST` | none | `42` |
| `1` | `SYS_WRITE` | `EBX = buffer`, `ECX = length` | bytes written, or `-1` |
| `2` | `SYS_EXIT` | `EBX = exit status`* | does not return |
| `3` | `SYS_YIELD` | none | `0` |
| `4` | `SYS_SLEEP` | `EBX = milliseconds` | `0` |
| `5` | `SYS_GETPID` | none | current PID |

\* `EBX` is reserved for the exit status in the ABI, but the current kernel implementation does not yet preserve or use that status. `SYS_EXIT` currently terminates the current task without returning.

Unknown syscall numbers return `-1`.

## `SYS_TEST` — 0

A simple kernel test syscall used during early userspace development.

### Usage

```asm
mov eax, SYS_TEST
int 0x80
```

### Return value

```text
EAX = 42
```

This syscall is primarily a sanity check that the Ring 3 → Ring 0 → Ring 3 path is working.

## `SYS_WRITE` — 1

Writes bytes from a user-space buffer to the kernel console.

### Arguments

```text
EBX = address of user buffer
ECX = number of bytes to write
```

### Usage

```asm
mov eax, SYS_WRITE
mov ebx, buffer
mov ecx, buffer_length
int 0x80
```

### Return value

On success:

```text
EAX = number of bytes written
```

If the supplied user buffer is invalid or is not accessible from Ring 3:

```text
EAX = -1
```

### User pointer validation

`SYS_WRITE` must not trust a pointer supplied by Ring 3. Before reading the buffer, the kernel validates the entire address range against the current process address space.

Validation includes:

- detecting zero-length and 32-bit address-range overflow;
- checking that each page is present;
- checking that both the page-directory entry and page-table entry have the user-accessible (`PTE_USER`) permission; and
- ensuring the range remains inside the defined user virtual address range.

The complete range is validated before the kernel begins reading from it, so a buffer that crosses into an unmapped or kernel-only page is rejected rather than causing a kernel page fault midway through the write.

## `SYS_EXIT` — 2

Terminates the current task.

### ABI

```text
EBX = exit status
```

The exit status is reserved by the ABI for future process-management functionality. The current implementation ignores it.

### Usage

```asm
mov eax, SYS_EXIT
xor ebx, ebx
int 0x80
```

`SYS_EXIT` should never return to user mode. The scheduler marks the task dead and switches to another runnable task.

## `SYS_YIELD` — 3

Voluntarily gives up the CPU and allows the scheduler to select another runnable task.

### Usage

```asm
mov eax, SYS_YIELD
int 0x80
```

### Return value

```text
EAX = 0
```

A task may resume later from the instruction following `int 0x80`.

## `SYS_SLEEP` — 4

Blocks the current task for a specified number of milliseconds.

### Arguments

```text
EBX = duration in milliseconds
```

### Usage

```asm
mov eax, SYS_SLEEP
mov ebx, 2000
int 0x80
```

### Behavior

The current task is changed from `TASK_RUNNING` to `TASK_WAITING`. The scheduler can then run another task while the sleep interval elapses. Timer ticks are used to determine when the task should become runnable again.

When the task is woken, execution continues after the syscall.

### Return value

```text
EAX = 0
```

## `SYS_GETPID` — 5

Returns the process/task ID of the currently running task.

### Usage

```asm
mov eax, SYS_GETPID
int 0x80
```

### Return value

```text
EAX = current task PID
```

PID `0` is used by the kernel's initial task (`kmain`). User tasks are assigned non-zero PIDs.

## Syscall Execution Flow

The current 32-bit implementation follows this path:

```text
User program (Ring 3)
        |
        | int 0x80
        v
ISR 128
        |
        v
Common ISR stub
        |
        v
isr_handler()
        |
        v
syscall_handler()
        |
        +--> SYS_TEST
        +--> SYS_WRITE
        +--> SYS_EXIT
        +--> SYS_YIELD
        +--> SYS_SLEEP
        +--> SYS_GETPID
        |
        v
Return value in EAX
        |
        v
iret
        |
        v
User program (Ring 3)
```

The interrupt path saves the CPU state on the task's kernel stack. For a Ring 3 → Ring 0 transition, the CPU uses the task's `TSS.ESP0` as the kernel stack. The scheduler updates that kernel-stack pointer when switching to a user task.

## Per-Process Address Spaces

User tasks have their own page directory. The scheduler loads the task's address space into `CR3` before restoring its CPU context.

Kernel mappings are shared between address spaces, while user mappings belong to the individual task. This allows multiple user tasks to use the same virtual addresses without sharing their private pages.

This is particularly important for syscalls that accept user pointers, such as `SYS_WRITE`: validation is performed against the address space of the currently running task.

## Userspace Testing

`src/apps/user_test.asm` provides a userspace regression test for the syscall ABI. It currently exercises:

```text
SYS_TEST
SYS_GETPID
SYS_WRITE (valid buffer)
SYS_WRITE (invalid buffer)
SYS_YIELD
SYS_SLEEP
SYS_EXIT
```

The test uses `SYS_WRITE` itself to report successful checks, including a deliberately invalid pointer (`0xDEADBEEF`) to verify that the kernel rejects invalid user memory without crashing.

A successful run ends with:

```text
================================
    ALL SYSCALL TESTS PASSED
================================
```

followed by `SYS_EXIT`, after which the user task is removed by the scheduler.

## Current Limitations

The syscall layer is intentionally minimal. In particular:

- `SYS_WRITE` writes directly to the kernel console; file descriptors are not implemented yet.
- `SYS_EXIT` does not currently expose an exit status to a parent process.
- There is no userspace C library or generic syscall wrapper layer yet; tests invoke `int 0x80` directly from assembly.
- The user executable is currently a raw userspace test image rather than an ELF-loaded process.
- User pointer validation currently checks page permissions, but the broader virtual-memory management and reclamation model is still under development.

As the kernel evolves, new syscalls should be added to the syscall table and their register-level ABI should be documented here at the same time.
