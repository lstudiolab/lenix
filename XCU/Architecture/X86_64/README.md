# x86-64 Port

Target: x86-64 long mode only.

Assembly source will use GNU assembler syntax. The port must explicitly define the boot handoff contract, descriptor tables, privilege transitions, interrupt/trap gates, syscall entry, per-CPU state, context switching, control-register operations, and TLB invalidation.

The kernel must not assume that firmware, bootloader, or emulator setup has already configured every required CPU feature. Required features must be checked before use.

Planned assembly areas: `Boot/`, `CPU/`, `Exceptions/`, `Interrupts/`, `Context/`, and `MMU/`.
