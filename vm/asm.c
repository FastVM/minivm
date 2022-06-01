#include "asm.h"

void vm_asm_strip(const char **src)
{
  while (**src == ' ' || **src == '\t')
  {
    *src += 1;
  }
}

void vm_asm_stripln(const char **src)
{
  while (**src == ' ' || **src == '\t' || **src == '\n' || **src == '\r' || **src == ';' || **src == '#')
  {
    if (**src == '#') {
      while (**src != '\n' && **src != '\0') {
        *src += 1;
      }
    } else {
      *src += 1;
    }
  }
}

int vm_asm_isdigit(char c)
{
  return ('0' <= c && c <= '9');
}

int vm_asm_isword(char c)
{
  return vm_asm_isdigit(c) || c == '_' || c == '.' || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
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

#define vm_asm_put(type_, value_) ({instrs[head] .type = (type_);instrs[head].value = (value_);head += 1;instrs[head].type = VM_ASM_INSTR_END; })
#define vm_asm_put_op(op_) vm_asm_put((VM_ASM_INSTR_RAW), (op_))
#define vm_asm_put_reg(reg_) ({int r=reg_; if (r>nregs){nregs=r;} vm_asm_put((VM_ASM_INSTR_RAW), (r));})
#define vm_asm_put_int(int_) vm_asm_put((VM_ASM_INSTR_RAW), (int_))
#define vm_asm_put_set(name_) ({*nsets += 1; vm_asm_put((VM_ASM_INSTR_SET), (size_t)(name_));})
#define vm_asm_put_get(name_) vm_asm_put((VM_ASM_INSTR_GET), (size_t)(name_))

vm_asm_instr_t *vm_asm_read(const char **src, size_t *nsets)
{
  size_t alloc = 32;
  vm_asm_instr_t *instrs = vm_malloc(sizeof(vm_asm_instr_t) * alloc);
  size_t head = 0;
  vm_asm_put_op(VM_OPCODE_JUMP);
  vm_asm_put_get("main");
  size_t where;
  int nregs = 0;
  for (;;)
  {
    if (head + 16 > alloc)
    {
      alloc = head * 4 + 16;
      instrs = vm_realloc(instrs, sizeof(vm_asm_instr_t) * alloc);
    }
    vm_asm_stripln(src);
    if (**src == '\0')
    {
      break;
    }
    if (**src == '@')
    {
      *src += 1;
      vm_asm_strip(src);
      vm_asm_put_set(*src);
      *src += vm_asm_word(*src);
      continue;
    }
    else if (**src =='r' && '0' <= (*src)[1] && (*src)[1] <= '9')
    {
      size_t regn = vm_asm_word(*src);
      vm_opcode_t regno = 0;
      int isreg = 1;
      for (size_t i = 1; i < regn; i++)
      {
        if (vm_asm_isdigit((*src)[i]) == 0)
        {
          isreg = false;
          break;
        }
        regno *= 10;
        regno += (*src)[i] - '0';
      }
      if (isreg)
      {
        *src += regn;
        vm_asm_strip(src);
        if (vm_asm_starts(*src, "<-") == 0)
        {
          goto err;
        }
        *src += 2;
        vm_asm_strip(src);
        const char *opname = *src;
        *src += vm_asm_word(*src);
        if (vm_asm_starts(opname, "sint"))
        {
          vm_asm_put_op(VM_OPCODE_SINT);
          vm_asm_put_reg(regno);
          vm_asm_put_int(vm_asm_read_int(src));
          continue;
        }
        if (vm_asm_starts(opname, "sneg"))
        {
          vm_asm_put_op(VM_OPCODE_SNEG);
          vm_asm_put_reg(regno);
          vm_asm_put_int(vm_asm_read_int(src));
          continue;
        }
        if (vm_asm_starts(opname, "fint"))
        {
          vm_asm_put_op(VM_OPCODE_FINT);
          vm_asm_put_reg(regno);
          vm_asm_put_int(vm_asm_read_int(src));
          continue;
        }
        if (vm_asm_starts(opname, "frac"))
        {
          vm_asm_put_op(VM_OPCODE_FINT);
          vm_asm_put_reg(regno);
          vm_asm_put_int(vm_asm_read_int(src));
          vm_asm_put_int(vm_asm_read_int(src));
          continue;
        }
        if (vm_asm_starts(opname, "addr"))
        {
          vm_asm_put_op(VM_OPCODE_ADDR);
          vm_asm_put_reg(regno);
          vm_asm_strip(src);
          vm_asm_put_get(*src);
          *src += vm_asm_word(*src);
          continue;
        }
        if (vm_asm_starts(opname, "reg"))
        {
          vm_asm_put_op(VM_OPCODE_REG);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "ftos"))
        {
          vm_asm_put_op(VM_OPCODE_FTOS);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "stof"))
        {
          vm_asm_put_op(VM_OPCODE_STOF);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "fadd"))
        {
          vm_asm_put_op(VM_OPCODE_FADD);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "fsub"))
        {
          vm_asm_put_op(VM_OPCODE_FSUB);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "fmul"))
        {
          vm_asm_put_op(VM_OPCODE_FMUL);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "fdiv"))
        {
          vm_asm_put_op(VM_OPCODE_FDIV);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "fmod"))
        {
          vm_asm_put_op(VM_OPCODE_FMOD);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "call")) {
          vm_asm_put_op(VM_OPCODE_CALL);
          vm_asm_put_reg(regno);
          vm_asm_strip(src);
          vm_asm_put_get(*src);
          *src += vm_asm_word(*src);
          vm_opcode_t args[8] = {0};
          size_t nargs = 0;
          vm_asm_strip(src);
          while (**src != '\n') {
            args[nargs++] = vm_asm_read_reg(src);
            vm_asm_strip(src);
          }
          vm_asm_put_int(nargs);
          for (size_t i = 0; i < nargs; i++) {
            vm_asm_put_reg(args[i]);
          }
          continue;
        }
        if (vm_asm_starts(opname, "dcall")) {
          vm_asm_put_op(VM_OPCODE_DCALL);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_opcode_t args[8] = {0};
          size_t nargs = 0;
          vm_asm_strip(src);
          while (**src != '\n') {
            args[nargs++] = vm_asm_read_reg(src);
            vm_asm_strip(src);
          }
          vm_asm_put_int(nargs);
          for (size_t i = 0; i < nargs; i++) {
            vm_asm_put_reg(args[i]);
          }
          continue;
        }
        if (vm_asm_starts(opname, "stof"))
        {
          vm_asm_put_op(VM_OPCODE_STOF);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "sadd"))
        {
          vm_asm_put_op(VM_OPCODE_SADD);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "ssub"))
        {
          vm_asm_put_op(VM_OPCODE_SSUB);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "smul"))
        {
          vm_asm_put_op(VM_OPCODE_SMUL);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "sdiv"))
        {
          vm_asm_put_op(VM_OPCODE_SDIV);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
        if (vm_asm_starts(opname, "smod"))
        {
          vm_asm_put_op(VM_OPCODE_SMOD);
          vm_asm_put_reg(regno);
          vm_asm_put_reg(vm_asm_read_reg(src));
          vm_asm_put_reg(vm_asm_read_reg(src));
          continue;
        }
      }
    }
    else
    {
      vm_asm_strip(src);
      const char *opname = *src;
      *src += vm_asm_word(*src);
      if (vm_asm_starts(opname, "func"))
      {
        nregs = 0;
        vm_asm_put_op(VM_OPCODE_FUNC);
        vm_asm_put_int(0);
        vm_asm_strip(src);
        const char *fn = *src;
        *src += vm_asm_word(*src);
        vm_asm_put_int(0);
        where = head;
        vm_asm_put_int(256);
        vm_asm_put_set(fn);
        continue;
      }
      if (vm_asm_starts(opname, "end")) {
        instrs[where].value = nregs+1;
        nregs = 0;
        continue;
      }
      if (vm_asm_starts(opname, "exit"))
      {
        vm_asm_put_op(VM_OPCODE_EXIT);
        continue;
      }
      if (vm_asm_starts(opname, "ret"))
      {
        vm_asm_put_op(VM_OPCODE_RET);
        vm_asm_put_reg(vm_asm_read_reg(src));
        continue;
      }
      if (vm_asm_starts(opname, "putchar"))
      {
        vm_asm_put_op(VM_OPCODE_PUTCHAR);
        vm_asm_put_reg(vm_asm_read_reg(src));
        continue;
      }
      if (vm_asm_starts(opname, "djump"))
      {
        vm_asm_put_op(VM_OPCODE_DJUMP);
        vm_asm_put_reg(vm_asm_read_reg(src));
        continue;
      }
      if (vm_asm_starts(opname, "jump"))
      {
        vm_asm_put_op(VM_OPCODE_JUMP);
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        continue;
      }
      if (vm_asm_starts(opname, "sbb"))
      {
        vm_asm_put_op(VM_OPCODE_SBB);
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        continue;
      }
      if (vm_asm_starts(opname, "sblt"))
      {
        vm_asm_put_op(VM_OPCODE_SBLT);
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        continue;
      }
      if (vm_asm_starts(opname, "sbeq"))
      {
        vm_asm_put_op(VM_OPCODE_SBEQ);
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        continue;
      }
      if (vm_asm_starts(opname, "fbb"))
      {
        vm_asm_put_op(VM_OPCODE_FBB);
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        continue;
      }
      if (vm_asm_starts(opname, "fblt"))
      {
        vm_asm_put_op(VM_OPCODE_FBLT);
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        continue;
      }
      if (vm_asm_starts(opname, "fbeq"))
      {
        vm_asm_put_op(VM_OPCODE_FBEQ);
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_put_reg(vm_asm_read_reg(src));
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        vm_asm_strip(src);
        vm_asm_put_get(*src);
        *src += vm_asm_word(*src);
        continue;
      }
    }
    goto err;
  }
  vm_asm_put_op(VM_OPCODE_EXIT);
  return instrs;
