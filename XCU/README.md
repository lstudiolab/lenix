# XCU

XCU is the kernel architecture of Lenix. It contains Nux and the architecture-specific code required to run Nux on AArch64 and x86-64.

## Design constraints

1. Nux is an original microkernel, not a modified upstream kernel.
2. Only AArch64 and x86-64 are supported targets.
3. Architecture-independent mechanisms must not contain CPU-specific register or instruction assumptions.
4. Privileged operations live behind narrow architecture interfaces.
5. Drivers and non-essential operating-system services should run outside the kernel when practical.
6. Cross-component communication uses explicit kernel IPC objects and capability-checked rights.
7. Kernel implementation is C and assembly only.

## Components

- `Nux/` — microkernel mechanisms and public internal interfaces.
- `Architecture/ARM64/` — AArch64 entry, CPU, MMU, interrupt, and context code.
- `Architecture/X86_64/` — x86-64 entry, CPU, MMU, interrupt, and context code.
- `Kernel/` — image entry, system-call boundary, initialization, and panic path.
- `Interfaces/` — stable interfaces exposed between kernel subsystems.
- `Tests/` — architecture-independent and architecture-specific tests.

## Hardware boundary

Assembly is the first layer that executes architecture-specific privileged instructions. It establishes a known CPU state and transfers control into C only after the relevant entry contract has been satisfied. C requests privileged work through explicit architecture functions; it does not directly duplicate low-level register sequences throughout the kernel.
