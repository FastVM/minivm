
#include <stdint.h>

uint8_t vm_peek_u8(uint8_t *p) {
    return *p;
}

void vm_poke_u8(uint8_t *p, uint8_t val) {
    *p = val;
}
