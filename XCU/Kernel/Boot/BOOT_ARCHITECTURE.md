# Lenix First-Party Boot Architecture

## Requirement

Lenix owns its complete operating-system boot path. The boot chain uses only Lenix-owned EFI applications and kernel code after the platform's standard UEFI firmware handoff. It does not bundle or depend on Limine, GRUB, systemd-boot, rEFInd, or another third-party boot manager/loader.

The staged design takes inspiration from the integrated startup model of macOS. It does not copy Apple's proprietary boot.efi, iBoot, BootPolicy, APFS Preboot, kernel collection, or Secure Enclave implementation.

## Boot stages

1. **Platform firmware** — UEFI initializes the machine and loads the architecture-matched Lenix EFI application from the EFI System Partition.
2. **Lenix Boot Manager** — `BOOTX64.EFI` or `BOOTAA64.EFI` validates the PE32+ EFI child image and starts `\\EFI\\LENIX\\NUX.EFI`.
3. **Nux Kernel Loader** — the second first-party EFI application opens `\\EFI\\LENIX\\NUX.ELF`, validates ELF64 class, endianness, machine, program-header bounds, segment ranges, and executable entry point, then reserves physical pages and copies PT_LOAD segments.
4. **Memory and address-space preparation** — the loader reserves a dedicated 64 KiB boot stack, constructs x86-64 four-level page tables or AArch64 TTBR0/TTBR1 tables, maps the first 4 GiB identity range for the loader context, maps a 64 GiB high-half direct-map window, and maps the high-half kernel image.
5. **Firmware handoff** — the loader captures the final UEFI memory map, converts conventional-memory descriptors into Nux memory-region records, discovers the ACPI 2.0 RSDP, retries `ExitBootServices` with a fresh map key when needed, and makes no firmware calls after a successful exit.
6. **Architecture entry** — the loader switches to the high direct-map boot stack and jumps to `_start_uefi`. Assembly preserves the versioned boot-info pointer, establishes the expected stack alignment, initializes architecture entry state, and calls `nux_kernel_main`.
7. **Nux initialization** — Nux validates the boot-info signature, version, structure size, architecture, memory map, and direct-map contract before initializing physical memory, ACPI/topology, virtual memory, processors, tasks, capabilities, threads, IPC, synchronization, syscalls, and time.
8. **User-mode validation** — CI executes a user ELF on both architectures, exercises the syscall exit path, and checks that Nux reaches `NUX:READY`.

## Supported platform contract

- x86-64 UEFI on x86-64 machines.
- AArch64 UEFI on AArch64 machines.
- No 32-bit x86 or 32-bit ARM boot target.
- UEFI firmware is the platform firmware interface, not a third-party bootloader.
- Native Apple Silicon startup is not implied by AArch64 UEFI support. Apple hardware has a distinct platform boot/security environment and requires a separately engineered platform port.
- No claim of Apple Secure Boot compatibility or signed Lenix image verification.

## First-party source ownership

The Boot Manager, PE32+ validator, ELF64 loader, page-table setup, boot-info handoff, and architecture entry code are original Lenix C and assembly. Build configuration uses standard host toolchains and image utilities. UEFI interfaces and PE/COFF and ELF formats are public platform/file-format specifications, not imported bootloader implementations.

## Current handoff ABI

The shared `nux_boot_info` structure is versioned and contains:
- signature, version, and structure size;
- target architecture and validation flags;
- pointer and count for Nux memory-region records;
- high-half direct-map base and size;
- ACPI RSDP pointer, when firmware provides it;
- kernel physical base, virtual base, and image size.

The memory-region pointer array, its entries, the boot-info structure, and ACPI root pointer are supplied through the direct map. The kernel rejects missing or mismatched signature/version/size/architecture fields, absent memory-map data, a mismatched direct-map contract, and an invalid kernel base/image size.

## Current implementation limits

- Physical direct map: first 64 GiB.
- Identity map retained for EFI loader execution: first 4 GiB.
- Kernel physical allocation and initial boot stack: below 4 GiB.
- Kernel ELF image: at most 64 MiB.
- Memory-map records: at most 256 regions.
- Initial kernel image mappings are not yet enforced as W^X per ELF segment.
- MMIO cache attributes and a dedicated device-memory mapping policy are not complete.
- The boot manager validates image structure and architecture but does not yet verify a signed boot manifest.
- Secure Boot key enrollment, recovery UI, boot-entry configuration UI, framebuffer diagnostics, SMBIOS handoff, and Device Tree support remain unfinished.

These are explicit initial implementation limits. The loader fails on invalid or unsupported inputs rather than silently claiming a broader hardware contract.

## Verified status (2026-11-23)

The production first-party boot path has been exercised in QEMU with OVMF and AAVMF firmware on both supported architectures. In each case:
- firmware starts the Lenix Boot Manager;
- the manager validates and starts the Nux Kernel Loader EFI application;
- the loader reads and validates the architecture-matched Nux ELF;
- the loader captures the memory map and exits UEFI boot services;
- the kernel starts through its high-half first-party entry;
- the user-mode smoke ELF enters user mode, performs its exit syscall, and returns to the kernel;
- the boot log reaches `NUX:READY`.

The CI also retains a smaller EFI-to-EFI smoke test for diagnosing firmware and Boot Manager regressions. The full-kernel test is the acceptance gate for the real boot chain.

## Remaining work

- [ ] Signed manifest, cryptographic verification, key enrollment, and Secure Boot integration.
- [ ] Recovery image/boot entry and boot policy UI.
- [ ] Per-segment W^X kernel mappings and stricter ELF relocation/overlap validation.
- [ ] Full physical-memory coverage beyond the initial 64 GiB direct-map window.
- [ ] MMIO/device-memory mappings with correct cache attributes.
- [ ] Firmware framebuffer and SMBIOS handoff.
- [ ] Device Tree support for platforms without ACPI.
- [ ] Physical-machine boot qualification on both architectures.
- [ ] First-party user-space bootstrap and system service manager.
