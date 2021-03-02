TOOLCHAIN_PATH   ?= /data/x86_64-unknown-elf
TOOLCHAIN_PREFIX ?= x86_64-unknown-elf-

all: build
	@make -sC build
	@echo "Verifying if binary conforms GRUB Multiboot2"
	@grub-file --is-x86-multiboot2 build/kernel/otrix_kernel
	@mkdir -p build/isofiles/boot/grub
	@mkdir -p build/iso
	@cp build/kernel/otrix_kernel build/isofiles/boot/
	@cp arch/x86_64/grub.cfg build/isofiles/boot/grub/
	@grub-mkrescue -o build/otrix.iso build/isofiles

test: tests_build
	@make -sC tests_build
	@make -sC tests_build test

.PHONY: clean
clean:
	@make -sC build clean

tests_build:
	@if [ ! -e "tests_build/Makefile" ]; then \
		rm -rf tests_build; \
		mkdir tests_build;\
		cd tests_build && cmake -DBUILD_HOST_TESTS=ON ../; \
	fi

build:
	@if [ ! -e "build/Makefile" ]; then \
		rm -rf build; \
		mkdir build;\
		cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-x86.cmake -DTOOLCHAIN_PATH=$(TOOLCHAIN_PATH) -DTOOLCHAIN_PREFIX=$(TOOLCHAIN_PREFIX) ../; \
	fi
