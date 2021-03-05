#!/usr/bin/env bash

ip tuntap add dev otrix_tap mode tap user $(id -u) group $(id -g)
ip link set dev otrix_tap up
