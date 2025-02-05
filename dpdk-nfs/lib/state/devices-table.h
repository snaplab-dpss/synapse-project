#ifndef _FWTBL_H_INCLUDED_
#define _FWTBL_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#include "lib/util/ether.h"

#define DEVTBL_CONFIG_FNAME_LEN 512

struct DevicesTable;

int devtbl_allocate(uint16_t max_devices, struct DevicesTable **ft);

// Fill the devices table with the devices and their MAC addresses.
// This is parsed from a configuration file "cfg_fname".
// The configuration file has lines with the following format: "<device> <mac addr>"
void devtbl_fill(struct DevicesTable *ft, const char *cfg_fname);
int devtbl_lookup(struct DevicesTable *ft, uint16_t dev, struct rte_ether_addr *mac);

#endif //_FWTBL_H_INCLUDED_