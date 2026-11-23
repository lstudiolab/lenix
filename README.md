# Lenix

**The Lenix Operating System**

Lenix is an original operating-system project targeting **AArch64 (ARM64)** and **x86-64**.

The kernel architecture is **XCU**. Its microkernel, **Nux**, is designed and implemented from original Lenix source code. Nux takes inspiration from Mach-style microkernel concepts, but does not incorporate Mach, Apple XNU, BSD, or Linux kernel source.

## Repository layout

- `XCU/` — kernel source, architecture ports, interfaces, tests, and build system.
- `Documentation/` — architecture and engineering documentation.

## Implementation languages

- **Assembly is primary at the hardware boundary.** Each CPU architecture has its own assembly implementation for firmware entry, CPU state, exception and interrupt entry/return, context switching, low-level memory-management operations, and system-call entry.
- **C implements portable kernel mechanisms** and policies: task/thread objects, scheduling decisions, IPC, capabilities, virtual-memory bookkeeping, synchronization, and service interfaces.
- Build configuration may use Makefiles and GitHub Actions. Kernel implementation code is limited to C and assembly.

## Supported architectures

| Architecture | Assembly | C ABI |
|---|---|---|
| AArch64 | GNU assembler syntax for AArch64 | AAPCS64 |
| x86-64 | GNU assembler syntax for x86-64 | System V AMD64 ABI for the kernel; Microsoft x64 ABI only at UEFI application boundaries |

## First-party boot architecture

Lenix owns its boot chain. It uses the platform's UEFI firmware interface, then runs two original Lenix EFI applications:

1. **Lenix Boot Manager** — architecture-specific `BOOTX64.EFI` or `BOOTAA64.EFI`, which validates and starts the Lenix kernel-loader application.
2. **Nux Kernel Loader** — `NUX.EFI`, which validates and loads the architecture-matched `NUX.ELF`, reserves kernel and boot-stack memory, builds identity and high-half/direct-map page tables, captures the final UEFI memory map, calls `ExitBootServices`, and transfers control to the kernel's first-party entry point.

There is no third-party bootloader in this path. The first-party chain and real Nux ELF kernels have both been booted under OVMF/AAVMF QEMU firmware on x86-64 and AArch64. The kernel's user-mode smoke program enters user mode, makes a syscall, exits, and returns to the kernel; CI verifies the `NUX:READY` milestone on both architectures.

The current loader maps the first 4 GiB identity range for its EFI execution context and provides a 64 GiB high-half direct-map window to Nux. The initial ELF image is limited to 64 MiB, and the handoff accepts up to 256 memory-map regions. These are explicit initial limits, not claims of unlimited physical-memory support. Signed boot manifests, Secure Boot key enrollment, recovery UI, device-tree support, physical-hardware qualification, and a production boot policy remain unfinished. Native Apple Silicon startup is not implied by AArch64 UEFI support.

See [the first-party boot architecture](XCU/Kernel/Boot/BOOT_ARCHITECTURE.md) for the handoff contract, implementation stages, and remaining work.

## Current kernel status

Nux has progressed beyond an architecture skeleton. Both architecture targets cross-build, the host-side regression suite passes in CI, and the first-party UEFI images reach the Nux `READY` phase in QEMU on x86-64 and AArch64.

Implemented foundations include physical memory management, architecture page tables and address-space switching, kernel and user threads, cooperative context switching, task-scoped capabilities, bounded IPC, synchronization primitives, anonymous demand-zero paging, anonymous copy-on-write with page reference counting and fault-time splitting, x86-64 `int 0x80` and AArch64 `SVC` syscall paths, ACPI topology discovery, static ELF64 loading, and initial user-stack construction.

**Nux is not yet a complete general-purpose or production-ready microkernel.** Preemptive scheduling, blocking IPC deadlines and RPC, external pager/file-backed demand paging, recoverable exception ports, interrupt-controller and timer integration, SMP startup, TLS/FP state management, a persistent user-space bootstrap/service manager, and device/filesystem/network servers remain incomplete. The detailed implementation matrix is maintained in [XCU/Nux/MACH_PARITY.md](XCU/Nux/MACH_PARITY.md), and the subsystem overview is in [XCU/Nux/README.md](XCU/Nux/README.md).

## Building from macOS

Install the required toolchain and image utilities:

```sh
brew install llvm lld make qemu xorriso mtools
export PATH="$(brew --prefix llvm)/bin:$(brew --prefix make)/libexec/gnubin:$PATH"
make -C XCU clean all iso uefi-disk
```

This builds the first-party EFI applications and kernel images for x86-64 and AArch64, runs the host-side regression tests, and creates UEFI-bootable ISO and removable-media images. The images require UEFI firmware; legacy BIOS boot is not provided.

## License

See [LICENSE](LICENSE).
