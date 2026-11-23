# Lenix First-Party Boot Architecture

## Goal

Lenix owns the complete boot manager, kernel loader, image format validation, handoff, and architecture entry code. The only external component in the firmware boot path is the platform's UEFI implementation. No third-party boot manager or kernel loader is used.

The design takes inspiration from the staged startup model of macOS, but does not copy Apple's proprietary iBoot, boot.efi, BootPolicy, APFS Preboot, kernel collection, or Secure Enclave implementation.

## Current first-party boot chain

1. **Platform firmware** — UEFI initializes the machine and loads the architecture-matched Lenix EFI application from the EFI System Partition.
2. **Lenix Boot Manager** — `BOOTX64.EFI` or `BOOTAA64.EFI`. It validates the architecture of `NUX.EFI` and starts the first-party kernel-loader application.
3. **Nux Kernel Loader** — `NUX.EFI`. It opens `\\EFI\\LENIX\\NUX.ELF`, validates the ELF64 header and PT_LOAD program headers, reserves physical pages for the kernel and boot stack, and copies the loadable segments.
4. **Firmware handoff** — the loader captures the final UEFI memory map, retries `ExitBootServices` when the map key changes, and makes no further boot-service calls after a successful exit.
5. **Architecture page tables** — x86-64 installs identity, high-half kernel, and HHDM mappings. AArch64 installs the TTBR0 identity map and TTBR1 kernel/direct-map split, then enters EL1.
6. **Nux entry** — the architecture entry assembly establishes the kernel execution state and passes the boot information to staged Nux initialization.
7. **User execution** — Nux creates an isolated transient user task for the boot smoke test, enters ring 3/EL0, performs a syscall, exits, and returns to the kernel.

## Supported platform contract

- x86-64 UEFI on x86-64 machines.
- AArch64 UEFI on AArch64 machines.
- No 32-bit x86 or 32-bit ARM boot target.
- Native Apple Silicon startup is not implied by AArch64 UEFI support. Apple hardware has a distinct platform boot/security environment and needs a separately engineered port.
- No claim of Apple Secure Boot compatibility. Lenix image signing and key enrollment are its own design.

## Implementation ownership

All Lenix boot manager, ELF loader, handoff, page-table setup, and architecture-entry code is original Lenix C and assembly. Standard UEFI protocols, PE/COFF EFI applications, and ELF64 are used as platform interfaces and executable formats; they are not imported bootloader implementations.

## Handoff contract

The current handoff contains the firmware memory map, HHDM offset, firmware table pointer, and boot flags. It is sufficient for the current kernel initialization path, but it is not yet a fully versioned production ABI.

The production handoff still needs:
- signature, version, and structure-size validation;
- descriptor size/version and explicit memory reservation records;
- kernel and boot-asset physical ranges;
- ACPI and optional SMBIOS pointers;
- framebuffer information when available;
- selected boot-entry identity and firmware/boot protocol revisions;
- verified-image state and signed-manifest digest.

Nux must reject unknown handoff versions, invalid descriptor sizes, arithmetic overflow, overlapping untrusted ranges, missing memory-map data, and architecture/entry mismatches.

## Current implementation status (2026-11-23)

The first-party EFI applications and real Nux ELF kernels now boot under OVMF/AAVMF QEMU firmware on both x86-64 and AArch64. CI verifies the complete chain from firmware to Boot Manager, kernel loader, Nux kernel, user-mode entry, syscall, user-thread exit, and `NUX:READY`. The same workflow also builds and packages UEFI ISO and removable-media images. The macOS Clang/LLD cross-build passes for both architectures.

Explicit initial limits:
- 4 GiB identity-mapped physical window for loader execution;
- 64 GiB high-half direct-map window for kernel physical memory;
- 64 MiB maximum ELF kernel span;
- 256 memory-map region entries.

These are implementation limits, not claims of unlimited memory support. The kernel and loader still require physical-machine qualification.

## Remaining boot work

- [ ] Versioned, size-checked handoff ABI and explicit memory reservations.
- [ ] Signed boot manifest and kernel-image signature verification.
- [ ] Secure Boot key enrollment and owner-controlled trust policy.
- [ ] Boot-entry configuration, graphical boot picker, recovery entry, and fallback policy.
- [ ] Early firmware console, serial, and framebuffer diagnostics.
- [ ] Device Tree support where ACPI is not available.
- [ ] Physical-machine boot qualification on x86-64 and AArch64.
- [ ] Larger identity map or removal of the loader's 4 GiB identity constraint.
- [ ] Recovery after kernel validation, memory-map, or handoff failures.

A successful QEMU boot validates the tested firmware and kernel path; it does not establish Secure Boot, hardware compatibility, arbitrary user workloads, or production readiness.