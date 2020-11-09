NASM64 = nasm -f elf64
GCC64 = gcc -c -m64 -ffreestanding
LD64 = ld -melf_x86_64 -T ../elf_x86_64.x -nostdlib -e Main -Ttext 0x200000
OBJCOPY64 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJ_DIR = Temp
SRC_DIR = Source

all: prepare Kernel64.bin

prepare:
	mkdir -p $(OBJ_DIR)

dep:
	@echo === Make Dependency File ===
	make -C $(OBJ_DIR) -f ../Makefile InternalDependency
	@echo === Dependency Search Complete ===

ExecuteInternalBuild: dep
	make -C $(OBJ_DIR) -f ../Makefile Kernel64.elf

Kernel64.bin: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJ_DIR)/Kernel64.elf $@

clean:
	rm -f *.bin
	rm -f $(OBJ_DIR)/*.*

# assume below code execute inside OBJ_DIR
ENTRYPOINT_SRC = ../$(SRC_DIR)/EntryPoint.s
ENTRYPOINT_OBJ = EntryPoint.o
C_SRC = $(wildcard ../$(SRC_DIR)/*.c)
ASM_SRC = $(wildcard ../$(SRC_DIR)/*.asm)
C_OBJ = $(notdir $(patsubst %.c,%.o,$(C_SRC)))
ASM_OBJ = $(notdir $(patsubst %.asm,%.o,$(ASM_SRC)))

$(ENTRYPOINT_OBJ): $(ENTRYPOINT_SRC)
	$(NASM64) -o $@ $<

%.o: ../$(SRC_DIR)/%.c
	$(GCC64) -c $<

%.o: ../$(SRC_DIR)/%.asm
	$(NASM64) -o $@ $<

InternalDependency:
	$(GCC64) -MM $(C_SRC) > Dependency.dep

Kernel64.elf: $(ENTRYPOINT_OBJ) $(C_OBJ) $(ASM_OBJ)
	$(LD64) -o $@ $^

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
