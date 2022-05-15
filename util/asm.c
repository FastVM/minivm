#include "../vm/io.h"
#include "../vm/jump.h"
#include "../vm/lib.h"
#include "../vm/vm.h"

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

const char *vm_asm_io_read(const char *filename)
{
  FILE *file = fopen(filename, "rb");
  if (file == NULL)
  {
    return NULL;
  }
  size_t nalloc = 16;
  char *ops = vm_malloc(sizeof(char) * nalloc);
  size_t nops = 0;
  size_t size;
  for (;;)
  {
    char op = 0;
    size = fread(&op, sizeof(char), 1, file);
    if (size == 0)
    {
      break;
    }
    if (nops + 2 >= nalloc)
    {
      nalloc *= 4;
      ops = vm_realloc(ops, sizeof(char) * nalloc);
    }
    ops[nops++] = (size_t)op;
  }
  ops[nops] = '\0';
  fclose(file);
  return ops;
}

void vm_asm_strip(const char **src)
{
  while (**src == ' ' || **src == '\t')
  {
    *src += 1;
  }
}

void vm_asm_stripln(const char **src)
{
  while (**src == ' ' || **src == '\t' || **src == '\n' || **src == '\r' || **src == ';')
  {
    *src += 1;
  }
}

int vm_asm_isdigit(char c)
{
  return ('0' <= c && c <= '9');
}

