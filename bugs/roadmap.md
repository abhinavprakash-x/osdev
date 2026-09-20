# OS Development Roadmap

## Development Philosophy

Bugs are fixed and verified before major new functionality is built.
New bugs discovered during development are added to the bug tracker rather
than being silently fixed as unrelated changes.

---

# Milestone 1 — Stabilize and Verify

Goal: establish a trustworthy baseline for the current kernel.

- Add regression tests for every fixed bug.
- Run the kernel under QEMU with assertions and logging.
- Test allocation failures.
- Test invalid addresses.
- Test task exhaustion.
- Test large inputs.
- Verify existing PMM, paging, heap, scheduler, and syscall behavior.

Completion criterion:

> Every currently tracked bug that is marked FIXED has a corresponding
> regression test and has been VERIFIED.

---

# Milestone 2 — Improve Memory Management

Goal: make physical and virtual memory management reliable enough
to support multiple independent processes.

- Add physical-frame ownership/reference counting.
- Implement page-fault diagnostics.
- Add virtual-memory allocation helpers.
- Separate kernel and user address-space policies.

Completion criterion:

> User processes can safely allocate, map, unmap, and release memory
> without leaking or freeing memory owned by another process.

---

# Milestone 3 — Build a Reliable Process Model

Goal: establish well-defined process/task lifetime and scheduling.

- Define task creation and cleanup contracts.
- Add an idle task.
- Make scheduler behavior robust when no normal task is runnable.
- Implement process termination.
- Add exit statuses.
- Preserve complete CPU state during context switches.

Completion criterion:

> Multiple processes can start, sleep, wake, terminate, and be cleaned up
> without corrupting scheduler state or leaking resources.

---

# Milestone 4 — Create a Real User-Program Loader

Goal: replace the temporary user-test mechanism with a proper executable loader.

- Replace raw `user_test` copying with an ELF loader.
- Allocate multiple code, data, and stack pages.
- Validate ELF headers.
- Validate ELF segments.
- Apply segment permissions correctly.
- Map user memory through safe kernel mappings.

Completion criterion:

> The kernel can load and start a valid ELF user program in its own address space.

---

# Milestone 5 — Add Core Kernel Services

Goal: provide the basic services required by real user programs.

- Validate system-call arguments.
- Add file-system support.
- Add block-device support.
- Add timer-based sleeping.
- Add synchronization primitives.
- Improve keyboard/input abstractions.

Completion criterion:

> User programs can interact with the kernel through well-defined system
> calls and basic persistent/device-backed services.

---

# Milestone 6 — Improve Engineering Quality

Goal: make the project easier to maintain and extend.

- Maintain automatic header dependency tracking.
- Add build-time kernel-size checks.
- Use consistent error-handling conventions.
- Define ownership rules consistently.
- Document invariants for:
  - PMM
  - paging
  - heap
  - scheduler
  - syscalls

Completion criterion:

> Core subsystems have documented invariants, predictable ownership rules,
> and a build system that catches common integration mistakes automatically.

---

# Current Major Milestone

> **Run multiple user processes safely, with reliable allocation,
> cleanup, scheduling, and page-fault handling.**