
static inline bool vm_obj_is_nil(vm_obj_t o) {
    return nanbox_is_empty(o);
}
static inline bool vm_obj_is_boolean(vm_obj_t o) {
    return nanbox_is_boolean(o);
}
static inline bool vm_obj_is_number(vm_obj_t o) {
    return nanbox_is_number(o);
}
static inline bool vm_obj_is_string(vm_obj_t o) {
    return nanbox_is_aux1(o);
}
static inline bool vm_obj_is_table(vm_obj_t o) {
    return nanbox_is_aux2(o);
}
static inline bool vm_obj_is_closure(vm_obj_t o) {
    return nanbox_is_aux3(o);
}
static inline bool vm_obj_is_ffi(vm_obj_t o) {
    return nanbox_is_aux4(o);
}
static inline bool vm_obj_is_block(vm_obj_t o) {
    return nanbox_is_aux5(o);
}
static inline bool vm_obj_is_error(vm_obj_t o) {
    return nanbox_is_pointer(o);
}

static inline bool vm_obj_get_boolean(vm_obj_t o) {
    assert(vm_obj_is_boolean(o));
    return nanbox_to_boolean(o);
}
static inline double vm_obj_get_number(vm_obj_t o) {
    assert(vm_obj_is_number(o));
    return nanbox_to_double(o);
}
static inline vm_io_buffer_t *vm_obj_get_string(vm_obj_t o) {
    assert(vm_obj_is_string(o));
    return ((vm_io_buffer_t *) nanbox_to_aux(o));
}
static inline vm_obj_table_t *vm_obj_get_table(vm_obj_t o) {
    assert(vm_obj_is_table(o));
    return ((vm_obj_table_t *) nanbox_to_aux(o));
}
static inline vm_obj_closure_t *vm_obj_get_closure(vm_obj_t o) {
    assert(vm_obj_is_closure(o));
    return ((vm_obj_closure_t *) nanbox_to_aux(o));
}
static inline vm_ffi_t *vm_obj_get_ffi(vm_obj_t o) {
    assert(vm_obj_is_ffi(o));
    return ((vm_ffi_t *) nanbox_to_aux(o));
}
static inline vm_ir_block_t *vm_obj_get_block(vm_obj_t o) {
    assert(vm_obj_is_block(o));
    return ((vm_ir_block_t *) nanbox_to_aux(o));
}
static inline vm_error_t *vm_obj_get_error(vm_obj_t o) {
    assert(vm_obj_is_error(o));
    return ((vm_error_t *) nanbox_to_pointer(o));
}

static inline vm_obj_t vm_obj_of_nil(void) {
    return nanbox_empty(); 
}
static inline vm_obj_t vm_obj_of_boolean(bool b) {
    return nanbox_from_boolean(b);
} 
static inline vm_obj_t vm_obj_of_number(double n) {
    return nanbox_from_double(n);
}
static inline vm_obj_t vm_obj_of_buffer(vm_io_buffer_t *o) {
    return nanbox_from_aux1(o);
}
static inline vm_obj_t vm_obj_of_table(vm_obj_table_t *o) {
    return nanbox_from_aux2(o);
}
static inline vm_obj_t vm_obj_of_closure(vm_obj_closure_t *o) {
    return nanbox_from_aux3(o);
}
static inline vm_obj_t vm_obj_of_ffi(vm_ffi_t *o) {
    return nanbox_from_aux4(o);
}
static inline vm_obj_t vm_obj_of_block(vm_ir_block_t *o) {
    return nanbox_from_aux5(o);
}
static inline vm_obj_t vm_obj_of_error(vm_error_t *o) {
    return nanbox_from_pointer(o);
}
