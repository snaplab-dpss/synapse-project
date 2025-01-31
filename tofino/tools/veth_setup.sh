#!/bin/bash

function add_hugepage() {
    sudo sh -c 'echo "#Enable huge pages support for DMA purposes" >> /etc/sysctl.conf'
    sudo sh -c 'echo "vm.nr_hugepages = 128" >> /etc/sysctl.conf'
}

function dma_setup() {
    echo "Setting up DMA Memory Pool"
    hp=$(sudo sysctl -n vm.nr_hugepages)

    if [ $hp -lt 128 ]; then
        if [ $hp -eq 0 ]; then
            add_hugepage
        else
            nl=$(egrep -c vm.nr_hugepages /etc/sysctl.conf)
            if [ $nl -eq 0 ]; then
                add_hugepage
            else
                sudo sed -i 's/vm.nr_hugepages.*/vm.nr_hugepages = 128/' /etc/sysctl.conf
            fi
        fi
        sudo sysctl -p /etc/sysctl.conf
    fi

    if [ ! -d /mnt/huge ]; then
        sudo mkdir /mnt/huge
    fi
    sudo mount -t hugetlbfs nodev /mnt/huge
}

function veth_setup() {
    noOfVeths=64
    echo "No of Veths is $noOfVeths"
    let "vethpairs=$noOfVeths/2"
    last=`expr $vethpairs - 1`
    veths=`seq 0 1 $last`
    # add CPU port
    veths+=" 125"

    for i in $veths; do
        intf0="veth$(($i*2))"
        intf1="veth$(($i*2+1))"
        if ! ip link show $intf0 &> /dev/null; then
            ip link add name $intf0 type veth peer name $intf1 &> /dev/null
        fi
        ip link set dev $intf0 up
        sysctl net.ipv6.conf.$intf0.disable_ipv6=1 &> /dev/null
        sysctl net.ipv6.conf.$intf1.disable_ipv6=1 &> /dev/null
        ip link set dev $intf0 up mtu 10240
        ip link set dev $intf1 up mtu 10240
        TOE_OPTIONS="rx tx sg tso ufo gso gro lro rxvlan txvlan rxhash"
        for TOE_OPTION in $TOE_OPTIONS; do
        /sbin/ethtool --offload $intf0 "$TOE_OPTION" off &> /dev/null
        /sbin/ethtool --offload $intf1 "$TOE_OPTION" off &> /dev/null
        done
    done
}

veth_setup
dma_setup