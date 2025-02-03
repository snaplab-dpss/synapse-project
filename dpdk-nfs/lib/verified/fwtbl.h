#ifndef _FWTBL_H_INCLUDED_
#define _FWTBL_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#include "lib/verified/ether.h"

struct ForwardingTable;

int fwtbl_allocate(uint16_t max_devices, struct ForwardingTable **ft);
void fwtbl_fill(struct ForwardingTable *ft, const char *cfg_fname);
int fwtbl_lookup(struct ForwardingTable *ft, uint16_t src_dev, uint16_t *dst_dev, bool *is_internal, struct rte_ether_addr *dst_addr);

#endif //_FWTBL_H_INCLUDED_