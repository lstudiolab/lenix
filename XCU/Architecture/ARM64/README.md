# AArch64 Port

Target: AArch64 / ARM64 only.

Assembly source will use GNU assembler syntax and the AAPCS64 C calling convention where applicable. The port must explicitly model exception levels, vector-table layout, system registers, cache/TLB maintenance, and the platform boot protocol.

Nux must not assume that every AArch64 machine starts at the same exception level or has the same interrupt controller. Platform discovery and interrupt-controller support belong behind platform interfaces.

Planned assembly areas: `Boot/`, `CPU/`, `Exceptions/`, `Interrupts/`, `Context/`, and `MMU/`.
