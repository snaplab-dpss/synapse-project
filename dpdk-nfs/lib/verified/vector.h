#ifndef _VECTOR_H_INCLUDED_
#define _VECTOR_H_INCLUDED_

#define VECTOR_CAPACITY_UPPER_LIMIT 140000

struct Vector;

int vector_allocate(int elem_size, unsigned capacity,
                    struct Vector **vector_out);

void vector_borrow(struct Vector *vector, int index, void **val_out);

void vector_return(struct Vector *vector, int index, void *value);

void vector_clear(struct Vector *vector);

#endif //_VECTOR_H_INCLUDED_
