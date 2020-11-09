NASM32 = nasm
GCC32 = gcc -c -m32 -ffreestanding
LD32 = ld -melf_i386 -T ../elf_i386.x -nostdlib -e Main -Ttext 0x10200
OBJCOPY32 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJ_DIR = Temp
SRC_DIR = Source

all: prepare Kernel32.bin

prepare:
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/EntryPoint.bin: $(SRC_DIR)/EntryPoint.s
	$(NASM32) -o $@ $<

dep:
	@echo === Make Dependency File ===
	make -C $(OBJ_DIR) -f ../Makefile InternalDependency
	@echo === Dependency Search Complete ===

ExecuteInternalBuild: dep
	make -C $(OBJ_DIR) -f ../Makefile Kernel32.elf

$(OBJ_DIR)/Kernel32.elf.bin: ExecuteInternalBuild
	$(OBJCOPY32) $(OBJ_DIR)/Kernel32.elf $@

Kernel32.bin: $(OBJ_DIR)/EntryPoint.bin $(OBJ_DIR)/Kernel32.elf.bin
	cat $^ > $@

clean:
	rm -f *.bin
	rm -f $(OBJ_DIR)/*.*

# assume below code execute inside OBJ_DIR
C_ENTRYPOINT_OBJ = Main.o
C_SRC = $(wildcard ../$(SRC_DIR)/*.c)
ASM_SRC = $(wildcard ../$(SRC_DIR)/*.asm)
C_OBJ = $(subst Main.o, , $(notdir $(patsubst %.c,%.o,$(C_SRC))))
ASM_OBJ = $(notdir $(patsubst %.asm,%.o,$(ASM_SRC)))

%.o: ../$(SRC_DIR)/%.c
	$(GCC32) -c $<

%.o: ../$(SRC_DIR)/%.asm
	$(NASM32) -f elf32 -o $@ $<

InternalDependency:
	$(GCC32) -MM $(C_SRC) > Dependency.dep

Kernel32.elf: $(C_ENTRYPOINT_OBJ) $(C_OBJ) $(ASM_OBJ)
	$(LD32) -o $@ $^

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
