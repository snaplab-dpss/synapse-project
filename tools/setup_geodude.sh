#!/bin/bash

sudo modprobe uio

pushd /opt/dpdk-kmods/linux/igb_uio
    make clean
    make
    sudo insmod igb_uio.ko
popd

sudo dpdk-devbind.py -b igb_uio 0000:af:00.0 0000:af:00.1
sudo dpdk-hugepages.py --setup 20G