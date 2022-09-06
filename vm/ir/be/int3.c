
#include "int3.h"
#include "../build.h"
#include <stddef.h>
#include <stdio.h>

#define vm_int_block_comp_put_ptr(arg_)                                        \
  buf.ops[buf.len++].ptr = ptrs[(arg_)]
#define vm_int_block_comp_put_out(out_) buf.ops[buf.len++].reg = (out_)
#define vm_int_block_comp_put_reg(reg_) buf.ops[buf.len++].reg = (reg_)
#define vm_int_block_comp_put_val(val_) buf.ops[buf.len++].val = (val_)
#define vm_int_block_comp_put_block(block_) buf.ops[buf.len++].block = (block_)
#define vm_int_block_comp_put_arg(arg_)                                        \
  ({                                                                           \
    vm_ir_arg_t arg = (arg_);                                                  \
    if (arg.type == VM_IR_ARG_REG) {                                           \
      vm_int_block_comp_put_reg(arg.reg);                                      \
    } else if (arg.type == VM_IR_ARG_NUM) {                                    \
      vm_int_block_comp_put_val(arg.num);                                      \
    } else if (arg.type == VM_IR_ARG_FUNC) {                                   \
      vm_int_block_comp_put_block(arg.func);                                   \
    } else {                                                                   \
      fprintf(stderr, "unknown arg type\n");                                   \
      __builtin_trap();                                                        \
    }                                                                          \
  })

