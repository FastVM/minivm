
#pragma once

// MiniVM needs these three at all times
typedef __SIZE_TYPE__ size_t;

typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;

typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;

typedef int32_t vm_file_opcode_t;
typedef int32_t vm_opcode_t;
typedef int64_t vm_number_t;
typedef size_t vm_counter_t;

/// The value type of minivm
union vm_obj_t;
/// These represent constant sized mutable arrays
struct vm_gc_entry_t;

typedef union vm_obj_t vm_obj_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

// I define libc things myself, this massivly speeds up compilation
struct FILE;
typedef struct FILE FILE;

void exit(int code);
size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);

void *malloc(size_t size);
void *realloc(void *ptr, size_t n);
void free(void *ptr);

int printf(const char *src, ...);
FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
