#pragma once

#include "io.h"
#include "jump.h"
#include "lib.h"
#include "vm.h"

enum vm_asm_instr_type_t;
typedef enum vm_asm_instr_type_t vm_asm_instr_type_t;

struct vm_asm_instr_t;
typedef struct vm_asm_instr_t vm_asm_instr_t;

struct vm_asm_buf_t;
typedef struct vm_asm_buf_t vm_asm_buf_t;

enum vm_asm_instr_type_t
{
  VM_ASM_INSTR_END,
  VM_ASM_INSTR_RAW,
  VM_ASM_INSTR_GET,
  VM_ASM_INSTR_SET,
};

struct vm_asm_instr_t
{
  uint8_t type;
  size_t value;
};

struct vm_asm_buf_t
{
  vm_opcode_t *ops;
  size_t nops;
};

const char *vm_asm_io_read(const char *filename);
void vm_asm_strip(const char **src);
void vm_asm_stripln(const char **src);
int vm_asm_isdigit(char c);
int vm_asm_isword(char c);
int vm_asm_starts(const char *in, const char *test);
size_t vm_asm_word(const char *src);
vm_opcode_t vm_asm_read_int(const char **src);
vm_opcode_t vm_asm_read_reg(const char **src);
vm_asm_instr_t *vm_asm_read(const char *src, size_t *out);
vm_asm_buf_t vm_asm_link(vm_asm_instr_t *instrs, size_t n);
vm_asm_buf_t vm_asm(const char *src);