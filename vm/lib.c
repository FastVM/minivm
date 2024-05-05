
#include "lib.h"

#if defined(__TINYC__)
void *end;
#endif

#if defined(_WIN32)
void __chkstk(void) {}
#endif
