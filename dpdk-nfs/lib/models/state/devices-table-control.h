#ifndef _FWTBL_STUB_CONTROL_H_INCLUDED_
#define _FWTBL_STUB_CONTROL_H_INCLUDED_

#include "lib/state/devices-table.h"
#include "lib/models/str-descr.h"

void devtbl_set_layout(struct DevicesTable *tb);
void devtbl_reset(struct DevicesTable *tb);

#endif