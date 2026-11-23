CC_X86 ?= x86_64-linux-gnu-gcc
AS_X86 ?= x86_64-linux-gnu-gcc
CC_ARM ?= aarch64-linux-gnu-gcc
AS_ARM ?= aarch64-linux-gnu-gcc

COMMON_CFLAGS := -std=gnu11 -ffreestanding -fno-stack-protector -fno-pic -fno-pie -fno-builtin -fno-unwind-tables -fno-asynchronous-unwind-tables -Wall -Wextra -Werror -O2 -I Nux/Include -I Kernel/Boot -I Kernel/Entry
LDFLAGS := -nostdlib -static -no-pie -Wl,--build-id=none

.PHONY: all x86_64 aarch64 clean
all: x86_64 aarch64

x86_64:
	mkdir -p Build/x86_64
	$(CC_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Kernel/Boot/limine.c -o Build/x86_64/limine.o
	$(CC_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Kernel/Initialization/main.c -o Build/x86_64/main.o
	$(CC_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Nux/Memory/physical.c -o Build/x86_64/physical.o
	$(AS_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/Boot/entry.S -o Build/x86_64/entry.o
	$(AS_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/CPU/early.S -o Build/x86_64/early.o
	$(CC_X86) $(LDFLAGS) -Wl,-T,Kernel/Linker/x86_64.ld -o Build/x86_64/nux.elf Build/x86_64/entry.o Build/x86_64/early.o Build/x86_64/main.o Build/x86_64/physical.o Build/x86_64/limine.o

aarch64:
	mkdir -p Build/aarch64
	$(CC_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Kernel/Boot/limine.c -o Build/aarch64/limine.o
	$(CC_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Kernel/Initialization/main.c -o Build/aarch64/main.o
	$(CC_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Nux/Memory/physical.c -o Build/aarch64/physical.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/Boot/entry.S -o Build/aarch64/entry.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/early.S -o Build/aarch64/early.o
	$(CC_ARM) $(LDFLAGS) -Wl,-T,Kernel/Linker/aarch64.ld -o Build/aarch64/nux.elf Build/aarch64/entry.o Build/aarch64/early.o Build/aarch64/main.o Build/aarch64/physical.o Build/aarch64/limine.o

clean:
	rm -rf Build
