# Lenix

**The Lenix Operating System**

Lenix is an original operating-system project targeting **AArch64 (ARM64)** and **x86-64**.

The kernel architecture is **XCU**. Its microkernel, **Nux**, is designed and implemented from original Lenix source code. Nux takes inspiration from Mach-style microkernel concepts, but does not incorporate Mach, Apple XNU, BSD, or Linux kernel source.

## Repository layout

- `XCU/` — kernel source, architecture ports, interfaces, tests, and build system.
- `Documentation/` — architecture and engineering documentation.

## Implementation languages

- **Assembly is primary at the hardware boundary.** Each CPU architecture has its own assembly implementation for boot entry, CPU state, exception and interrupt entry/return, context switching, low-level memory-management operations, and system-call entry.
- **C implements portable kernel mechanisms** and policies: task/thread objects, scheduling decisions, IPC, capabilities, virtual-memory bookkeeping, synchronization, and service interfaces.
- Build configuration may use Makefiles and GitHub Actions. Kernel implementation code is limited to C and assembly.

## Supported architectures

| Architecture | Assembly | C ABI |
|---|---|---|
| AArch64 | GNU assembler syntax for AArch64 | AAPCS64 |
| x86-64 | GNU assembler syntax for x86-64 | System V AMD64 ABI (kernel-specific rules documented per entry path) |

## Current implementation status

Nux has progressed beyond an architecture skeleton. Both architecture targets cross-build, the host-side regression suite passes in CI, and UEFI images have reached the Nux `READY` phase in QEMU on x86-64 and AArch64.

Implemented foundations include physical memory management, architecture page tables and address-space switching, kernel and user threads, cooperative context switching, task-scoped capabilities, bounded IPC, synchronization primitives, x86-64 `int 0x80` and AArch64 `SVC` syscall paths, ACPI topology discovery, static ELF64 loading, and initial user-stack construction. QEMU smoke tests execute a user ELF and exit through the syscall ABI on both architectures.

**Nux is not yet a complete general-purpose or production-ready microkernel.** The initial user-mode entry and syscall paths are now exercised in QEMU, but there is no persistent user-space bootstrap/service manager yet. Preemptive scheduling, blocking IPC, demand paging/COW, recoverable exception ports, interrupt-controller and timer integration, SMP startup, TLS/FP state management, and user-space service infrastructure remain incomplete. The detailed implementation matrix is maintained in [XCU/Nux/MACH_PARITY.md](XCU/Nux/MACH_PARITY.md), and the subsystem overview is in [XCU/Nux/README.md](XCU/Nux/README.md).

## Building from macOS

Install the required toolchain and image utilities:

```sh
brew install llvm lld make qemu xorriso mtools
export PATH="$(brew --prefix llvm)/bin:$(brew --prefix make)/libexec/gnubin:$PATH"
make -C XCU clean all iso uefi-disk
```

This builds both ELF kernels, runs the host-side tests, and creates UEFI ISO and removable-media images for x86-64 and AArch64. CI additionally boots both architectures under QEMU. The boot chain is being migrated from the temporary Limine development handoff to the first-party Lenix Boot Manager. The target design is documented in [XCU/Kernel/Boot/BOOT_ARCHITECTURE.md](XCU/Kernel/Boot/BOOT_ARCHITECTURE.md); until the migration is validated, existing images must still be treated as development images, not the final first-party boot path. This does not establish native Apple Silicon boot support.

## License

See [LICENSE](LICENSE).
