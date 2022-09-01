
#if !defined(VM_HEADER_ASM)
#define VM_HEADER_ASM

#include "jump.h"
#include "lib.h"
#include "bc.h"

enum vm_asm_instr_type_t
{
  VM_ASM_INSTR_END,
  VM_ASM_INSTR_RAW,
  VM_ASM_INSTR_GET,
  VM_ASM_INSTR_SET,
  VM_ASM_INSTR_GETI,
  VM_ASM_INSTR_SETI,
};
typedef enum vm_asm_instr_type_t vm_asm_instr_type_t;

struct vm_asm_instr_t;
typedef struct vm_asm_instr_t vm_asm_instr_t;

struct vm_asm_instr_t
{
  size_t value;
  uint8_t type;
};

void vm_asm_strip(const char **src);
void vm_asm_stripln(const char **src);
int vm_asm_isdigit(char c);
int vm_asm_isword(char c);
int vm_asm_starts(const char *in, const char *test);
size_t vm_asm_word(const char *src);
vm_opcode_t vm_asm_read_int(const char **src);
vm_opcode_t vm_asm_read_reg(const char **src);
vm_asm_instr_t *vm_asm_read(const char **src, size_t *out_ns, size_t *out_ni);
vm_bc_buf_t vm_asm_link(vm_asm_instr_t *instrs, size_t ns, size_t ni);
vm_bc_buf_t vm_asm(const char *src);

#endif
