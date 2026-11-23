CC_X86 ?= x86_64-linux-gnu-gcc
AS_X86 ?= x86_64-linux-gnu-gcc
CC_ARM ?= aarch64-linux-gnu-gcc
AS_ARM ?= aarch64-linux-gnu-gcc

COMMON_CFLAGS := -std=gnu11 -ffreestanding -fno-stack-protector -fno-pic -fno-pie -fno-builtin -fno-unwind-tables -fno-asynchronous-unwind-tables -Wall -Wextra -Werror -O2 -I Nux/Include -I Kernel/Boot -I Kernel/Entry
LDFLAGS := -nostdlib -static -no-pie -Wl,--build-id=none
C_SOURCES := Kernel/Boot/limine.c Kernel/Initialization/main.c Nux/Memory/physical.c Nux/Task/task.c Nux/Capability/capability.c

.PHONY: all headers x86_64 aarch64 clean
all: headers x86_64 aarch64

headers:
\t$(CC_X86) $(COMMON_CFLAGS) -fsyntax-only Nux/Tests/header_check.c
\t$(CC_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -fsyntax-only Nux/Tests/header_check.c

x86_64:
	mkdir -p Build/x86_64
	@for source in $(C_SOURCES); do object=Build/x86_64/$$(echo $$source | tr '/.' '__').o; $(CC_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c $$source -o $$object || exit 1; done
	$(AS_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/Boot/entry.S -o Build/x86_64/entry.o
	$(AS_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/CPU/early.S -o Build/x86_64/early.o
	$(AS_X86) $(COMMON_CFLAGS) -mno-red-zone -mcmodel=kernel -c Architecture/X86_64/CPU/lock.S -o Build/x86_64/lock.o
	$(CC_X86) $(LDFLAGS) -Wl,-T,Kernel/Linker/x86_64.ld -o Build/x86_64/nux.elf Build/x86_64/*.o

aarch64:
	mkdir -p Build/aarch64
	@for source in $(C_SOURCES); do object=Build/aarch64/$$(echo $$source | tr '/.' '__').o; $(CC_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c $$source -o $$object || exit 1; done
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/Boot/entry.S -o Build/aarch64/entry.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/early.S -o Build/aarch64/early.o
	$(AS_ARM) $(COMMON_CFLAGS) -mgeneral-regs-only -c Architecture/ARM64/CPU/lock.S -o Build/aarch64/lock.o
	$(CC_ARM) $(LDFLAGS) -Wl,-T,Kernel/Linker/aarch64.ld -o Build/aarch64/nux.elf Build/aarch64/*.o

clean:
	rm -rf Build
