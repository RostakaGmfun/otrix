#!/usr/bin/env bash

qemu-kvm -cdrom ./build/iso/otrix.iso -nographic \
         -device virtio-serial -chardev file,path=/tmp/otrix-log,id=otrix-log \
         -device virtconsole,name=jobsfoo,chardev=otrix-log,name=c \
         -netdev user,id=n1 -device virtio-net-pci,netdev=n1
