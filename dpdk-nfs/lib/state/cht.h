#ifndef _CHT_H_INCLUDED_
#define _CHT_H_INCLUDED_

#include "lib/state/double-chain.h"
#include "lib/state/vector.h"

struct CHT;

int cht_allocate(uint32_t height, uint32_t key_size, struct CHT **cht_out);
int cht_add_backend(struct CHT *cht, int backend);
int cht_remove_backend(struct CHT *cht, int backend);
int cht_find_backend(struct CHT *cht, void *key, int *chosen_backend_out);

#endif //_CHT_H_INCLUDED_
