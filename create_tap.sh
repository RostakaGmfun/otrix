#!/usr/bin/env bash

ip tuntap add dev otrix_tap mode tap
ip link add dev br0 type bridge
ip link add dev lo0 type dummy
ip link set dev otrix_tap master br0
ip link set dev lo0 master br0
ip addr add 20.0.0.1/24 dev br0
ip link set dev br0 up
ip link set dev lo0 up
ip link set dev otrix_tap up
