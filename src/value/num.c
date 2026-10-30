
#include "num.h"

double mx_num_fmod(double a, double b) {
    double r = fmod(a, b);
    if (r != 0 && (r < 0) != (b < 0)) {
        r += b;
    }
    return r;
}
