#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int main() {
    int32_t count = 1;
    int32_t max = 1000000;
    int32_t pprime = 3;
    while (pprime < max) {
        int32_t check = 3;
        int32_t isprime = 1;
        while (check * check <= pprime) {
            if (pprime % check == 0) {
                isprime = 0;
            }
            check += 2;
        }
        count += isprime;
        pprime += 2;
    }
    printf("%"PRIi32"\n", count);
}
