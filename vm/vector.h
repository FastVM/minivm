#pragma once

#include <vm/libc.h>

struct vec_s;

typedef struct vec_s *vec_t;

struct vec_s
{
  int length;
  int allocated;
  int size;
  char values[0];
};

static vec_t vec_new(int elem_size)
{
  int allocated = 4;
  vec_t ret = vm_mem_alloc(sizeof(struct vec_s) + elem_size * allocated);
  ret->length = 0;
  ret->size = elem_size;
  ret->allocated = elem_size * allocated;
  return ret;
}

static void vec_resize(vec_t *vecp, int ngrow)
{
  if ((*vecp)->length + (*vecp)->size * ngrow * 2 >= (*vecp)->allocated)
  {
    int nallocated = ((*vecp)->allocated + ngrow) * 4;
    vec_t next = vm_mem_alloc(nallocated);
    *next = **vecp;
    next->allocated = nallocated;
    for (int i = 0; i < (*vecp)->allocated; i++)
    {
      next->values[i] = (*vecp)->values[i];
    }
    vm_mem_free(*vecp);
    *vecp = next;
  }
}

#define vec_new(t) vec_new(sizeof(t))
#define vec_push(vec, value)                                     \
  (                                                              \
      {                                                          \
        vec_resize(&vec, 1);                                     \
        *(typeof(value) *)&(vec)->values[(vec)->length] = value; \
        (vec)->length += (vec)->size;                            \
      })
#define vec_set_size(vec, n) ((vec)->length = n * (vec)->size)
#define vec_pop(vec) ((vec)->length -= (vec)->size)
#define vec_del(vec) ((void)free(vec))
#define vec_get(vec, index) ((vec)->values + index * (vec)->size)
#define vec_size(vec) ((vec)->length / (vec)->size)
#define vec_last(vec) (vec_get(vec, vec_size(vec) - 1))
#define vec_load_pop(vec) (vec_pop(vec), vec_get(vec, vec_size(vec)))
#define vec_foreach(type, item, vec)                          \
  for (                                                       \
      type *item = (type *)(vec)->values;                     \
      (void *)item < (void *)((vec)->values + (vec)->length); \
      item += 1)
