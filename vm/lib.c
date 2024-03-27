
#include "lib.h"

#if defined(__TINYC__)
void *end;
#endif

#if defined(_WIN32)
void vm_hack_chkstk(void) {}
#endif
