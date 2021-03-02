#!/usr/bin/env bash

qemu-kvm -cpu host -cdrom ./build/iso/otrix.iso -nographic -s \
         -device virtio-serial -chardev file,path=/tmp/otrix-log,id=otrix-log \
         -device virtconsole,name=jobsfoo,chardev=otrix-log,name=c \
         -netdev user,id=n1 -device virtio-net-pci,netdev=n1 \
         -object filter-dump,id=f1,netdev=n1,file=net_out.pcap
