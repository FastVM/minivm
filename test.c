typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;
typedef __SIZE_TYPE__ size_t;
typedef struct {
  void * v32;
} v30_ret_t;
typedef v30_ret_t(*v30_t)(void *, void *);
typedef struct {
  void * v51;
  void * v52;
} v49_ret_t;
typedef v49_ret_t(*v49_t)(void *);
typedef struct {
  void * v55;
  uint64_t v56;
  uint32_t v57;
} v53_ret_t;
typedef v53_ret_t(*v53_t)();
typedef struct {
  uint64_t v0;
  uint32_t v1;
} ret_t;

ret_t entry(uint64_t v4) {
  uint8_t v12d[0x8];
  void * v12;
  uint8_t v17d[0x20];
  void * v17;
  void * v21;
  void * v23;
  void * v25;
  uint64_t v27;
  void * v28;
    void * v32;
  void * v33;
  uint32_t v34;
  char v41;
  uint64_t v35;
  void * v46;
  void * v47;
    void * v51;
    void * v52;
    void * v55;
    uint64_t v56;
    uint32_t v57;
  uint32_t v9;
  uint32_t v6a0;
  uint64_t v8;
  uint64_t v6a1;

bb1:;
  v12 = (void *) &v12d[0];
  *(uint64_t*) v12 = v4;
  goto bb15;

bb15:;
  v17 = (void *) &v17d[0];
  *(uint64_t*) v17 = (uint64_t)2llu;
  v21 = (void*) ((size_t) v17 + 8);
  *(uint32_t*) v21 = (uint32_t)6llu;
  v23 = (void*) ((size_t) v17 + 16);
  v25 = (void*) ((size_t) v23 + 8);
  *(uint32_t*) v25 = (uint32_t)0llu;
  v27 = *(uint64_t*) v12;
  if (1) {
    union {uint64_t src; void * dest;} tmp;
    tmp.src = v27;
    v28 = tmp.dest;
  }
  {
    v30_ret_t ret = ((v30_t) v28)((void *)0x5555556cdfa0llu, v17);
    v32 = ret.v32;
  }
  v33 = (void*) ((size_t) v17 + 8);
  v34 = *(uint32_t*) v33;
  v41 = v34 == (uint32_t)14llu;
  if (v41 !== 0) {
  goto bb43;
  } else {
  goto bb44;
  }


bb43:;
  goto bb36;

bb36:;
  v35 = *(uint64_t*) v17;
  v6a0 = v34;
  v6a1 = v35;
  goto bb6;

bb44:;
  goto bb38;

bb38:;
  v46 = (void*) ((size_t) (void *)0x5555556c7940llu + v34 * 8);
  v47 = *(void **) v46;
  {
    v49_ret_t ret = ((v49_t) /* vm_tb_rfunc_comp */(void *) 0x55555556f4f0)(v47);
    v51 = ret.v51;
    v52 = ret.v52;
  }
  {
    v53_ret_t ret = ((v53_t) v52)();
    v55 = ret.v55;
    v56 = ret.v56;
    v57 = ret.v57;
  }
  v6a0 = v57;
  v6a1 = v56;
  goto bb6;

bb6:;
  v9 = v6a0;
  v8 = v6a1;
  {
    ret_t ret;
    ret.v0 = v8;
    ret.v1 = v9;
    return ret;
  }
}
