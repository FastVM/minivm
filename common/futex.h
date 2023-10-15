#pragma once

#include <stdint.h>

#ifdef __APPLE__
typedef _Atomic int32_t Futex;
#else
typedef _Atomic int64_t Futex;
#endif

void futex_inc(Futex* f);
void futex_dec(Futex* f);
void futex_signal(Futex* f);
void futex_broadcast(Futex* f);
void futex_wait(Futex* f, Futex val); // leaves if *f != val
void futex_wait_eq(Futex* f, Futex val); // leaves if *f == val