err:
  fprintf(stderr, "%.100s", *src);
  fprintf(stderr, "asm error: source can not be parsed\n");
  free(instrs);
  return NULL;
}

typedef struct
{
  const char *name;
  vm_opcode_t where;
} vm_asm_link_t;

static size_t vm_asm_hash(const char *str, size_t n)
{
    size_t hash = 5381;
    for (size_t i = 0; i < n; i++) {
      hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

vm_asm_buf_t vm_asm_link(vm_asm_instr_t *instrs, size_t n)
{
  size_t links_alloc = n * 2;
  vm_asm_link_t *links = vm_alloc0(sizeof(vm_asm_link_t) * links_alloc);
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
      const char *str = (const char *)cur->value;
      size_t put = vm_asm_hash(str, vm_asm_word(str));
      while (links[put % links_alloc].name != NULL) {
        put += 1;
      }
      links[put % links_alloc].name = str;
      links[put % links_alloc].where = cur_loc;
      break;
    }
    }
  }
  size_t ops_alloc = 256;
  vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * ops_alloc);
  size_t nops = 0;
  for (vm_asm_instr_t *cur = instrs; cur->type != VM_ASM_INSTR_END; cur++)
  {
    if (nops + 2 > ops_alloc)
    {
      ops_alloc = nops * 4 + 2;
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
      size_t findlen = vm_asm_word(find);
      size_t get = vm_asm_hash(find, findlen);
      size_t start = get;
      while (get != start + links_alloc) {
        const char *name = links[get % links_alloc].name;
        if (name == NULL) {
          get += 1;
          continue;
        }
        size_t linklen = vm_asm_word(name);
        if (linklen == findlen) {
          size_t head = 0;
          while (head < findlen)
          {
            if (find[head] != name[head])
            {
              break;
            }
            head += 1;
            if (head == findlen) {
              val = links[get % links_alloc].where;
              goto done;
            }
          }
        }
        get += 1;
      }
      if (findlen == 0) {
        fprintf(stderr, "linker error: empty symbol\n");
      }
      fprintf(stderr, "linker error: undefined: %.*s\n", (int)vm_asm_word(find), find);
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

vm_asm_buf_t vm_asm(const char *src) {
  const char **psrc = vm_malloc(sizeof(const char *));
  *psrc = src;
  size_t *n = vm_malloc(sizeof(size_t));
  *n = 0;
  vm_asm_instr_t *instrs = vm_asm_read(psrc, n);
  if (instrs == NULL)
  {
    vm_free(psrc);
    vm_free(n);
    return (vm_asm_buf_t) {
      .nops = 0,
      .ops = NULL,
    };
  }
  vm_asm_buf_t buf = vm_asm_link(instrs, *n);
  vm_free(psrc);
  vm_free(n);
  vm_free(instrs);
  return buf;
}