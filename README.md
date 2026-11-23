# Lenix

**The Lenix Operating System**

Lenix is an original operating-system project targeting **AArch64 (ARM64)** and **x86-64**.

The kernel architecture is **XCU**. Its microkernel, **Nux**, is designed and implemented from original Lenix source code. Nux takes inspiration from the concepts and organization of Mach-style microkernels, but does not incorporate Mach, Apple XNU, BSD, or Linux kernel source.

## Repository layout

- `XCU/` — kernel source, architecture ports, interfaces, and kernel tests.
- `Documentation/` — architecture and engineering documentation.

## Implementation languages

- **Assembly is primary at the hardware boundary.** Each CPU architecture has its own assembly implementation for boot entry, CPU state, exception and interrupt entry/return, context switching, low-level memory-management operations, and system-call entry.
- **C implements the portable kernel mechanisms** and policies: task/thread objects, scheduling decisions, IPC, capabilities, virtual-memory bookkeeping, synchronization, and service interfaces.
- Build configuration may use Makefiles and other declarative build metadata. Kernel implementation code is limited to C and assembly.

## Supported architectures

| Architecture | Assembly | C ABI |
|---|---|---|
| AArch64 | GNU assembler syntax for AArch64 | AAPCS64 |
| x86-64 | GNU assembler syntax for x86-64 | System V AMD64 ABI (kernel-specific rules documented per entry path) |

Nux is not yet a bootable or production-ready kernel. The initial work establishes its architecture and interfaces before privileged execution paths are implemented and tested.

## License

See [LICENSE](LICENSE).
