#!/bin/bash

sudo ifconfig enp0s29u1u1u2 up
sudo ./install/bin/bf_kpkt_mod_unload $SDE_INSTALL || true
sudo ./install/bin/bf_kdrv_mod_load $SDE_INSTALL