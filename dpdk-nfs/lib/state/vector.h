#include <stdint.h>

#ifndef _VECTOR_H_INCLUDED_
#define _VECTOR_H_INCLUDED_

#include "lib/util/time.h"

#define VECTOR_CAPACITY_UPPER_LIMIT 140000

struct Vector;

int vector_allocate(int elem_size, unsigned capacity, struct Vector **vector_out);

void vector_borrow(struct Vector *vector, int index, void **val_out);

void vector_return(struct Vector *vector, int index, void *value);

void vector_clear(struct Vector *vector);

// For a vector whose cells are a key of `key_size` bytes followed by an unsigned little-endian
// value of `value_size` bytes. `pair` is such a cell. If the cell at `index` holds the pair's key,
// the pair's value is added to the cell's; otherwise, if the cell's value is below the pair's or
// `evict` is set, the cell and the pair change places. Either way `pair` comes back holding what
// is NOT in the cell afterwards: all zeros after an increment, the pair itself when the cell keeps
// its own.
void vector_inc_or_swap(struct Vector *vector, int index, void *pair, unsigned key_size, unsigned value_size, int evict);

int vector_periodic_clear(struct Vector *vector, time_ns_t now, time_ns_t interval);

// Randomly sample an element from the vector and compare it with the provided
// threshold value.
// Little endian byte by byte comparison (so it doesn't work for signed
// integers)
int vector_sample_lt(struct Vector *vector, int samples, void *threshold, int *index_out);

#endif //_VECTOR_H_INCLUDED_
