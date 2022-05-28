#pragma once

struct vm_io_res_t;
typedef struct vm_io_res_t vm_io_res_t;

#include "opcode.h"

struct vm_io_res_t
{
  size_t nops;
  vm_opcode_t *ops;
};

static inline vm_io_res_t vm_io_read(const char *filename)
{
  void *file = fopen(filename, "rb");
  if (file == NULL)
  {
    fprintf(stderr, "cannot run vm: file could not be read\n");
    return (vm_io_res_t) {
      .nops = 0,
      .ops = NULL,
    };
  }
  size_t nalloc = 1 << 8;
  vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * nalloc);
  size_t nops = 0;
  size_t size;
  for (;;)
  {
    size = fread(&ops[nops], sizeof(vm_opcode_t), 1, file);
    if (size == 0)
    {
      break;
    }
    nops += 1;
    if (nops + 4 >= nalloc)
    {
      nalloc *= 4;
      ops = vm_realloc(ops, sizeof(vm_opcode_t) * nalloc);
    }
  }
  fclose(file);
  return (vm_io_res_t){
      .nops = nops,
      .ops = ops,
  };
}