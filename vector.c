#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct vec_s;

typedef struct vec_s *vec_t;

struct vec_s {
  int length;
  int allocated;
  char values[0];
};

vec_t vec_new(int elem_size) {
  int allocated = 4;
  vec_t ret = malloc(sizeof(struct vec_s) + elem_size * allocated);
  ret->length = 0;
  ret->allocated = allocated;
  return ret;
}

void vec_resize(int elem_size, vec_t *vecp, int ngrow) {
  if ((*vecp)->length * elem_size + ngrow * elem_size >= (*vecp)->allocated) {
    int nallocated = ((*vecp)->allocated + ngrow * elem_size) * 4;
    *vecp = realloc(*vecp, nallocated);
    (*vecp)->allocated = nallocated;
  }
}

#define vec_new(t) vec_new(sizeof(t))
#define vec_push(type, vec, value, cons)                                       \
  ({                                                                           \
    if (vec == NULL) {                                                         \
      vec = vec_new(sizeof(type));                                             \
    }                                                                          \
    vec_resize(sizeof(type), &vec, 1);                                         \
    ((type *)vec->values)[vec->length++] = cons(value);                        \
  })
#define vec_pop(type, vec, destroy)                                            \
  ({ destroy(((type *)vec->values)[--vec->length]); })
#define vec_del(type, vec, destroy)                                            \
  ({                                                                           \
    for (int i = 0; i < vec->length; i++) {                                    \
      destroy(((type *)vec->values)[i]);                                       \
    }                                                                          \
    free(vec);                                                                 \
  })
#define vec_get(type, vec, index) ((type *)(vec)->values)[index]

int main() {
  vec_t strs = vec_new(int *);
  for (int i = 0; i < 100000000; i++) {
    vec_push(char *, strs, "test_", strdup);
  }
  printf("%i\n", strs->length);
  for (int i = 0; i < 100000000; i++) {
    vec_pop(char *, strs, free);
  }
  vec_del(int *, strs, free);
}