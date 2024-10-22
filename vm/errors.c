
#include "errors.h"
#include "io.h"

typedef struct {
    size_t line;
    size_t col;
} vm_place_t;

vm_error_t *vm_error_from_msg(vm_location_range_t range, const char *msg) {
    vm_error_t *err = vm_malloc(sizeof(vm_error_t));
    *err = (vm_error_t) {
        .msg = vm_strdup(msg),
        .range = range,
        .child = NULL,
    };
    return err;
}

vm_error_t *vm_error_from_error(vm_location_range_t range, vm_error_t *child) {
    vm_error_t *err = vm_malloc(sizeof(vm_error_t));
    *err = (vm_error_t) {
        .msg = NULL,
        .range = range,
        .child = child,
    };
    return err;
}

static vm_place_t vm_location_get_line_num(const char *src, vm_location_t loc) {
    vm_place_t place = (vm_place_t) {
        .line = 1,
        .col = 1,
    };
    for (size_t i = 0; i < loc.byte; i++) {
        if (src[i] == '\n') {
            place.line += 1;
            place.col = 1;
        } else {
            place.col += 1;
        }
    }
    return place;
}

const char *vm_error_report_to_string(vm_error_t *error) {
    vm_io_buffer_t *buf = vm_io_buffer_new();
    while (error != NULL) {
        if (error->child != NULL) {
            vm_place_t start = vm_location_get_line_num(error->range.src, error->range.start);
            // vm_place_t stop = vm_location_get_line_num(error->range.src, error->range.stop);
            if (error->range.src != NULL) {
                vm_io_buffer_format(buf, "in: %s:%zu:%zu\n", error->range.file, start.line, start.col);
            }
        } else if (error->msg != NULL) {
            vm_place_t start = vm_location_get_line_num(error->range.src, error->range.start);
            // vm_place_t stop = vm_location_get_line_num(error->range.src, error->range.stop);
            if (error->range.src != NULL) {
                vm_io_buffer_format(buf, "at: %s:%zu:%zu\n", error->range.file, start.line, start.col);
            }
            vm_io_buffer_format(buf, "error: %s\n", error->msg);
        }
        error = error->child;
    }
    const char *ret = buf->buf;
    vm_free(buf);
    return ret;
}

void vm_error_report(vm_error_t *error, FILE *out) {
    const char *str = vm_error_report_to_string(error);
    fputs(str, out);
    vm_free(str);
}
