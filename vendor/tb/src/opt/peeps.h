// This is the table of all peephole rewrite rules, there's a few categories:
//
// * Idealize: replaces a node with a simplified form, this is run until exhaustion
//             on a node (although in practice will warn past 5 trips, it's weird to
//             even do more than 2 trips)
//
// * Identity: replace a node with it's direct inputs (one step).
//
// * ValueOf:  constant propagation crap (works for pessimistic and optimistic crap out of the box)
//
// * GVN:      if a node has some identical copy, it will be replaced with it.
//
// They're run in this order for every node and given each is well-formed (one step Church-Rosser)
// the number of rewrites performed should scale linearly with the size of the IR.
typedef TB_Node* (*NodeIdealize)(TB_Passes* restrict p, TB_Function* f, TB_Node* n);
typedef TB_Node* (*NodeIdentity)(TB_Passes* restrict p, TB_Function* f, TB_Node* n);
typedef Lattice* (*NodeValueOf)(TB_Passes* restrict p, TB_Node* n);

enum {
    NODE_IS_CTRL = 1,
};

typedef struct {
    NodeIdealize  idealize;
    NodeIdentity  identity;
    NodeValueOf   value;
    uint32_t      flags;
} NodeVtable;

static const NodeVtable vtables[TB_NODE_TYPE_MAX] = {
    // type                 ideal              identity            value
    [TB_INTEGER_CONST]  = { NULL,              NULL,               value_int        },
    // memory
    [TB_LOAD]           = { ideal_load,        identity_load,      NULL             },
    [TB_STORE]          = { ideal_store,       NULL,               value_mem        },
    [TB_MEMSET]         = { ideal_memset,      NULL,               value_mem        },
    [TB_MEMCPY]         = { ideal_memcpy,      NULL,               value_mem        },
    [TB_SPLITMEM]       = { NULL,              NULL,               value_split_mem  },
    [TB_MERGEMEM]       = { ideal_merge_mem,   NULL,               value_merge_mem  },
    // ptr values
    [TB_LOCAL]          = { NULL,              NULL,               value_ptr_vals   },
    [TB_SYMBOL]         = { NULL,              NULL,               value_ptr_vals   },
    // pointer arithmetic
    [TB_MEMBER_ACCESS]  = { ideal_member_ptr,  identity_member_ptr,NULL             },
    [TB_ARRAY_ACCESS]   = { ideal_array_ptr,   NULL,               NULL             },
    // arithmetic
    [TB_ADD]            = { ideal_int_binop,   identity_int_binop, value_arith      },
    [TB_SUB]            = { ideal_int_binop,   identity_int_binop, value_arith      },
    [TB_MUL]            = { ideal_int_binop,   identity_int_binop, value_arith      },
    [TB_UDIV]           = { ideal_int_div,     identity_int_binop, NULL             },
    [TB_SDIV]           = { ideal_int_div,     identity_int_binop, NULL             },
    [TB_UMOD]           = { ideal_int_mod,     identity_int_binop, NULL             },
    [TB_SMOD]           = { ideal_int_mod,     identity_int_binop, NULL             },
    // comparisons
    [TB_CMP_EQ]         = { ideal_int_binop,   identity_int_binop, value_cmp        },
    [TB_CMP_NE]         = { ideal_int_binop,   identity_int_binop, value_cmp        },
    [TB_CMP_SLT]        = { ideal_int_binop,   identity_int_binop, value_cmp        },
    [TB_CMP_SLE]        = { ideal_int_binop,   identity_int_binop, value_cmp        },
    [TB_CMP_ULT]        = { ideal_int_binop,   identity_int_binop, value_cmp        },
    [TB_CMP_ULE]        = { ideal_int_binop,   identity_int_binop, value_cmp        },
    // bitwise ops
    [TB_AND]            = { ideal_int_binop,   identity_int_binop, value_bits       },
    [TB_OR]             = { ideal_int_binop,   identity_int_binop, value_bits       },
    [TB_XOR]            = { ideal_int_binop,   identity_int_binop, value_bits       },
    // shift
    [TB_SHL]            = { ideal_int_binop,   identity_int_binop, value_shift      },
    [TB_SHR]            = { ideal_int_binop,   identity_int_binop, value_shift      },
    [TB_SAR]            = { ideal_int_binop,   identity_int_binop, value_shift      },
    // unary
    [TB_NEG]            = { NULL,              NULL,               value_unary      },
    [TB_NOT]            = { NULL,              NULL,               value_unary      },
    // casts
    [TB_BITCAST]        = { ideal_bitcast,     NULL,               value_bitcast    },
    [TB_TRUNCATE]       = { ideal_truncate,    NULL,               value_trunc      },
    [TB_ZERO_EXT]       = { ideal_extension,   NULL,               value_zext       },
    [TB_SIGN_EXT]       = { ideal_extension,   NULL,               value_sext       },
    // misc
    [TB_LOOKUP]         = { NULL,              NULL,               value_lookup     },
    [TB_PROJ]           = { NULL,              NULL,               value_proj       },
    [TB_SELECT]         = { ideal_select,      NULL,               value_select     },
    [TB_PHI]            = { ideal_phi,         identity_phi,       value_phi        },
    // control flow
    [TB_RETURN]         = { ideal_return,      NULL,               value_ctrl,       NODE_IS_CTRL },
    [TB_REGION]         = { ideal_region,      identity_region,    value_region,     NODE_IS_CTRL },
    [TB_NATURAL_LOOP]   = { ideal_region,      identity_region,    value_region,     NODE_IS_CTRL },
    [TB_AFFINE_LOOP]    = { ideal_region,      identity_region,    value_region,     NODE_IS_CTRL },
    [TB_BRANCH]         = { ideal_branch,      NULL,               value_branch,     NODE_IS_CTRL },
    [TB_SAFEPOINT_POLL] = { NULL,              identity_safepoint, value_ctrl,       NODE_IS_CTRL },
    [TB_CALL]           = { ideal_libcall,     NULL,               value_call,       NODE_IS_CTRL },
    [TB_TAILCALL]       = { NULL,              NULL,               value_ctrl,       NODE_IS_CTRL },
    [TB_SYSCALL]        = { NULL,              NULL,               value_call,       NODE_IS_CTRL },
    [TB_DEBUGBREAK]     = { NULL,              NULL,               value_ctrl,       NODE_IS_CTRL },
    [TB_TRAP]           = { NULL,              NULL,               value_ctrl,       NODE_IS_CTRL },
    [TB_UNREACHABLE]    = { NULL,              NULL,               value_ctrl,       NODE_IS_CTRL },
};