int vm_asm_isword(char c)
{
  return vm_asm_isdigit(c) || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

int vm_asm_starts(const char *in, const char *test)
{
  while (*test)
  {
    if (*test != *in)
    {
      return 0;
    }
    test += 1;
    in += 1;
  }
  return 1;
}

size_t vm_asm_word(const char *src)
{
  size_t n = 0;
  while (vm_asm_isword(src[n]))
  {
    n += 1;
  }
  return n;
}

vm_opcode_t vm_asm_read_int(const char **src)
{
  vm_asm_strip(src);
  size_t n = 0;
  while ('0' <= **src && **src <= '9')
  {
    n *= 10;
    n += **src - '0';
    *src += 1;
  }
  return n;
}

vm_opcode_t vm_asm_read_reg(const char **src)
{
  vm_asm_strip(src);
  *src += 1;
  size_t n = 0;
  while ('0' <= **src && **src <= '9')
  {
    n *= 10;
    n += **src - '0';
    *src += 1;
  }
  return n;
}

#define vm_asm_put(type_, value_) ({instrs[head] = (vm_asm_instr_t) {.type = (type_),.value = (value_),};head += 1;instrs[head] = (vm_asm_instr_t) {.type = VM_ASM_INSTR_END,}; })
#define vm_asm_put_op(op_) vm_asm_put((VM_ASM_INSTR_RAW), (op_))
#define vm_asm_put_reg(reg_) vm_asm_put((VM_ASM_INSTR_RAW), (reg_))
#define vm_asm_put_int(int_) vm_asm_put((VM_ASM_INSTR_RAW), (int_))
#define vm_asm_put_set(name_) vm_asm_put((VM_ASM_INSTR_SET), (size_t)(name_))
#define vm_asm_put_get(name_) vm_asm_put((VM_ASM_INSTR_GET), (size_t)(name_))

vm_asm_instr_t *vm_asm_read(const char *src)
{
  size_t alloc = 32;
  vm_asm_instr_t *instrs = vm_malloc(sizeof(vm_asm_instr_t) * alloc);
  size_t head = 0;
  vm_asm_put_op(VM_OPCODE_JUMP);
  vm_asm_put_get("main");
  for (;;)
  {
    if (head + 16 > alloc)
    {
      alloc = head * 4 + 16;
      instrs = vm_realloc(instrs, sizeof(vm_asm_instr_t) * alloc);
    }
    vm_asm_stripln(&src);
    if (*src == '\0')
    {
      break;
    }
    if (*src == '@')
    {
      src += 1;
      vm_asm_strip(&src);
      vm_asm_put_set(src);
      src += vm_asm_word(src);
      continue;
    }
    else if (*src == 'r' && '0' <= src[1] && src[1] <= '9')
    {
      size_t regn = vm_asm_word(src);
      vm_opcode_t regno = 0;
      int isreg = 1;
      for (size_t i = 1; i < regn; i++)
      {
        if (!vm_asm_isdigit(src[i]))
        {
          isreg = false;
          break;
        }
        regno *= 10;
        regno += src[i] - '0';
      }
      if (isreg)
      {
        src += regn;
        vm_asm_strip(&src);
        if (!vm_asm_starts(src, "<-"))
        {
          goto err;
        }
        src += 2;
        vm_asm_strip(&src);
        const char *opname = src;
        src += vm_asm_word(src);
        if (vm_asm_starts(opname, "int"))
        {
          vm_asm_put_op(VM_OPCODE_INT);
          vm_asm_put_reg(regno);
          vm_asm_put_int(vm_asm_read_int(&src));
          continue;
        }
        if (vm_asm_starts(opname, "addr"))
        {
          vm_asm_put_op(VM_OPCODE_INTF);
          vm_asm_put_reg(regno);
          vm_asm_strip(&src);
          vm_asm_put_get(src);
          src += vm_asm_word(src);
          continue;
        }
        if (vm_asm_starts(opname, "reg"))
        {
          vm_asm_put_op(VM_OPCODE_REG);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(&src));
          continue;
        }
        if (vm_asm_starts(opname, "add"))
        {
          vm_asm_put_op(VM_OPCODE_ADD);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(&src));
          vm_asm_put_reg(vm_asm_read_reg(&src));
          continue;
        }
        if (vm_asm_starts(opname, "sub"))
        {
          vm_asm_put_op(VM_OPCODE_SUB);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(&src));
          vm_asm_put_reg(vm_asm_read_reg(&src));
          continue;
        }
        if (vm_asm_starts(opname, "mul"))
        {
          vm_asm_put_op(VM_OPCODE_MUL);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(&src));
          vm_asm_put_reg(vm_asm_read_reg(&src));
          continue;
        }
        if (vm_asm_starts(opname, "div"))
        {
          vm_asm_put_op(VM_OPCODE_DIV);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(&src));
          vm_asm_put_reg(vm_asm_read_reg(&src));
          continue;
        }
        if (vm_asm_starts(opname, "mod"))
        {
          vm_asm_put_op(VM_OPCODE_MOD);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(&src));
          vm_asm_put_reg(vm_asm_read_reg(&src));
          continue;
        }
        if (vm_asm_starts(opname, "pair"))
        {
          vm_asm_put_op(VM_OPCODE_PAIR);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(&src));
          vm_asm_put_reg(vm_asm_read_reg(&src));
          continue;
        }
        if (vm_asm_starts(opname, "first"))
        {
          vm_asm_put_op(VM_OPCODE_FIRST);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(&src));
          continue;
        }
        if (vm_asm_starts(opname, "second"))
        {
          vm_asm_put_op(VM_OPCODE_SECOND);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(&src));
          continue;
        }
      }
    }
    else
    {
      vm_asm_strip(&src);
      const char *opname = src;
      src += vm_asm_word(src);
      if (vm_asm_starts(opname, "exit"))
      {
        vm_asm_put_op(VM_OPCODE_EXIT);
        continue;
      }
      if (vm_asm_starts(opname, "ret"))
      {
        vm_asm_put_op(VM_OPCODE_RET);
        vm_asm_put_reg(vm_asm_read_reg(&src));
        continue;
      }
      if (vm_asm_starts(opname, "putchar"))
      {
        vm_asm_put_op(VM_OPCODE_PUTCHAR);
        vm_asm_put_reg(vm_asm_read_reg(&src));
        continue;
      }
      if (vm_asm_starts(opname, "jump"))
      {
        vm_asm_put_op(VM_OPCODE_JUMP);
        vm_asm_strip(&src);
        vm_asm_put_get(src);
        src += vm_asm_word(src);
        continue;
      }
      if (vm_asm_starts(opname, "bb"))
      {
        vm_asm_put_op(VM_OPCODE_BB);
        vm_asm_put_reg(vm_asm_read_reg(&src));
        vm_asm_strip(&src);
        vm_asm_put_get(src);
        src += vm_asm_word(src);
        vm_asm_strip(&src);
        vm_asm_put_get(src);
        src += vm_asm_word(src);
        continue;
      }
      if (vm_asm_starts(opname, "blt"))
      {
        vm_asm_put_op(VM_OPCODE_BLT);
        vm_asm_put_reg(vm_asm_read_reg(&src));
        vm_asm_put_reg(vm_asm_read_reg(&src));
        vm_asm_strip(&src);
        vm_asm_put_get(src);
        src += vm_asm_word(src);
        vm_asm_strip(&src);
        vm_asm_put_get(src);
        src += vm_asm_word(src);
        continue;
      }
      if (vm_asm_starts(opname, "beq"))
      {
        vm_asm_put_op(VM_OPCODE_BEQ);
        vm_asm_put_reg(vm_asm_read_reg(&src));
        vm_asm_put_reg(vm_asm_read_reg(&src));
        vm_asm_strip(&src);
        vm_asm_put_get(src);
        src += vm_asm_word(src);
        vm_asm_strip(&src);
        vm_asm_put_get(src);
        src += vm_asm_word(src);
        continue;
      }
    }
    goto err;
  }
  return instrs;
