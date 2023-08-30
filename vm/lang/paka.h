#if !defined(VM_HEADER_LANG_PAKA)
#define VM_HEADER_LANG_PAKA

#include "../ir.h"
#include "../lib.h"

struct vm_paka_parser_t;
typedef struct vm_paka_parser_t vm_paka_parser_t;

struct vm_paka_blocks_t;
typedef struct vm_paka_blocks_t vm_paka_blocks_t;

struct vm_paka_comp_t;
typedef struct vm_paka_comp_t vm_paka_comp_t;

struct vm_paka_name_map_t;
typedef struct vm_paka_name_map_t vm_paka_name_map_t;

struct vm_paka_parser_t {
    const char *src;
    size_t index;
    size_t line;
    size_t col;
};

struct vm_paka_blocks_t {
    vm_block_t **blocks;
    size_t len;
    size_t alloc;
};

struct vm_paka_comp_t {
    vm_block_t *write;
    size_t *regs;
    vm_paka_name_map_t *names;
    vm_paka_blocks_t *blocks;
    size_t nfuncs;
    const char **func_names;
    vm_block_t **func_blocks;
    size_t funcs_alloc;
};

struct vm_paka_name_map_t {
    vm_paka_name_map_t *next;
    const char **keys;
    uint8_t *values;
    size_t len;
    size_t alloc;
};

// char test funcs
bool vm_paka_parser_is_ident0_char(char c);
bool vm_paka_parser_is_ident1_char(char c);
// very basics
void vm_paka_parser_skip(vm_paka_parser_t *parser);
char vm_paka_parser_peek(vm_paka_parser_t *parser);
char vm_paka_parser_read(vm_paka_parser_t *parser);
size_t vm_paka_parser_tell(vm_paka_parser_t *parser);
// if it matches, advance
bool vm_paka_parser_match(vm_paka_parser_t *parser, const char *str);
bool vm_paka_parser_match_if(vm_paka_parser_t *parser, bool (*fn)(char));
// cleaners
void vm_paka_parser_strip_spaces(vm_paka_parser_t *parser);
// words and keywords
size_t vm_paka_parser_ident_len(vm_paka_parser_t *parser);
bool vm_paka_parser_match_keyword(vm_paka_parser_t *parser,
                                  const char *keyword);
// grammar parts
bool vm_paka_parser_branch(vm_paka_parser_t *parser, vm_paka_comp_t *comp,
                           vm_block_t *iftrue, vm_block_t *iffalse);
vm_arg_t vm_paka_parser_expr_single(vm_paka_parser_t *src,
                                    vm_paka_comp_t *comp);
vm_arg_t vm_paka_parser_expr_mul(vm_paka_parser_t *src, vm_paka_comp_t *comp);
vm_arg_t vm_paka_parser_expr_add(vm_paka_parser_t *src, vm_paka_comp_t *comp);
vm_arg_t vm_paka_parser_expr_base(vm_paka_parser_t *src, vm_paka_comp_t *comp);
vm_arg_t vm_paka_parser_postfix(vm_paka_parser_t *parser, vm_paka_comp_t *comp,
                                vm_arg_t arg);
int vm_paka_parser_block(vm_paka_parser_t *parser, vm_paka_comp_t *comp);
// string parsers
vm_block_t *vm_paka_parse(const char *src);
vm_paka_blocks_t vm_paka_parse_blocks(const char *src);

#endif