void *vm_int_block_comp(vm_int_state_t *state, void **ptrs, vm_ir_block_t *block) {
  if (block->data != NULL) {
    return block->data;
  }
  vm_int_buf_t buf;
  buf.len = 0;
  buf.alloc = 16;
  buf.ops = vm_alloc0(sizeof(vm_int_opcode_t) * buf.alloc);
  for (size_t arg = 0; arg < block->len; arg++) {
    if (buf.len + 8 >= buf.alloc) {
      buf.alloc = buf.len * 2 + 8;
      buf.ops = vm_realloc(buf.ops, sizeof(vm_int_opcode_t) * buf.alloc);
    }
    vm_ir_instr_t *instr = block->instrs[arg];
    switch (instr->op) {
    case VM_IR_IOP_NOP: {
      break;
    }
    case VM_IR_IOP_MOVE: {
      if (instr->out.type == VM_IR_ARG_REG) {
        switch (instr->args[0].type) {
        case VM_IR_ARG_REG: {
          // r = move r
          vm_int_block_comp_put_ptr(VM_INT_OP_MOV_R);
          vm_int_block_comp_put_out(instr->out.reg);
          vm_int_block_comp_put_arg(instr->args[0]);
          break;
        }
        case VM_IR_ARG_NUM: {
          // r = move i
          vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
          vm_int_block_comp_put_out(instr->out.reg);
          vm_int_block_comp_put_arg(instr->args[0]);
          break;
        }
        case VM_IR_ARG_STR: {
          fprintf(stderr, "NO STRINGS YET\n");
          __builtin_trap();
        }
        case VM_IR_ARG_FUNC: {
          // r = move i
          vm_int_block_comp_put_ptr(VM_INT_OP_MOV_T);
          vm_int_block_comp_put_out(instr->out.reg);
          vm_int_block_comp_put_arg(instr->args[0]);
          break;
        }
        }
      }
      break;
    }
    case VM_IR_IOP_ADD: {
      if (instr->out.type == VM_IR_ARG_REG) {
        if (instr->args[0].type == VM_IR_ARG_REG) {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = add r r
            vm_int_block_comp_put_ptr(VM_INT_OP_ADD_RR);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          } else {
            // r = add r i
            vm_int_block_comp_put_ptr(VM_INT_OP_ADD_RI);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          }
        } else {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = add i r
            vm_int_block_comp_put_ptr(VM_INT_OP_ADD_RI);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[1]);
            vm_int_block_comp_put_arg(instr->args[0]);
          } else {
            // r = move i
            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_val(instr->args[0].num + instr->args[1].num);
          }
        }
      }
      break;
    }
    case VM_IR_IOP_SUB: {
      if (instr->out.type == VM_IR_ARG_REG) {
        if (instr->args[0].type == VM_IR_ARG_REG) {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = sub r r
            vm_int_block_comp_put_ptr(VM_INT_OP_SUB_RR);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          } else {
            // r = sub r i
            vm_int_block_comp_put_ptr(VM_INT_OP_SUB_RI);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          }
        } else {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = sub i r
            vm_int_block_comp_put_ptr(VM_INT_OP_SUB_IR);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          } else {
            // r = move i
            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_val(instr->args[0].num - instr->args[1].num);
          }
        }
      }
      break;
    }
    case VM_IR_IOP_MUL: {
      if (instr->out.type == VM_IR_ARG_REG) {
        if (instr->args[0].type == VM_IR_ARG_REG) {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = mul r r
            vm_int_block_comp_put_ptr(VM_INT_OP_MUL_RR);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          } else {
            // r = mul r i
            vm_int_block_comp_put_ptr(VM_INT_OP_MUL_RI);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          }
        } else {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = mul i r
            vm_int_block_comp_put_ptr(VM_INT_OP_MUL_RI);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[1]);
            vm_int_block_comp_put_arg(instr->args[0]);
          } else {
            // r = move i
            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_val(instr->args[0].num * instr->args[1].num);
          }
        }
      }
      break;
    }
    case VM_IR_IOP_DIV: {
      if (instr->out.type == VM_IR_ARG_REG) {
        if (instr->args[0].type == VM_IR_ARG_REG) {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = div r r
            vm_int_block_comp_put_ptr(VM_INT_OP_DIV_RR);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          } else {
            // r = div r i
            vm_int_block_comp_put_ptr(VM_INT_OP_DIV_RI);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          }
        } else {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = div i r
            vm_int_block_comp_put_ptr(VM_INT_OP_DIV_IR);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          } else {
            // r = move i
            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_val(instr->args[0].num / instr->args[1].num);
          }
        }
      }
      break;
    }
    case VM_IR_IOP_MOD: {
      if (instr->out.type == VM_IR_ARG_REG) {
        if (instr->args[0].type == VM_IR_ARG_REG) {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = mod r r
            vm_int_block_comp_put_ptr(VM_INT_OP_MOD_RR);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          } else {
            // r = mod r i
            vm_int_block_comp_put_ptr(VM_INT_OP_MOD_RI);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          }
        } else {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = mod i r
            vm_int_block_comp_put_ptr(VM_INT_OP_MOD_IR);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          } else {
            // r = move i
            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_val(instr->args[0].num % instr->args[1].num);
          }
        }
      }
      break;
    }
    case VM_IR_IOP_CALL: {
      for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
        if (instr->args[i].type == VM_IR_ARG_NUM) {
          vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
          vm_int_block_comp_put_out(state->framesize + i);
          vm_int_block_comp_put_arg(instr->args[0]);
        }
      }
      size_t nargs = 0;
      for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
        nargs += 1;
      }
      if (instr->args[0].type == VM_IR_ARG_FUNC) {
        vm_int_block_comp_put_ptr(VM_INT_OP_CALL_T0 + nargs);
        vm_int_block_comp_put_block(instr->args[0].func);
      } else if (instr->args[0].type == VM_IR_ARG_EXTERN) {
        vm_int_block_comp_put_ptr(VM_INT_OP_CALL_X0 + nargs);
        vm_int_block_comp_put_val(instr->args[0].num);
      } else {
        vm_int_block_comp_put_ptr(VM_INT_OP_CALL_R0 + nargs);
        vm_int_block_comp_put_arg(instr->args[0]);
      }
      for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
        if (instr->args[i].type == VM_IR_ARG_REG) {
          vm_int_block_comp_put_arg(instr->args[i]);
        } else {
          vm_int_block_comp_put_reg(state->framesize + i);
        }
      }
      if (instr->out.type == VM_IR_ARG_REG) {
        vm_int_block_comp_put_out(instr->out.reg);
      } else {
        vm_int_block_comp_put_out(block->nregs + 1);
      }
      break;
    }
    case VM_IR_IOP_ARR: {
      if (instr->out.type == VM_IR_ARG_REG) {
        if (instr->args[0].type == VM_IR_ARG_REG) {
          // r = new r
          vm_int_block_comp_put_ptr(VM_INT_OP_NEW_R);
          vm_int_block_comp_put_out(instr->out.reg);
          vm_int_block_comp_put_arg(instr->args[0]);
        } else {
          // r = new i
          vm_int_block_comp_put_ptr(VM_INT_OP_NEW_I);
          vm_int_block_comp_put_out(instr->out.reg);
          vm_int_block_comp_put_arg(instr->args[0]);
        }
      }
      break;
    }
    case VM_IR_IOP_GET: {
      if (instr->out.type == VM_IR_ARG_REG) {
        if (instr->args[0].type == VM_IR_ARG_REG) {
          if (instr->args[1].type == VM_IR_ARG_REG) {
            // r = get r r
            vm_int_block_comp_put_ptr(VM_INT_OP_GET_RR);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          } else {
            // r = get r i
            vm_int_block_comp_put_ptr(VM_INT_OP_GET_RI);
            vm_int_block_comp_put_out(instr->out.reg);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
          }
        }
      }
      break;
    }
    case VM_IR_IOP_SET: {
      if (instr->args[0].type == VM_IR_ARG_REG) {
        if (instr->args[1].type == VM_IR_ARG_REG) {
          if (instr->args[2].type == VM_IR_ARG_REG) {
            // set r r r
            vm_int_block_comp_put_ptr(VM_INT_OP_SET_RRR);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
            vm_int_block_comp_put_arg(instr->args[2]);
          } else {
            // set r r i
            vm_int_block_comp_put_ptr(VM_INT_OP_SET_RRI);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
            vm_int_block_comp_put_arg(instr->args[2]);
          }
        } else {
          if (instr->args[2].type == VM_IR_ARG_REG) {
            // set r i r
            vm_int_block_comp_put_ptr(VM_INT_OP_SET_RIR);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
            vm_int_block_comp_put_arg(instr->args[2]);
          } else {
            // set r i i
            vm_int_block_comp_put_ptr(VM_INT_OP_SET_RII);
            vm_int_block_comp_put_arg(instr->args[0]);
            vm_int_block_comp_put_arg(instr->args[1]);
            vm_int_block_comp_put_arg(instr->args[2]);
          }
        }
      }
      break;
    }
    case VM_IR_IOP_LEN: {
      if (instr->out.type == VM_IR_ARG_REG) {
        if (instr->args[0].type == VM_IR_ARG_REG) {
          // r = len r
          vm_int_block_comp_put_ptr(VM_INT_OP_LEN_R);
          vm_int_block_comp_put_out(instr->out.reg);
          vm_int_block_comp_put_arg(instr->args[0]);
        }
      }
      break;
    }
    case VM_IR_IOP_TYPE: {
      if (instr->out.type == VM_IR_ARG_REG) {
        if (instr->args[0].type == VM_IR_ARG_REG) {
          // r = type r
        } else {
          // r = move i
        }
      }
      break;
    }
    case VM_IR_IOP_OUT: {
      if (instr->args[0].type == VM_IR_ARG_NUM) {
        // out i
        vm_int_block_comp_put_ptr(VM_INT_OP_OUT_I);
        vm_int_block_comp_put_arg(instr->args[0]);
      } else {
        // out r
        vm_int_block_comp_put_ptr(VM_INT_OP_OUT_R);
        vm_int_block_comp_put_arg(instr->args[0]);
      }
      break;
    }
    }
  }
  switch (block->branch->op) {
  case VM_IR_BOP_JUMP: {
    // jump l
    vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
    vm_int_block_comp_put_block(block->branch->targets[0]);
    break;
  }
  case VM_IR_BOP_BOOL: {
    if (block->branch->args[0].type == VM_IR_ARG_NUM) {
      // jump l
      vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
      vm_int_block_comp_put_block(
          block->branch->targets[block->branch->args[0].num != 0]);
    } else {
      // bb r l l
      vm_int_block_comp_put_ptr(VM_INT_OP_BB_RTT);
      vm_int_block_comp_put_arg(block->branch->args[0]);
      vm_int_block_comp_put_block(block->branch->targets[0]);
      vm_int_block_comp_put_block(block->branch->targets[1]);
    }
    break;
  }
  case VM_IR_BOP_LESS: {
    if (block->branch->args[0].type == VM_IR_ARG_NUM) {
      if (block->branch->args[1].type == VM_IR_ARG_NUM) {
        // jump l
        vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
        vm_int_block_comp_put_block(
            block->branch->targets[block->branch->args[0].num <
                                   block->branch->args[1].num]);
      } else {
        // blt i r l l
        vm_int_block_comp_put_ptr(VM_INT_OP_BLT_IRTT);
        vm_int_block_comp_put_arg(block->branch->args[0]);
        vm_int_block_comp_put_arg(block->branch->args[1]);
        vm_int_block_comp_put_block(block->branch->targets[0]);
        vm_int_block_comp_put_block(block->branch->targets[1]);
      }
    } else {
      if (block->branch->args[1].type == VM_IR_ARG_NUM) {
        // blt r i l l
        vm_int_block_comp_put_ptr(VM_INT_OP_BLT_RITT);
        vm_int_block_comp_put_arg(block->branch->args[0]);
        vm_int_block_comp_put_arg(block->branch->args[1]);
        vm_int_block_comp_put_block(block->branch->targets[0]);
        vm_int_block_comp_put_block(block->branch->targets[1]);
      } else {
        // blt r r l l
        vm_int_block_comp_put_ptr(VM_INT_OP_BLT_RRTT);
        vm_int_block_comp_put_arg(block->branch->args[0]);
        vm_int_block_comp_put_arg(block->branch->args[1]);
        vm_int_block_comp_put_block(block->branch->targets[0]);
        vm_int_block_comp_put_block(block->branch->targets[1]);
      }
    }
    break;
  }
  case VM_IR_BOP_EQUAL: {
    if (block->branch->args[0].type == VM_IR_ARG_NUM) {
      if (block->branch->args[1].type == VM_IR_ARG_NUM) {
        // jump l
        vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
        vm_int_block_comp_put_block(
            block->branch->targets[block->branch->args[0].num ==
                                   block->branch->args[1].num]);
      } else {
        // beq i r l l
        vm_int_block_comp_put_ptr(VM_INT_OP_BEQ_IRTT);
        vm_int_block_comp_put_arg(block->branch->args[0]);
        vm_int_block_comp_put_arg(block->branch->args[1]);
        vm_int_block_comp_put_block(block->branch->targets[0]);
        vm_int_block_comp_put_block(block->branch->targets[1]);
      }
    } else {
      if (block->branch->args[1].type == VM_IR_ARG_NUM) {
        // beq r i l l
        vm_int_block_comp_put_ptr(VM_INT_OP_BEQ_RITT);
        vm_int_block_comp_put_arg(block->branch->args[0]);
        vm_int_block_comp_put_arg(block->branch->args[1]);
        vm_int_block_comp_put_block(block->branch->targets[0]);
        vm_int_block_comp_put_block(block->branch->targets[1]);
      } else {
        // beq r r l l
        vm_int_block_comp_put_ptr(VM_INT_OP_BEQ_RRTT);
        vm_int_block_comp_put_arg(block->branch->args[0]);
        vm_int_block_comp_put_arg(block->branch->args[1]);
        vm_int_block_comp_put_block(block->branch->targets[0]);
        vm_int_block_comp_put_block(block->branch->targets[1]);
      }
    }
    break;
  }
  case VM_IR_BOP_RET: {
    vm_value_t val;
    if (block->branch->args[0].type == VM_IR_ARG_NUM) {
      // ret i
      vm_int_block_comp_put_ptr(VM_INT_OP_RET_I);
      vm_int_block_comp_put_arg(block->branch->args[0]);
    } else {
      // ret r
      vm_int_block_comp_put_ptr(VM_INT_OP_RET_R);
      vm_int_block_comp_put_arg(block->branch->args[0]);
    }
    break;
  }
  case VM_IR_BOP_EXIT: {
    // exit
    vm_int_block_comp_put_ptr(VM_INT_OP_EXIT);
    break;
  }
  }
  vm_int_block_comp_put_block(block);
  block->data = buf.ops;
  return buf.ops;
}

