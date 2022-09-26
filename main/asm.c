
#include "../vm/asm.h"

#include "../vm/ir/be/int3.h"
#include "../vm/ir/build.h"
#include "../vm/ir/toir.h"

static const char *vm_asm_io_read(const char *filename) {
    void *file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }
    size_t nalloc = 16;
    char *ops = vm_malloc(sizeof(char) * nalloc);
    size_t nops = 0;
    size_t size;
    for (;;) {
        size = fread(&ops[nops], sizeof(char), 1, file);
        if (size == 0) {
            break;
        }
        nops += 1;
        if (nops + 4 >= nalloc) {
            nalloc *= 4;
            ops = vm_realloc(ops, sizeof(char) * nalloc);
        }
    }
    ops[nops] = '\0';
    fclose(file);
    return ops;
}

int main(int argc, char **argv) {
    bool iiiclose = false;
    FILE *iii = NULL;
    const char *iit = NULL;
    const char *dump = NULL;
    const char *filename = NULL;
    const char *debug = NULL;
    size_t jit = 1;
    size_t runs = 1;
    size_t jitdumpir = 0;
    size_t jitdumpopt = 0;
    while (true) {
        if (argc < 2) {
            if (filename == NULL) {
                fprintf(stderr, "too few args\n");
                return 1;
            } else {
                break;
            }
        }
        if (!strcmp(argv[1], "-o") || !strcmp(argv[1], "--output")) {
            argv += 1;
            argc -= 1;
            dump = argv[1];
            argv += 1;
            argc -= 1;
            continue;
        }
        if (!strcmp(argv[1], "-n")) {
            argv += 1;
            argc -= 1;
            size_t n = 0;
            char *ptr = argv[1];
            while (*ptr != '\0') {
                n *= 10;
                n += *ptr - '0';
                ptr += 1;
            }
            argv += 1;
            argc -= 1;
            runs = n;
            continue;
        }
        if (argv[1][0] == '-' && argv[1][1] == 'i') {
            char *tmp = argv[1] + 2;
            argv += 1;
            argc -= 1;
            if (!strcmp(tmp, "dump=pre")) {
                jitdumpir = 1;
            } else if (!strcmp(tmp, "dump=opt")) {
                jitdumpopt = 1;
            } else if (tmp[0] == 'i' && tmp[1] == 't') {
                iit = argv[1];
                argv += 1;
                argc -= 1;
            } else if (tmp[0] == 'i' && tmp[1] == 'i') {
                if (!strcmp(argv[1], "/dev/stdout")) {
                    iii = stdout;
                } else if (!strcmp(argv[1], "/dev/stderr")) {
                    iii = stderr;
                } else {
                    iii = fopen(argv[1], "w");
                    iiiclose = true;
                }
                argv += 1;
                argc -= 1;
            } else {
                fprintf(stderr, "unknown -j option: -j%s\n", tmp);
                return 1;
            }
            continue;
        }
        if (filename != NULL) {
            fprintf(stderr, "cannot handle multiple files at cli\n");
            return 1;
        } else {
            filename = argv[1];
            argv += 1;
            argc -= 1;
        }
    }
    if (!jit && (jitdumpopt || jitdumpir)) {
        fprintf(stderr, "cannot use -jdump with out jit turned on (-jon vs -joff)");
        return 1;
    }
    const char *src = vm_asm_io_read(filename);
    if (src == NULL) {
        fprintf(stderr, "could not read file\n");
        return 1;
    }
    for (size_t i = 0; i < runs; i++) {
        vm_bc_buf_t buf = vm_asm(src);
        if (dump != NULL) {
            FILE *out = fopen(dump, "wb");
            if (!out) {
                fprintf(stderr, "error opening output file");
                return 1;
            }
            fwrite(buf.ops, sizeof(vm_opcode_t), buf.nops, out);
            fclose(out);
        } else {
            size_t nblocks = buf.nops;
            vm_ir_block_t *blocks = vm_ir_parse(nblocks, buf.ops);
            if (jitdumpir) {
                vm_ir_print_blocks(stderr, nblocks, blocks);
            }
            if (jitdumpopt) {
                vm_ir_print_blocks(stderr, nblocks, blocks);
            }
            vm_ir_block_t *cur = &blocks[0];
            vm_int_state_t state = (vm_int_state_t){0};

            size_t nregs = 1 << 16;
            vm_value_t *locals = vm_alloc0(sizeof(vm_value_t) * nregs);
            state.debug_print_instrs = iii;
            state.framesize = 1;
            state.funcs = NULL;
            for (size_t i = 0; i < nblocks; i++) {
                if (blocks[i].id >= 0) {
                    if (blocks[i].nregs >= state.framesize) {
                        state.framesize = blocks[i].nregs + 1;
                    }
                }
            }
            state.locals = locals;
            state.heads = vm_malloc(sizeof(vm_int_opcode_t *) * (nregs / state.framesize + 1));
            if (iit != NULL) {
                state.use_spall = true;
                state.spall_ctx = vm_trace_init(iit, 0.000303);
                vm_trace_begin(&state.spall_ctx, NULL, vm_trace_time(), "MiniVM Invocation");
            }
            vm_gc_init(&state.gc, nregs, locals);
            vm_value_t ret = vm_int_run(&state, cur);
            vm_gc_deinit(&state.gc);
            if (iit != NULL) {
                vm_trace_quit(&state.spall_ctx);
                vm_trace_end(&state.spall_ctx, NULL, vm_trace_time());
            }
            vm_ir_blocks_free(nblocks, blocks);
        }
        vm_free(buf.ops);
    }
    if (iiiclose) {
        fclose(iii);
    }
    vm_free((void *)src);
    return 0;
}
