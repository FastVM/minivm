
#include "spall.h"

#if defined(SPALL_BUFFER_PROFILING) && !defined(SPALL_BUFFER_PROFILING_GET_TIME)
#error "You must #define SPALL_BUFFER_PROFILING_GET_TIME() to profile buffer flushes."
#endif

#ifdef SPALL_BUFFER_PROFILING
#define SPALL_BUFFER_PROFILE_BEGIN() double spall_time_begin = (SPALL_BUFFER_PROFILING_GET_TIME())
// Don't call this with anything other than a string literal
#define SPALL_BUFFER_PROFILE_END(name)                                                                                                                         \
    do {                                                                                                                                                       \
        double spall_time_end = (SPALL_BUFFER_PROFILING_GET_TIME());                                                                                           \
        char temp_buffer_data[sizeof(vm_trace_begin_event_t) + sizeof("" name "") - 2 + sizeof(vm_trace_EndEvent)];                                            \
        vm_trace_buffer_t temp_buffer = {temp_buffer_data, sizeof(temp_buffer_data)};                                                                          \
        if (!vm_trace_begin_len_tid_pid(ctx, &temp_buffer, spall_time_begin, name, sizeof(name) - 1, (uint32_t)(uintptr_t)wb->data, 4222222222)) return false; \
        if (!vm_trace_end_tid_pid(ctx, &temp_buffer, spall_time_end, (uint32_t)(uintptr_t)wb->data, 4222222222)) return false;                                 \
        vm_trace___FileWrite(ctx, temp_buffer_data, sizeof(temp_buffer_data));                                                                                 \
    } while (0)
#else
#define SPALL_BUFFER_PROFILE_BEGIN()
#define SPALL_BUFFER_PROFILE_END(name)
#endif

static bool vm_trace___FileWrite(vm_trace_profile_t *ctx, void *p, size_t n) {
    if (!ctx->file) return false;
    // if (feof(ctx->file)) return false;
    if (ferror(ctx->file)) return false;
    if (fwrite(p, n, 1, ctx->file) != 1) return false;
    return true;
}
static bool vm_trace___FileFlush(vm_trace_profile_t *ctx) {
    if (!ctx->file) return false;
    if (fflush(ctx->file)) return false;
    return true;
}
// TODO: vm_trace___FilePrintf
// TODO: vm_trace___FileClose

static bool vm_trace___BufferFlush(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb) {
    // precon: wb
    // precon: wb->data
    // precon: wb->head <= wb->length
    if (wb->ctx != ctx) return false;  // Buffer must be bound to this context (or to NULL)
    if (wb->head && ctx) {
        if (!ctx->file) return false;
        SPALL_BUFFER_PROFILE_BEGIN();
        if (!ctx->write(ctx, wb->data, wb->head)) return false;
        SPALL_BUFFER_PROFILE_END("Buffer Flush");
    }
    wb->head = 0;
    wb->recent_string_index = 0;
    memset(wb->recent_strings, 0, sizeof(wb->recent_strings));
    return true;
}

static bool vm_trace___BufferWrite(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, void *p, size_t n) {
    // precon: !wb || wb->head < wb->length
    // precon: ctx->file
    if (!wb) return ctx->write(ctx, p, n);
    if (wb->head + n > wb->length && !vm_trace___BufferFlush(ctx, wb)) return false;
    if (n > wb->length) {
        SPALL_BUFFER_PROFILE_BEGIN();
        if (!ctx->write(ctx, p, n)) return false;
        SPALL_BUFFER_PROFILE_END("Unbuffered Write");
        return true;
    }
    memcpy((char *)wb->data + wb->head, p, n);
    wb->head += n;
    return true;
}

bool vm_trace_buffer_flush(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb) {
    if (!wb) return false;
    if (!wb->data) return false;
    if (!vm_trace___BufferFlush(ctx, wb)) return false;
    return true;
}

bool vm_trace_buffer_init(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb) {
    if (!vm_trace_buffer_flush(NULL, wb)) return false;
    wb->ctx = ctx;
    return true;
}
bool vm_trace_buffer_quit(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb) {
    if (!vm_trace_buffer_flush(ctx, wb)) return false;
    wb->ctx = NULL;
    return true;
}

static vm_trace_profile_t vm_trace___Init(const char *filename, double timestamp_unit, bool is_json) {
    vm_trace_profile_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    if (timestamp_unit < 0) return ctx;
    if (!filename) return ctx;
    ctx.file = fopen(filename, "wb");  // TODO: handle utf8 and long paths on windows
    ctx.write = vm_trace___FileWrite;
    ctx.flush = vm_trace___FileFlush;
    if (!ctx.file) {
        vm_trace_quit(&ctx);
        return ctx;
    }
    ctx.timestamp_unit = timestamp_unit;
    ctx.is_json = is_json;
    if (ctx.is_json) {
        if (fprintf(ctx.file, "{\"traceEvents\":[\n") <= 0) {
            vm_trace_quit(&ctx);
            return ctx;
        }
    } else {
        vm_trace_header_t header;
        header.magic_header = 0x0BADF00D;
        header.version = 0;
        header.timestamp_unit = timestamp_unit;
        header.must_be_0 = 0;
        if (!ctx.write(&ctx, &header, sizeof(header))) {
            vm_trace_quit(&ctx);
            return ctx;
        }
    }
    return ctx;
}