err:
  free(instrs);
  return NULL;
}

typedef struct
{
  const char *name;
  vm_opcode_t where;
} vm_asm_link_t;

vm_asm_buf_t vm_asm_link(vm_asm_instr_t *instrs)
{

  size_t links_alloc = 4;
  vm_asm_link_t *links = vm_malloc(sizeof(vm_asm_link_t) * links_alloc);
  size_t nlinks = 0;
  size_t cur_loc = 0;
  for (vm_asm_instr_t *cur = instrs; cur->type != VM_ASM_INSTR_END; cur++)
  {
    switch (cur->type)
    {
    case VM_ASM_INSTR_RAW:
    {
      cur_loc += 1;
      break;
    }
    case VM_ASM_INSTR_GET:
    {
      cur_loc += 1;
      break;
    }
    case VM_ASM_INSTR_SET:
    {
      if (nlinks + 1 > links_alloc)
      {
        links_alloc = nlinks * 4 + 1;
        links = vm_realloc(links, sizeof(vm_opcode_t) * links_alloc);
      }
      links[nlinks++] = (vm_asm_link_t){
          .name = (const char *)cur->value,
          .where = cur_loc,
      };
      break;
    }
    }
  }
  size_t ops_alloc = 16;
  vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * ops_alloc);
  size_t nops = 0;
  for (vm_asm_instr_t *cur = instrs; cur->type != VM_ASM_INSTR_END; cur++)
  {
    if (nops + 1 > ops_alloc)
    {
      ops_alloc = nops * 4 + 1;
      ops = vm_realloc(ops, sizeof(vm_opcode_t) * ops_alloc);
    }
    switch (cur->type)
    {
    case VM_ASM_INSTR_RAW:
    {
      ops[nops++] = cur->value;
      break;
    }
    case VM_ASM_INSTR_GET:
    {
      int val = -1;
      const char *find = (const char *)cur->value;
      for (size_t linkno = 0; linkno < nlinks; linkno++)
      {
        size_t head = 0;
        for (;;)
        {
          if (!vm_asm_isword(find[head]))
          {
            if (!vm_asm_isword(links[linkno].name[head]))
            {
              val = links[linkno].where;
              goto done;
            }
            break;
          }
          if (find[head] != links[linkno].name[head])
          {
            break;
          }
          head += 1;
        }
      }
      printf("linker error: undefined: %.*s\n", (int)vm_asm_word(find), find);
      goto err;
    done:
      ops[nops++] = (vm_opcode_t)val;
      break;
    }
    case VM_ASM_INSTR_SET:
    {
      break;
    }
    }
  }
  free(links);
  return (vm_asm_buf_t){
      .ops = ops,
      .nops = nops,
  };
err:
  free(links);
  free(ops);
  return (vm_asm_buf_t){
      .nops = 0,
  };
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    return 1;
  }
  const char *src = vm_asm_io_read(argv[1]);
  if (src == NULL)
  {
    return 1;
  }
  vm_asm_instr_t *instrs = vm_asm_read(src);
  if (instrs == NULL)
  {
    vm_free((void *)src);
    return 1;
  }
  vm_asm_buf_t buf = vm_asm_link(instrs);
  vm_free(instrs);
  vm_free((void *)src);
  if (buf.nops == 0)
  {
    return 1;
  }
  int res = vm_run_arch_int(buf.nops, buf.ops);
  free(buf.ops);
  if (res != 0)
  {
    return 1;
  }
  return 0;
}