#define vm_int_run_read() (*({vm_int_opcode_t *ret = head; head += 1; ret;}))
#if 0
#define vm_int_debug(v)                                                        \
  ({                                                                           \
    typeof(v) v_ = v;                                                          \
    fprintf(stderr, "DEBUG(%zu): %zX\n", (size_t) __LINE__, (size_t)v_);                               \
    v_;                                                                        \
  })
#else
#define vm_int_debug(v) (v)
#endif
#define vm_int_run_next() goto *vm_int_debug(vm_int_run_read().ptr)
#define vm_int_not_implemented()                                               \
  ({                                                                           \
    fprintf(stderr, "UNIMPLEMENTED line=%zu\n", (size_t)__LINE__);             \
    return;                                                                    \
  })

vm_value_t vm_int_run(vm_int_state_t *state, vm_ir_block_t *block) {
  static void *ptrs[VM_INT_MAX_OP] = {
      [VM_INT_OP_MOV_I] = &&do_mov_i,
      [VM_INT_OP_MOV_R] = &&do_mov_r,
      [VM_INT_OP_MOV_T] = &&do_mov_t,
      [VM_INT_OP_ADD_RR] = &&do_add_rr,
      [VM_INT_OP_ADD_RI] = &&do_add_ri,
      [VM_INT_OP_SUB_RR] = &&do_sub_rr,
      [VM_INT_OP_SUB_RI] = &&do_sub_ri,
      [VM_INT_OP_SUB_IR] = &&do_sub_ir,
      [VM_INT_OP_MUL_RR] = &&do_mul_rr,
      [VM_INT_OP_MUL_RI] = &&do_mul_ri,
      [VM_INT_OP_DIV_RR] = &&do_div_rr,
      [VM_INT_OP_DIV_RI] = &&do_div_ri,
      [VM_INT_OP_DIV_IR] = &&do_div_ir,
      [VM_INT_OP_MOD_RR] = &&do_mod_rr,
      [VM_INT_OP_MOD_RI] = &&do_mod_ri,
      [VM_INT_OP_MOD_IR] = &&do_mod_ir,
      [VM_INT_OP_CALL_L0] = &&do_call_l0,
      [VM_INT_OP_CALL_L1] = &&do_call_l1,
      [VM_INT_OP_CALL_L2] = &&do_call_l2,
      [VM_INT_OP_CALL_L3] = &&do_call_l3,
      [VM_INT_OP_CALL_L4] = &&do_call_l4,
      [VM_INT_OP_CALL_L5] = &&do_call_l5,
      [VM_INT_OP_CALL_L6] = &&do_call_l6,
      [VM_INT_OP_CALL_L7] = &&do_call_l7,
      [VM_INT_OP_CALL_L8] = &&do_call_l8,
      [VM_INT_OP_CALL_R0] = &&do_call_r0,
      [VM_INT_OP_CALL_R1] = &&do_call_r1,
      [VM_INT_OP_CALL_R2] = &&do_call_r2,
      [VM_INT_OP_CALL_R3] = &&do_call_r3,
      [VM_INT_OP_CALL_R4] = &&do_call_r4,
      [VM_INT_OP_CALL_R5] = &&do_call_r5,
      [VM_INT_OP_CALL_R6] = &&do_call_r6,
      [VM_INT_OP_CALL_R7] = &&do_call_r7,
      [VM_INT_OP_CALL_R8] = &&do_call_r8,
      [VM_INT_OP_CALL_X0] = &&do_call_x0,
      [VM_INT_OP_CALL_X1] = &&do_call_x1,
      [VM_INT_OP_CALL_X2] = &&do_call_x2,
      [VM_INT_OP_CALL_X3] = &&do_call_x3,
      [VM_INT_OP_CALL_X4] = &&do_call_x4,
      [VM_INT_OP_CALL_X5] = &&do_call_x5,
      [VM_INT_OP_CALL_X6] = &&do_call_x6,
      [VM_INT_OP_CALL_X7] = &&do_call_x7,
      [VM_INT_OP_CALL_X8] = &&do_call_x8,
      [VM_INT_OP_NEW_I] = &&do_new_i,
      [VM_INT_OP_NEW_R] = &&do_new_r,
      [VM_INT_OP_SET_RRR] = &&do_set_rrr,
      [VM_INT_OP_SET_RRI] = &&do_set_rri,
      [VM_INT_OP_SET_RIR] = &&do_set_rir,
      [VM_INT_OP_SET_RII] = &&do_set_rii,
      [VM_INT_OP_GET_RR] = &&do_get_rr,
      [VM_INT_OP_GET_RI] = &&do_get_ri,
      [VM_INT_OP_LEN_R] = &&do_len_r,
      [VM_INT_OP_OUT_I] = &&do_out_i,
      [VM_INT_OP_OUT_R] = &&do_out_r,
      [VM_INT_OP_JUMP_L] = &&do_jump_l,
      [VM_INT_OP_BB_RLL] = &&do_bb_rll,
      [VM_INT_OP_BLT_RRLL] = &&do_blt_rrll,
      [VM_INT_OP_BLT_RILL] = &&do_blt_rill,
      [VM_INT_OP_BLT_IRLL] = &&do_blt_irll,
      [VM_INT_OP_BEQ_RRLL] = &&do_beq_rrll,
      [VM_INT_OP_BEQ_RILL] = &&do_beq_rill,
      [VM_INT_OP_BEQ_IRLL] = &&do_beq_irll,
      [VM_INT_OP_RET_I] = &&do_ret_i,
      [VM_INT_OP_RET_R] = &&do_ret_r,
      [VM_INT_OP_CALL_T0] = &&do_call_t0,
      [VM_INT_OP_CALL_T1] = &&do_call_t1,
      [VM_INT_OP_CALL_T2] = &&do_call_t2,
      [VM_INT_OP_CALL_T3] = &&do_call_t3,
      [VM_INT_OP_CALL_T4] = &&do_call_t4,
      [VM_INT_OP_CALL_T5] = &&do_call_t5,
      [VM_INT_OP_CALL_T6] = &&do_call_t6,
      [VM_INT_OP_CALL_T7] = &&do_call_t7,
      [VM_INT_OP_CALL_T8] = &&do_call_t8,
      [VM_INT_OP_JUMP_T] = &&do_jump_t,
      [VM_INT_OP_BB_RTT] = &&do_bb_rtt,
      [VM_INT_OP_BLT_RRTT] = &&do_blt_rrtt,
      [VM_INT_OP_BLT_RITT] = &&do_blt_ritt,
      [VM_INT_OP_BLT_IRTT] = &&do_blt_irtt,
      [VM_INT_OP_BEQ_RRTT] = &&do_beq_rrtt,
      [VM_INT_OP_BEQ_RITT] = &&do_beq_ritt,
      [VM_INT_OP_BEQ_IRTT] = &&do_beq_irtt,
      [VM_INT_OP_EXIT] = &&do_exit,
  };
  vm_int_opcode_t *head = vm_int_block_comp(state, ptrs, block);
  vm_value_t *locals = state->locals;
  vm_value_t *init_locals = locals;
  static int n=0;
  // if (++n == 2)
    // __builtin_trap();
  vm_int_run_next();
do_mov_i : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_int_opcode_t value = vm_int_run_read();
  locals[out.reg] = vm_value_from_int(value.val);
  vm_int_run_next();
}
do_mov_r : {
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t value = locals[vm_int_run_read().reg];
  *out = value;
  vm_int_run_next();
}
do_mov_t : {
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_ir_block_t *block = vm_int_run_read().block;
  *out = vm_value_from_block(block);
  vm_int_run_next();
}
do_add_rr : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  vm_value_t rhs = locals[vm_int_run_read().reg];
  locals[out.reg] = vm_value_add(lhs, rhs);
  vm_int_run_next();
}
do_add_ri : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  ptrdiff_t rhs = vm_int_run_read().val;
  locals[out.reg] = vm_value_addi(lhs, rhs);
  vm_int_run_next();
}
do_sub_rr : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  vm_value_t rhs = locals[vm_int_run_read().reg];
  locals[out.reg] = vm_value_sub(lhs, rhs);
  vm_int_run_next();
}
do_sub_ri : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  ptrdiff_t rhs = vm_int_run_read().val;
  locals[out.reg] = vm_value_subi(lhs, rhs);
  vm_int_run_next();
}
do_sub_ir : {
  vm_int_opcode_t out = vm_int_run_read();
  ptrdiff_t lhs = vm_int_run_read().val;
  vm_value_t rhs = locals[vm_int_run_read().reg];
  locals[out.reg] = vm_value_isub(lhs, rhs);
  vm_int_run_next();
}
do_mul_rr : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  vm_value_t rhs = locals[vm_int_run_read().reg];
  locals[out.reg] = vm_value_mul(lhs, rhs);
  vm_int_run_next();
}
do_mul_ri : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  ptrdiff_t rhs = vm_int_run_read().val;
  locals[out.reg] = vm_value_muli(lhs, rhs);
  vm_int_run_next();
}
do_div_rr : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  vm_value_t rhs = locals[vm_int_run_read().reg];
  locals[out.reg] = vm_value_div(lhs, rhs);
  vm_int_run_next();
}
do_div_ri : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  ptrdiff_t rhs = vm_int_run_read().val;
  locals[out.reg] = vm_value_divi(lhs, rhs);
  vm_int_run_next();
}
do_div_ir : {
  vm_int_opcode_t out = vm_int_run_read();
  ptrdiff_t lhs = vm_int_run_read().val;
  vm_value_t rhs = locals[vm_int_run_read().reg];
  locals[out.reg] = vm_value_idiv(lhs, rhs);
  vm_int_run_next();
}
do_mod_rr : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  vm_value_t rhs = locals[vm_int_run_read().reg];
  locals[out.reg] = vm_value_mod(lhs, rhs);
  vm_int_run_next();
}
do_mod_ri : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t lhs = locals[vm_int_run_read().reg];
  ptrdiff_t rhs = vm_int_run_read().val;
  locals[out.reg] = vm_value_modi(lhs, rhs);
  vm_int_run_next();
}
do_mod_ir : {
  vm_int_opcode_t out = vm_int_run_read();
  ptrdiff_t lhs = vm_int_run_read().val;
  vm_value_t rhs = locals[vm_int_run_read().reg];
  locals[out.reg] = vm_value_imod(lhs, rhs);
  vm_int_run_next();
}
do_call_l0 : {
  vm_int_opcode_t block = vm_int_run_read();
  *state->heads++ = head;
  locals += state->framesize;
  head = block.ptr;
  vm_int_run_next();
}
do_call_l1 : {
  vm_int_opcode_t block = vm_int_run_read();
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = block.ptr;
  vm_int_run_next();
}
do_call_l2 : {
  vm_int_opcode_t block = vm_int_run_read();
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = block.ptr;
  vm_int_run_next();
}
do_call_l3 : {
  vm_int_opcode_t block = vm_int_run_read();
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = block.ptr;
  vm_int_run_next();
}
do_call_l4 : {
  vm_int_opcode_t block = vm_int_run_read();
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = block.ptr;
  vm_int_run_next();
}
do_call_l5 : {
  vm_int_opcode_t block = vm_int_run_read();
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 4] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = block.ptr;
  vm_int_run_next();
}
do_call_l6 : {
  vm_int_opcode_t block = vm_int_run_read();
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 4] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 5] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = block.ptr;
  vm_int_run_next();
}
do_call_l7 : {
  vm_int_opcode_t block = vm_int_run_read();
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 4] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 5] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 6] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = block.ptr;
  vm_int_run_next();
}
do_call_l8 : {
  vm_int_opcode_t block = vm_int_run_read();
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 4] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 5] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 6] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 7] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = block.ptr;
  vm_int_run_next();
}
do_call_r0 : {
  vm_ir_block_t *func = vm_value_to_block(locals[vm_int_run_read().reg]);
  void *ptr = vm_int_block_comp(state, ptrs, func);
  *state->heads++ = head;
  locals += state->framesize;
  head = ptr;
  vm_int_run_next();
}
do_call_r1 : {
  vm_ir_block_t *func = vm_value_to_block(locals[vm_int_run_read().reg]);
  void *ptr = vm_int_block_comp(state, ptrs, func);
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = ptr;
  vm_int_run_next();
}
do_call_r2 : {
  vm_ir_block_t *func = vm_value_to_block(locals[vm_int_run_read().reg]);
  void *ptr = vm_int_block_comp(state, ptrs, func);
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = ptr;
  vm_int_run_next();
}
do_call_r3 : {
  vm_ir_block_t *func = vm_value_to_block(locals[vm_int_run_read().reg]);
  void *ptr = vm_int_block_comp(state, ptrs, func);
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = ptr;
  vm_int_run_next();
}
do_call_r4 : {
  vm_ir_block_t *func = vm_value_to_block(locals[vm_int_run_read().reg]);
  void *ptr = vm_int_block_comp(state, ptrs, func);
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = ptr;
  vm_int_run_next();
}
do_call_r5 : {
  vm_ir_block_t *func = vm_value_to_block(locals[vm_int_run_read().reg]);
  void *ptr = vm_int_block_comp(state, ptrs, func);
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 4] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = ptr;
  vm_int_run_next();
}
do_call_r6 : {
  vm_ir_block_t *func = vm_value_to_block(locals[vm_int_run_read().reg]);
  void *ptr = vm_int_block_comp(state, ptrs, func);
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 4] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 5] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = ptr;
  vm_int_run_next();
}
do_call_r7 : {
  vm_ir_block_t *func = vm_value_to_block(locals[vm_int_run_read().reg]);
  void *ptr = vm_int_block_comp(state, ptrs, func);
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 4] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 5] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 6] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = ptr;
  vm_int_run_next();
}
do_call_r8 : {
  vm_ir_block_t *func = vm_value_to_block(locals[vm_int_run_read().reg]);
  void *ptr = vm_int_block_comp(state, ptrs, func);
  locals[state->framesize + 1 + 0] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 1] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 2] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 3] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 4] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 5] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 6] = locals[vm_int_run_read().reg];
  locals[state->framesize + 1 + 7] = locals[vm_int_run_read().reg];
  *state->heads++ = head;
  locals += state->framesize;
  head = ptr;
  vm_int_run_next();
}
do_call_x0 : {
  vm_int_func_t ptr = state->funcs[vm_int_run_read().val];
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t *last = state->locals;
  state->locals = locals + state->framesize;
  *out = ptr.func(ptr.data, 0, NULL);
  state->locals = last;
  vm_int_run_next();
}
do_call_x1 : {
  vm_int_func_t ptr = state->funcs[vm_int_run_read().val];
  vm_value_t values[1] = {locals[vm_int_run_read().reg]};
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t *last = state->locals;
  state->locals = locals + state->framesize;
  *out = ptr.func(ptr.data, 1, &values[0]);
  state->locals = last;
  vm_int_run_next();
}
do_call_x2 : {
  vm_int_func_t ptr = state->funcs[vm_int_run_read().val];
  vm_value_t values[2] = {locals[vm_int_run_read().reg],
                          locals[vm_int_run_read().reg]};
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t *last = state->locals;
  state->locals = locals + state->framesize;
  *out = ptr.func(ptr.data, 2, &values[0]);
  state->locals = last;
  vm_int_run_next();
}
do_call_x3 : {
  vm_int_func_t ptr = state->funcs[vm_int_run_read().val];
  vm_value_t values[3] = {locals[vm_int_run_read().reg],
                          locals[vm_int_run_read().reg],
                          locals[vm_int_run_read().reg]};
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t *last = state->locals;
  state->locals = locals + state->framesize;
  *out = ptr.func(ptr.data, 3, &values[0]);
  state->locals = last;
  vm_int_run_next();
}
do_call_x4 : {
  vm_int_func_t ptr = state->funcs[vm_int_run_read().val];
  vm_value_t values[4] = {
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg]};
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t *last = state->locals;
  state->locals = locals + state->framesize;
  *out = ptr.func(ptr.data, 4, &values[0]);
  state->locals = last;
  vm_int_run_next();
}
do_call_x5 : {
  vm_int_func_t ptr = state->funcs[vm_int_run_read().val];
  vm_value_t values[5] = {
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg]};
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t *last = state->locals;
  state->locals = locals + state->framesize;
  *out = ptr.func(ptr.data, 5, &values[0]);
  state->locals = last;
  vm_int_run_next();
}
do_call_x6 : {
  vm_int_func_t ptr = state->funcs[vm_int_run_read().val];
  vm_value_t values[6] = {
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg]};
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t *last = state->locals;
  state->locals = locals + state->framesize;
  *out = ptr.func(ptr.data, 6, &values[0]);
  state->locals = last;
  vm_int_run_next();
}
do_call_x7 : {
  vm_int_func_t ptr = state->funcs[vm_int_run_read().val];
  vm_value_t values[7] = {
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg]};
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t *last = state->locals;
  state->locals = locals + state->framesize;
  *out = ptr.func(ptr.data, 7, &values[0]);
  state->locals = last;
  vm_int_run_next();
}
do_call_x8 : {
  vm_int_func_t ptr = state->funcs[vm_int_run_read().val];
  vm_value_t values[8] = {
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg],
      locals[vm_int_run_read().reg], locals[vm_int_run_read().reg]};
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t *last = state->locals;
  state->locals = locals + state->framesize;
  *out = ptr.func(ptr.data, 8, &values[0]);
  state->locals = last;
  vm_int_run_next();
}
do_new_i : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_int_opcode_t len = vm_int_run_read();
  locals[out.reg] = vm_gc_new(&state->gc, len.val);
  vm_int_run_next();
}
do_new_r : {
  vm_int_opcode_t out = vm_int_run_read();
  vm_value_t len = locals[vm_int_run_read().reg];
  locals[out.reg] = vm_gc_new(&state->gc, vm_value_to_int(len));
  vm_int_run_next();
}
do_set_rrr : {
  vm_value_t obj = locals[vm_int_run_read().reg];
  vm_value_t key = locals[vm_int_run_read().reg];
  vm_value_t val = locals[vm_int_run_read().reg];
  vm_gc_set_vv(&state->gc, obj, key, val);
  vm_int_run_next();
}
do_set_rri : {
  vm_value_t obj = locals[vm_int_run_read().reg];
  vm_value_t key = locals[vm_int_run_read().reg];
  ptrdiff_t val = vm_int_run_read().val;
  vm_gc_set_vi(&state->gc, obj, key, val);
  vm_int_run_next();
}
do_set_rir : {
  vm_value_t obj = locals[vm_int_run_read().reg];
  ptrdiff_t key = vm_int_run_read().val;
  vm_value_t val = locals[vm_int_run_read().reg];
  vm_gc_set_iv(&state->gc, obj, key, val);
  vm_int_run_next();
}
do_set_rii : {
  vm_value_t obj = locals[vm_int_run_read().reg];
  ptrdiff_t key = vm_int_run_read().val;
  ptrdiff_t val = vm_int_run_read().val;
  vm_gc_set_ii(&state->gc, obj, key, val);
  vm_int_run_next();
}
do_get_rr : {
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t obj = locals[vm_int_run_read().reg];
  vm_value_t key = locals[vm_int_run_read().reg];
  *out = vm_gc_get_v(&state->gc, obj, key);
  vm_int_run_next();
}
do_get_ri : {
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t obj = locals[vm_int_run_read().reg];
  ptrdiff_t key = vm_int_run_read().val;
  *out = vm_gc_get_i(&state->gc, obj, key);
  vm_int_run_next();
}
do_len_r : {
  vm_value_t *out = &locals[vm_int_run_read().reg];
  vm_value_t obj = locals[vm_int_run_read().reg];
  *out = vm_value_from_int(vm_gc_len(&state->gc, obj));
  vm_int_run_next();
}
do_out_i : {
  fprintf(stdout, "%c", (int)vm_int_run_read().val);
  vm_int_run_next();
}
do_out_r : {
  fprintf(stdout, "%c",
          (int)vm_value_to_int(locals[vm_int_run_read().reg]));
  vm_int_run_next();
}
do_jump_l : {
  head = head->ptr;
  vm_int_run_next();
}
do_bb_rll : {
  ptrdiff_t val = vm_value_to_int(locals[vm_int_run_read().reg]);
  if (val != 0) {
    head = head[1].ptr;
  } else {
    head = head[0].ptr;
  }
  vm_int_run_next();
}
do_blt_rrll : {
  ptrdiff_t lhs = vm_value_to_int(locals[vm_int_run_read().reg]);
  ptrdiff_t rhs = vm_value_to_int(locals[vm_int_run_read().reg]);
  if (lhs < rhs) {
    head = head[1].ptr;
  } else {
    head = head[0].ptr;
  }
  vm_int_run_next();
}
do_blt_rill : {
  ptrdiff_t lhs = vm_value_to_int(locals[vm_int_run_read().reg]);
  ptrdiff_t rhs = vm_int_run_read().val;
  if (lhs < rhs) {
    head = head[1].ptr;
  } else {
    head = head[0].ptr;
  }
  vm_int_run_next();
}
do_blt_irll : {
  ptrdiff_t lhs = vm_int_run_read().val;
  ptrdiff_t rhs = vm_value_to_int(locals[vm_int_run_read().reg]);
  if (lhs < rhs) {
    head = head[1].ptr;
  } else {
    head = head[0].ptr;
  }
  vm_int_run_next();
}
do_beq_rrll : {
  ptrdiff_t lhs = vm_value_to_int(locals[vm_int_run_read().reg]);
  ptrdiff_t rhs = vm_value_to_int(locals[vm_int_run_read().reg]);
  if (lhs == rhs) {
    head = head[1].ptr;
  } else {
    head = head[0].ptr;
  }
  vm_int_run_next();
}
do_beq_rill : {
  ptrdiff_t lhs = vm_value_to_int(locals[vm_int_run_read().reg]);
  ptrdiff_t rhs = vm_int_run_read().val;
  if (lhs == rhs) {
    head = head[1].ptr;
  } else {
    head = head[0].ptr;
  }
  vm_int_run_next();
}
do_beq_irll : {
  ptrdiff_t lhs = vm_int_run_read().val;
  ptrdiff_t rhs = vm_value_to_int(locals[vm_int_run_read().reg]);
  if (lhs == rhs) {
    head = head[1].ptr;
  } else {
    head = head[0].ptr;
  }
  vm_int_run_next();
}
do_ret_i : {
  vm_value_t value = vm_value_from_int(vm_int_run_read().val);
  if (locals == init_locals) {
    return value;
  }
  head = *--state->heads;
  vm_int_opcode_t out = vm_int_run_read();
  locals -= state->framesize;
  locals[out.reg] = value;
  vm_int_run_next();
}
do_ret_r : {
  vm_value_t value = locals[vm_int_run_read().reg];
  if (locals == init_locals) {
    return value;
  }
  head = *--state->heads;
  vm_int_opcode_t out = vm_int_run_read();
  locals -= state->framesize;
  locals[out.reg] = value;
  vm_int_run_next();
}
do_exit : { return vm_value_from_int(0); }
do_call_t0 : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_call_l0;
  vm_int_opcode_t *block = &vm_int_run_read();
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_call_t1 : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_call_l1;
  vm_int_opcode_t *block = &vm_int_run_read();
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_call_t2 : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_call_l2;
  vm_int_opcode_t *block = &vm_int_run_read();
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_call_t3 : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_call_l3;
  vm_int_opcode_t *block = &vm_int_run_read();
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_call_t4 : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_call_l4;
  vm_int_opcode_t *block = &vm_int_run_read();
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_call_t5 : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_call_l5;
  vm_int_opcode_t *block = &vm_int_run_read();
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_call_t6 : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_call_l6;
  vm_int_opcode_t *block = &vm_int_run_read();
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_call_t7 : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_call_l7;
  vm_int_opcode_t *block = &vm_int_run_read();
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_call_t8 : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_call_l8;
  vm_int_opcode_t *block = &vm_int_run_read();
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_jump_t : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_jump_l;
  vm_int_opcode_t *block = &vm_int_run_read();
  vm_ir_block_t *base = vm_int_run_read().block;
  block->ptr = vm_int_block_comp(state, ptrs, block->block);
  head = loc;
  vm_int_run_next();
}
do_bb_rtt : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_bb_rll;
  head += 1;
  vm_int_opcode_t *block1 = &vm_int_run_read();
  vm_int_opcode_t *block2 = &vm_int_run_read();
  vm_ir_block_t *base = vm_int_run_read().block;
  block1->ptr = vm_int_block_comp(state, ptrs, block1->block);
  block2->ptr = vm_int_block_comp(state, ptrs, block2->block);
  head = loc;
  vm_int_run_next();
}
do_blt_rrtt : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_blt_rrll;
  head += 2;
  vm_int_opcode_t *block1 = &vm_int_run_read();
  vm_int_opcode_t *block2 = &vm_int_run_read();
  block1->ptr = vm_int_block_comp(state, ptrs, block1->block);
  block2->ptr = vm_int_block_comp(state, ptrs, block2->block);
  head = loc;
  vm_int_run_next();
}
do_blt_ritt : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_blt_rill;
  head += 2;
  vm_int_opcode_t *block1 = &vm_int_run_read();
  vm_int_opcode_t *block2 = &vm_int_run_read();
  vm_ir_block_t *base = vm_int_run_read().block;
  block1->ptr = vm_int_block_comp(state, ptrs, block1->block);
  block2->ptr = vm_int_block_comp(state, ptrs, block2->block);
  head = loc;
  vm_int_run_next();
}
do_blt_irtt : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_blt_irll;
  head += 2;
  vm_int_opcode_t *block1 = &vm_int_run_read();
  vm_int_opcode_t *block2 = &vm_int_run_read();
  vm_ir_block_t *base = vm_int_run_read().block;
  block1->ptr = vm_int_block_comp(state, ptrs, block1->block);
  block2->ptr = vm_int_block_comp(state, ptrs, block2->block);
  head = loc;
  vm_int_run_next();
}
do_beq_rrtt : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_beq_rrll;
  head += 2;
  vm_int_opcode_t *block1 = &vm_int_run_read();
  vm_int_opcode_t *block2 = &vm_int_run_read();
  block1->ptr = vm_int_block_comp(state, ptrs, block1->block);
  block2->ptr = vm_int_block_comp(state, ptrs, block2->block);
  head = loc;
  vm_int_run_next();
}
do_beq_ritt : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_beq_rill;
  head += 2;
  vm_int_opcode_t *block1 = &vm_int_run_read();
  vm_int_opcode_t *block2 = &vm_int_run_read();
  vm_ir_block_t *base = vm_int_run_read().block;
  block1->ptr = vm_int_block_comp(state, ptrs, block1->block);
  block2->ptr = vm_int_block_comp(state, ptrs, block2->block);
  head = loc;
  vm_int_run_next();
}
do_beq_irtt : {
  void *loc = --head;
  vm_int_run_read().ptr = &&do_beq_irll;
  head += 2;
  vm_int_opcode_t *block1 = &vm_int_run_read();
  vm_int_opcode_t *block2 = &vm_int_run_read();
  vm_ir_block_t *base = vm_int_run_read().block;
  block1->ptr = vm_int_block_comp(state, ptrs, block1->block);
  block2->ptr = vm_int_block_comp(state, ptrs, block2->block);
  head = loc;
  vm_int_run_next();
}
}

