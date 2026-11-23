# Nux architecture and boot contract

Nux supports only x86-64 and AArch64. Both platforms use the same firmware-to-kernel lifecycle:

1. UEFI firmware loads the architecture-matched Limine EFI application.
2. Limine loads the matching Nux ELF kernel and supplies the memory map and HHDM.
3. The architecture entry stub masks interrupts, establishes ABI-safe stack state, and normalizes processor privilege state.
4. Nux installs its exception vectors and enters the common C kernel entry.
5. Common initialization establishes physical memory, virtual memory, tasks, capabilities, threads, IPC, synchronization, syscalls, and timekeeping.
6. The kernel marks itself ready and starts user-space bootstrap only after interrupt delivery and user-mode execution are implemented.

The design follows the shape of the macOS/XNU boot chain—firmware, boot manager, kernel entry, platform initialization, kernel services, then user-space bootstrap—but does not use Apple's proprietary boot.efi, kernel collections, or XNU code. Limine is the current portable UEFI boot manager for both supported architectures.

## Architecture handoff

- x86-64: Limine enters the 64-bit kernel using the System V ABI. Nux disables interrupts, clears the direction flag, aligns the stack, installs the IDT, and passes a normalized nux_boot_info record to C.
- AArch64: Limine base revision 6 can enter at EL1 or EL2 with VHE. Nux detects EL2, transfers the boot stack to SP_EL1, configures EL1 counter access, exits EL2 to EL1h, masks interrupts, installs VBAR_EL1, and enters the common C path.
- VM isolation: the kernel remains mapped through TTBR1_EL1; TTBR0_EL1 is reserved for user address spaces. The default TTBR0 root is empty, rather than reusing firmware mappings.

## Boot diagnostics

nux_boot_status is a debugger-visible word. Its upper bits contain the NUX marker and current phase; the low 32 bits contain a failure code when the phase is FAILED. It is updated before each initialization subsystem, so a halted machine can be inspected without a console driver.

The boot protocol request region is explicitly delimited and retained by both linker scripts. The request segment remains writable because Limine writes response pointers into the request objects.

## Boot limitations

The current boot contract is a Limine handoff, not a native Apple bootloader implementation. The repository now builds UEFI ISO and removable-media images for both targets. x86-64 and AArch64 UEFI boot both reach NUX:READY in QEMU; the AArch64 diagnostic image uses semihosting. The macOS Clang/LLD cross-build is verified. A user-space init/bootstrap executable, a real console, timer/interrupt-controller setup, and physical-hardware qualification remain. An ELF cross-build alone does not prove that firmware can boot the image.
