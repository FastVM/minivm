/**
* Copyright (c) 2020 rxi
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the MIT license. See `log.c` for details.
*
* Modified by NeGate to enforce C11 mutexes
*/
#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#define LOG_VERSION "0.1.0"

typedef struct {
    va_list ap;
    const char *fmt;
    const char *file;
    struct tm *time;
    void *udata;
    int tid;
    int line;
    int level;
} log_Event;

typedef void (*log_LogFn)(log_Event *ev);
typedef void (*log_LockFn)(bool lock, void *udata);

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#if !defined(LOG_SUPPRESS)
#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define log_watch(fmt, var) log_log(LOG_DEBUG, __FILE__, __LINE__, #var " = " fmt, (uint64_t)var)
#else
#define log_trace(...) (__VA_ARGS__, 0)
#define log_debug(...) (__VA_ARGS__, 0)
#define log_info(...)  (__VA_ARGS__, 0)
#define log_warn(...)  (__VA_ARGS__, 0)
#define log_error(...) (__VA_ARGS__, 0)
#define log_fatal(...) (__VA_ARGS__, 0)
#define log_watch(fmt, var) (var, 0)
#endif

const char* log_level_string(int level);
void log_set_level(int level);
void log_set_quiet(bool enable);
int log_add_callback(log_LogFn fn, void *udata, int level);
int log_add_fp(FILE *fp, int level);

void log_log(int level, const char *file, int line, const char *fmt, ...);
