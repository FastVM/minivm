
#pragma once

#if !defined(MX_HEADER_CLI_MAIN)
#define MX_HEADER_CLI_MAIN

#include "../vm/vm.h"

#define MX_MAIN_BANNER "MiniVM " MX_VERSION " for Lua 5.5  Copyright (C) 2020-2026 Shaw Summa"
#define MX_MAIN_CHUNK_COMMAND "=(command line)"

typedef uint8_t mx_main_exit_t;
enum {
    MX_MAIN_EXIT_OK,
    MX_MAIN_EXIT_ERROR,
};

typedef uint8_t mx_main_step_kind_t;
enum {
    MX_MAIN_STEP_EXEC,
    MX_MAIN_STEP_REQUIRE,
    MX_MAIN_STEP_WARN,
};

typedef struct mx_main_step_t mx_main_step_t;
typedef struct mx_main_cmd_t mx_main_cmd_t;

struct mx_main_step_t {
    const char *text;
    mx_main_step_kind_t kind;
};

struct mx_main_cmd_t {
    mx_main_step_t *steps;
    uint32_t nsteps;
    int32_t script;
    const char *bad;
    bool is_stdin_script: 1;
    bool is_missing_arg: 1;
    bool is_ext: 1;
    bool is_noenv: 1;
    bool is_interactive: 1;
    bool is_version: 1;
    bool has_exec: 1;
};

void mx_main_usage(const char *prog, const mx_main_cmd_t *cmd);
mx_main_cmd_t mx_main_parse(int32_t argc, char **argv);
void mx_main_drop_extensions(mx_t *vm);
void mx_main_report(mx_t *vm, const char *prog, mx_obj_t err);
void mx_main_msgh(mx_t *vm, size_t nargs, mx_obj_t *args);
mx_obj_t mx_main_call(mx_t *vm, mx_obj_t fn);
mx_main_exit_t mx_main_run_source(mx_t *vm, const char *prog, const char *src, uint32_t len, const char *chunkname);
mx_main_exit_t mx_main_run_file(mx_t *vm, const char *prog, const char *fname);
mx_main_exit_t mx_main_require(mx_t *vm, const char *prog, const char *spec);
mx_main_exit_t mx_main_run_init(mx_t *vm, const char *prog);
mx_main_exit_t mx_main_run_steps(mx_t *vm, const char *prog, const mx_main_cmd_t *cmd);

#endif
