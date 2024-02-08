// This is the table of all peephole rewrite rules, there's a few categories:
//
// * Idealize: replaces a node with a simplified form, this is run until exhaustion
//             on a node (although in practice will warn past 5 trips, it's weird to
//             even do more than 2 trips)
//
// * Identity: replace a node with it's direct inputs (one step).
//
// * SCCP:     runs SCCP on a node.
//
// * GVN:      if a node has some identical copy, it will be replaced with it.
//
// They're run in this order for every node and given each is well-formed (one step Church-Rosser)
// the number of rewrites performed should scale linearly with the size of the IR.
typedef TB_Node* (*NodeIdealize)(TB_Passes* restrict p, TB_Function* f, TB_Node* n);
typedef TB_Node* (*NodeIdentity)(TB_Passes* restrict p, TB_Function* f, TB_Node* n);
typedef Lattice* (*NodeConstprop)(TB_Passes* restrict p, TB_Node* n);

typedef struct {
    NodeIdealize  idealize;
    NodeIdentity  identity;
    NodeConstprop constprop;
} NodeVtable;

static const NodeVtable vtables[TB_NODE_TYPE_MAX] = {
    // type                 ideal              identity            sccp
    [TB_INTEGER_CONST]  = { NULL,              NULL,               sccp_int         },
    // memory
    [TB_LOAD]           = { ideal_load,        identity_load,      NULL             },
    [TB_STORE]          = { ideal_store,       NULL,               sccp_mem         },
    [TB_MEMSET]         = { ideal_memset,      NULL,               sccp_mem         },
    [TB_MEMCPY]         = { ideal_memcpy,      NULL,               sccp_mem         },
    [TB_SPLITMEM]       = { NULL,              NULL,               sccp_split_mem   },
    [TB_MERGEMEM]       = { ideal_merge_mem,   NULL,               sccp_merge_mem   },
    // ptr values
    [TB_LOCAL]          = { NULL,              NULL,               sccp_ptr_vals    },
    [TB_SYMBOL]         = { NULL,              NULL,               sccp_ptr_vals    },
    // pointer arithmetic
    [TB_MEMBER_ACCESS]  = { ideal_member_ptr,  identity_member_ptr,NULL             },
    [TB_ARRAY_ACCESS]   = { ideal_array_ptr,   NULL,               NULL             },
    // arithmetic
    [TB_ADD]            = { ideal_int_binop,   identity_int_binop, sccp_arith       },
    [TB_SUB]            = { ideal_int_binop,   identity_int_binop, sccp_arith       },
    [TB_MUL]            = { ideal_int_binop,   identity_int_binop, sccp_arith       },
    [TB_UDIV]           = { ideal_int_div,     identity_int_binop, NULL             },
    [TB_SDIV]           = { ideal_int_div,     identity_int_binop, NULL             },
    [TB_UMOD]           = { ideal_int_mod,     identity_int_binop, NULL             },
    [TB_SMOD]           = { ideal_int_mod,     identity_int_binop, NULL             },
    // comparisons
    [TB_CMP_EQ]         = { ideal_int_binop,   identity_int_binop, sccp_cmp         },
    [TB_CMP_NE]         = { ideal_int_binop,   identity_int_binop, sccp_cmp         },
    [TB_CMP_SLT]        = { ideal_int_binop,   identity_int_binop, sccp_cmp         },
    [TB_CMP_SLE]        = { ideal_int_binop,   identity_int_binop, sccp_cmp         },
    [TB_CMP_ULT]        = { ideal_int_binop,   identity_int_binop, sccp_cmp         },
    [TB_CMP_ULE]        = { ideal_int_binop,   identity_int_binop, sccp_cmp         },
    // bitwise ops
    [TB_AND]            = { ideal_int_binop,   identity_int_binop, sccp_bits        },
    [TB_OR]             = { ideal_int_binop,   identity_int_binop, sccp_bits        },
    [TB_XOR]            = { ideal_int_binop,   identity_int_binop, sccp_bits        },
    // shift
    [TB_SHL]            = { ideal_int_binop,   identity_int_binop, sccp_shift       },
    [TB_SHR]            = { ideal_int_binop,   identity_int_binop, sccp_shift       },
    [TB_SAR]            = { ideal_int_binop,   identity_int_binop, sccp_shift       },
    // unary
    [TB_NEG]            = { NULL,              NULL,               sccp_unary       },
    [TB_NOT]            = { NULL,              NULL,               sccp_unary       },
    // casts
    [TB_BITCAST]        = { ideal_bitcast,     NULL,               sccp_bitcast     },
    [TB_TRUNCATE]       = { ideal_truncate,    NULL,               sccp_trunc       },
    [TB_ZERO_EXT]       = { ideal_extension,   NULL,               sccp_zext        },
    [TB_SIGN_EXT]       = { ideal_extension,   NULL,               sccp_sext        },
    // misc
    [TB_LOOKUP]         = { NULL,              NULL,               sccp_lookup      },
    [TB_PROJ]           = { NULL,              NULL,               sccp_proj        },
    [TB_SELECT]         = { ideal_select,      NULL,               sccp_meetchads   },
    [TB_PHI]            = { ideal_phi,         identity_phi,       sccp_meetchads   },
    // control flow
    [TB_REGION]         = { ideal_region,      identity_region,    sccp_region      },
    [TB_BRANCH]         = { ideal_branch,      NULL,               sccp_branch      },
    [TB_SAFEPOINT_POLL] = { NULL,              identity_safepoint, sccp_ctrl        },
    [TB_CALL]           = { ideal_libcall,     identity_ctrl,      sccp_call        },
    [TB_TAILCALL]       = { NULL,              identity_ctrl,      sccp_ctrl        },
    [TB_SYSCALL]        = { NULL,              identity_ctrl,      sccp_call        },
    [TB_DEBUGBREAK]     = { NULL,              identity_ctrl,      sccp_ctrl        },
    [TB_TRAP]           = { NULL,              identity_ctrl,      sccp_ctrl        },
    [TB_UNREACHABLE]    = { NULL,              identity_ctrl,      sccp_ctrl        },
};
