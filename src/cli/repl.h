
#pragma once

#if !defined(MX_HEADER_CLI_REPL)
#define MX_HEADER_CLI_REPL

#include "main.h"
#include "../../vendor/isocline/include/isocline.h"
#include "../vm/vm.h"

#define MX_REPL_PROMPT ""
#define MX_REPL_CONT_PROMPT ">"
#define MX_REPL_HISTORY_MAX 200
#define MX_REPL_INCOMPLETE_MARK "<eof>"
#define MX_REPL_RETURN_PREFIX "return "
#define MX_REPL_CHUNK "=stdin"

void mx_repl_completer(ic_completion_env_t *cenv, const char *prefix);
bool mx_repl_is_incomplete(mx_error_t *err);
void mx_repl_print_results(mx_t *vm);
bool mx_repl_try_expression(mx_t *vm, const char *prog, const char *line);
void mx_repl_run_chunk(mx_t *vm, const char *prog, mx_ir_block_t *entry);
mx_main_exit_t mx_repl_run(mx_t *vm, const char *prog);

#endif