vm_trace_profile_t vm_trace_init(const char *filename, double timestamp_unit) { return vm_trace___Init(filename, timestamp_unit, false); }
vm_trace_profile_t vm_trace_init_json(const char *filename, double timestamp_unit) { return vm_trace___Init(filename, timestamp_unit, true); }

void vm_trace_quit(vm_trace_profile_t *ctx) {
    if (!ctx) return;
    if (ctx->file) {
        if (ctx->is_json) {
            fseek(ctx->file, -2, SEEK_CUR);  // seek back to overwrite trailing comma
            fprintf(ctx->file, "\n]}\n");
        }
        fflush(ctx->file);
        fclose(ctx->file);
    }
    memset(ctx, 0, sizeof(*ctx));
}

bool vm_trace_begin_len_tid_pid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, signed long name_len, uint32_t tid, uint32_t pid) {
    vm_trace_begin_event_max_t ev;
    if (!ctx) return false;
    if (!name) return false;
    if (!ctx->file) return false;
    // if (ctx->times_are_u64) return false;
    if (name_len <= 0) return false;
    if (name_len > 255) name_len = 255;  // will be interpreted as truncated in the app (?)
    ev.event.type = VM_TRACE_EVENT_TYPE_BEGIN;
    ev.event.pid = pid;
    ev.event.tid = tid;
    ev.event.when = when;
    ev.event.name.length = (uint8_t)name_len;
    memcpy(ev.event.name.bytes, name, (uint8_t)name_len);
    if (ctx->is_json) {
        if (fprintf(ctx->file,
                    "{\"name\":\"%.*s\",\"ph\":\"B\",\"pid\":%u,\"tid\":%u,\"ts\":%f},\n",
                    (int)ev.event.name.length, ev.event.name.bytes,
                    ev.event.pid,
                    ev.event.tid,
                    ev.event.when * ctx->timestamp_unit) <= 0) return false;
    } else {
        if (!vm_trace___BufferWrite(ctx, wb, &ev, sizeof(vm_trace_begin_event_t) + (uint8_t)name_len - 1)) return false;
    }
    return true;
}
bool vm_trace_begin_tid_pid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, uint32_t tid, uint32_t pid) {
    unsigned long name_len;
    if (!name) return false;
    name_len = strlen(name);
    if (!name_len) return false;
    return vm_trace_begin_len_tid_pid(ctx, wb, when, name, (signed long)name_len, tid, pid);
}
bool vm_trace_begin_len_tid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, signed long name_len, uint32_t tid) { return vm_trace_begin_len_tid_pid(ctx, wb, when, name, name_len, tid, 0); }
bool vm_trace_begin_len(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, signed long name_len) { return vm_trace_begin_len_tid_pid(ctx, wb, when, name, name_len, 0, 0); }
bool vm_trace_begin_tid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name, uint32_t tid) { return vm_trace_begin_tid_pid(ctx, wb, when, name, tid, 0); }
bool vm_trace_begin(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, const char *name) { return vm_trace_begin_tid_pid(ctx, wb, when, name, 0, 0); }

bool vm_trace_end_tid_pid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, uint32_t tid, uint32_t pid) {
    vm_trace_EndEvent ev;
    if (!ctx) return false;
    if (!ctx->file) return false;
    // if (ctx->times_are_u64) return false;
    ev.type = VM_TRACE_EVENT_TYPE_END;
    ev.pid = pid;
    ev.tid = tid;
    ev.when = when;
    if (ctx->is_json) {
        if (fprintf(ctx->file,
                    "{\"ph\":\"E\",\"pid\":%u,\"tid\":%u,\"ts\":%f},\n",
                    ev.pid,
                    ev.tid,
                    ev.when * ctx->timestamp_unit) <= 0) return false;
    } else {
        if (!vm_trace___BufferWrite(ctx, wb, &ev, sizeof(ev))) return false;
    }
    return true;
}

bool vm_trace_end_tid(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when, uint32_t tid) { return vm_trace_end_tid_pid(ctx, wb, when, tid, 0); }
bool vm_trace_end(vm_trace_profile_t *ctx, vm_trace_buffer_t *wb, double when) { return vm_trace_end_tid_pid(ctx, wb, when, 0, 0); }
