NAME = BootLoader.bin

all: $(NAME)

$(NAME): BootLoader.asm
	nasm -o $@ $<

clean:
	rm -f $(NAME)
