
#include <stdint.h>
#include <stdlib.h>

uint8_t *vm_new_u8(void) {
    return malloc(sizeof(uint8_t));
}

uint8_t vm_peek_u8(uint8_t *p) {
    return *p;
}

void vm_poke_u8(uint8_t *p, uint8_t val) {
    *p = val;
}
