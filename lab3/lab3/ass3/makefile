ASM = nasm
ASMFLAGS = -f bin
SOURCE = boot.asm
IMAGE = boot.img

all: $(IMAGE)

$(IMAGE): $(SOURCE)
	$(ASM) $(ASMFLAGS) $(SOURCE) -o $(IMAGE)
	truncate -s 1440k $(IMAGE)

clean:
	rm -f $(IMAGE)

run: $(IMAGE)
	qemu-system-i386 -fda $(IMAGE)

.PHONY: all clean run
