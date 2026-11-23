# Lenix First-Party Boot Architecture

## Requirement

Lenix owns its complete operating-system boot path. The production boot path must not depend on Limine, GRUB, systemd-boot, rEFInd, or another third-party boot manager/loader. UEFI is the firmware interface on supported PC hardware; it is not a bundled third-party bootloader.

The design takes inspiration from the staged, integrated startup model used by macOS. It does not copy Apple's proprietary boot.efi, iBoot, BootPolicy, APFS Preboot, kernel collection, or Secure Enclave implementation.

## Boot stages

1. **Platform firmware** — UEFI initializes the machine and loads the architecture-matched Lenix EFI application from the EFI System Partition.
2. **Lenix Boot Manager** — a first-party, architecture-specific EFI application. It owns boot policy, boot-entry selection, configuration parsing, diagnostics, and recovery selection.
3. **Image verification** — verify a signed Lenix boot manifest and every referenced boot artifact before execution. The trust root is controlled by the machine owner or an enrolled Lenix key; firmware Secure Boot policy remains firmware-controlled.
4. **Kernel and boot assets** — load the XCU/Nux kernel and explicitly declared boot assets, validate image bounds and architecture, reserve their memory, and construct a versioned handoff record.
5. **Firmware handoff** — obtain the final UEFI memory map, capture ACPI/SMBIOS configuration-table pointers, call ExitBootServices with the matching map key, and never call boot services again.
6. **Architecture entry** — assembly normalizes processor state, establishes the kernel stack and exception vectors, and transfers to the C kernel entry with the versioned handoff record.
7. **Nux initialization** — initialize physical memory, page tables, processors, capabilities, tasks, IPC, synchronization, clocks, and the initial user-space bootstrap.
8. **Lenix system startup** — launch the first-party service manager, then device, filesystem, network, login, and user-session services. These are user-space services, not bootloader code.

## Supported platform contract

- x86-64 UEFI on x86-64 machines.
- AArch64 UEFI on AArch64 machines.
- No 32-bit x86 or 32-bit ARM boot target.
- Native Apple Silicon startup is not implied by AArch64 UEFI support. Apple hardware has a distinct platform boot/security environment and requires a separately engineered platform port.
- No claim of Apple Secure Boot compatibility. Lenix image verification and key enrollment are its own design.

## First-party source ownership

All Lenix boot manager, manifest, image-loading, handoff, and architecture-entry implementation is original Lenix C and assembly. Build scripts may use standard host toolchains. The firmware's UEFI interfaces and standard executable formats are specifications, not imported bootloader implementations.

## Handoff invariants

The boot manager must provide Nux with:
- a versioned structure with size and signature fields;
- the final firmware memory map and descriptor size/version;
- all kernel and boot-asset physical ranges reserved from the allocator;
- ACPI RSDP and optional SMBIOS table pointers;
- framebuffer information when present;
- the boot device and selected entry identity;
- verified-image state and manifest digest;
- architecture, firmware, and boot protocol revisions.

The kernel must reject an unknown handoff version, invalid descriptor sizes, arithmetic overflow, overlapping untrusted ranges, missing required memory-map data, or a kernel image whose architecture/entry point does not match the running machine.

## Migration policy

The existing Limine path is a temporary development dependency and is not the target architecture. It must not be described as the Lenix boot implementation. Limine references, protocol requests, downloaded binaries, and packaging steps are removed only when the first-party EFI path has equivalent tested functionality on both architectures. The final CI gate must boot first-party EFI images under x86-64 and AArch64 UEFI firmware and assert that Nux reaches its user-space bootstrap milestone.

## Implementation gates

- [ ] Define and test the versioned first-party boot handoff ABI.
- [ ] Implement the x86-64 EFI entry and first-party boot manager.
- [ ] Implement the AArch64 EFI entry and first-party boot manager.
- [ ] Implement bounded ELF/PE image validation and physical-memory reservation.
- [ ] Implement manifest signature verification and recovery policy.
- [ ] Implement correct final memory-map capture and ExitBootServices retry handling.
- [ ] Establish architecture page tables/direct map before relying on physical-to-virtual mappings.
- [ ] Remove Limine protocol requests and all Limine packaging/download steps.
- [ ] Boot both first-party images in QEMU and validate on physical hardware.
- [ ] Launch the first-party system service manager from Nux.
