# OS Development Roadmap

## Development Philosophy

Bugs are fixed and verified before major new functionality is built.
New bugs discovered during development are added to the bug tracker rather
than being silently fixed as unrelated changes.

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

Step 3. interactive userspace (SYS_READ SYS_EXEC)
step 4. Writable filesystem
Step 6. Directories/improving the BMFS
Step 7. Resolve BUG-046
Step 8. Documentation Overhaul