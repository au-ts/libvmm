CROSS_COMPILE ?= riscv64-unknown-elf-
ARCH_GENERIC_FLAGS = -mcmodel=medany -march=rv64imac -mabi=lp64
ARCH_ASFLAGS = 
ARCH_CFLAGS = 
ARCH_CPPFLAGS =	
ARCH_LDFLAGS = --specs=nano.specs
