#if !defined(VM_HEADER_TAG)
#define VM_HEADER_TAG

enum {
    VM_TAG_INIT,
    VM_TAG_UNK,
    VM_TAG_NIL,
    VM_TAG_BOOL,
    VM_TAG_I8,
    VM_TAG_I16,
    VM_TAG_I32,
    VM_TAG_I64,
    VM_TAG_U8,
    VM_TAG_U16,
    VM_TAG_U32,
    VM_TAG_U64,
    VM_TAG_F32,
    VM_TAG_F64,
    VM_TAG_MEM,
    VM_TAG_FN,
};

static const char *vm_tag_to_str(uint8_t tag) {
    static const char *table[] = {
        [VM_TAG_INIT] = "init",
        [VM_TAG_UNK] = "unk",
        [VM_TAG_NIL] = "nil",
        [VM_TAG_BOOL] = "bool",
        [VM_TAG_I8] = "i8",
        [VM_TAG_I16] = "i16",
        [VM_TAG_I32] = "i32",
        [VM_TAG_I64] = "i64",
        [VM_TAG_U8] = "u8",
        [VM_TAG_U16] = "u16",
        [VM_TAG_U32] = "u32",
        [VM_TAG_U64] = "u64",
        [VM_TAG_F32] = "f32",
        [VM_TAG_F64] = "f64",
        [VM_TAG_MEM] = "mem",
        [VM_TAG_FN] = "fn",
    };
    return table[tag];
}

#endif
