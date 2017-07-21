iso=build/otrix.iso
kernel=build/otrix.bin
kernel_objects=build/arch/x86_64/boot.o build/arch/x86_64/multiboot.o

.PHONY: all clean run install

all: $(kernel)

clean:
	rm -r build

run: $(iso)
	kvm -cdrom $(iso)

$(iso): $(kernel)
	mkdir -p build/isofiles/boot/grub
	cp $(kernel) build/isofiles/boot/
	cp arch/x86_64/grub.cfg build/isofiles/boot/grub/
	grub-mkrescue -o $(iso) build/isofiles
	rm -r build/isofiles

$(kernel): $(kernel_objects) arch/x86_64/linker.ld
	ld -n -T arch/x86_64/linker.ld -o $(kernel) $(kernel_objects)

build/arch/x86_64/%.o: arch/x86_64/%.asm
	mkdir -p $(shell dirname $@)
	nasm -felf64 $< -o $@