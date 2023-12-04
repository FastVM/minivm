#if !defined(VM_HEADER_LANG_LUA_REPL)
#define VM_HEADER_LANG_LUA_REPL

#include "../config.h"
#include "../obj.h"
#include "../lib.h"
#include "../../isocline/isocline.h"
#include "../../isocline/completions.h"

struct vm_lang_lua_repl_complete_state_t;
struct vm_lang_lua_repl_complete_state_t;

typedef struct vm_lang_lua_repl_complete_state_t vm_lang_lua_repl_complete_state_t;
typedef struct vm_lang_lua_repl_highlight_state_t vm_lang_lua_repl_highlight_state_t;

struct vm_lang_lua_repl_complete_state_t {
    vm_config_t *config;
    vm_table_t *std;
};
struct vm_lang_lua_repl_highlight_state_t {
    vm_config_t *config;
    vm_table_t *std;
};

#endif
