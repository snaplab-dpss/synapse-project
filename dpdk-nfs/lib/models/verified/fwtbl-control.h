#ifndef _FWTBL_STUB_CONTROL_H_INCLUDED_
#define _FWTBL_STUB_CONTROL_H_INCLUDED_

#include "lib/verified/fwtbl.h"
#include "lib/models/str-descr.h"

void fwtbl_set_layout(struct ForwardingTable *tb);
void fwtbl_reset(struct ForwardingTable *tb);

#endif