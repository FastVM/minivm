#if !defined(VM_HEADER_LANG_PAKA)
#define VM_HEADER_LANG_PAKA

#include "../lib.h"
#include "../ir.h"

struct vm_paka_parser_t;
typedef struct vm_paka_parser_t vm_paka_parser_t;

struct vm_paka_comp_t;
typedef struct vm_paka_comp_t vm_paka_comp_t;

struct vm_paka_parser_t {
    const char *src;
    size_t index;
    size_t line;
    size_t col;
};

struct vm_paka_comp_t {
    size_t nregs;
    vm_block_t *write;
    vm_block_t *jump;
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
bool vm_paka_parser_match_if(vm_paka_parser_t *parser, bool(*fn)(char));
// cleaners
void vm_paka_parser_strip_spaces(vm_paka_parser_t *parser);
// words and keywords
size_t vm_paka_parser_ident_len(vm_paka_parser_t *parser);
bool vm_paka_parser_match_keyword(vm_paka_parser_t *parser, const char *keyword);
// grammar parts
size_t vm_paka_parser_expr_base(vm_paka_parser_t *src, vm_paka_comp_t *comp);
void vm_paka_parser_block(vm_paka_parser_t *parser, vm_paka_comp_t *comp);
// string parsers
vm_block_t *vm_paka_parse(const char *src);

#endif