vm_value_t vm_ir_be_int3(size_t nblocks, vm_ir_block_t *blocks,
                         vm_int_func_t *funcs) {
  vm_ir_block_t *cur = &blocks[0];
  vm_int_state_t state = (vm_int_state_t){0};
  size_t nregs = 1 << 16;
  vm_value_t *locals = vm_malloc(sizeof(vm_value_t) * nregs);
  state.framesize = 1;
  state.funcs = funcs;
  for (size_t i = 0; i < nblocks; i++) {
    if (blocks[i].id == i) {
      if (blocks[i].nregs > state.framesize) {
        state.framesize = blocks[i].nregs;
      }
    }
  }
  state.locals = locals;
  state.heads = vm_malloc(sizeof(vm_int_opcode_t *) *
                          (nregs / (state.framesize - 1)));
  vm_value_t ret = vm_int_run(&state, cur);
  return ret;
}

#include "../opt.h"
#include "../toir.h"

vm_value_t vm_run_arch_int(size_t nops, vm_opcode_t *ops,
                           vm_int_func_t *funcs) {
  size_t nblocks = nops;
  vm_ir_block_t *blocks = vm_ir_parse(nblocks, ops);
  vm_ir_opt_all(&nblocks, &blocks);
  vm_value_t ret = vm_ir_be_int3(nblocks, blocks, funcs);
  vm_ir_blocks_free(nblocks, blocks);
  return ret;
}
