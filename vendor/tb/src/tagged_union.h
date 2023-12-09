// Template for creating these small tagged unions
//
// #define TERMS(x) (SumType, SUM, x(SOME_NAME, some_name, int x, y))
// #include "tagged_union.h"
//
#define NAME      PP_JOIN(PP_ARG0,   TERMS(PP_VOID))
#define BODY(x)   PP_JOIN(PP_AFTER0, TERMS(x))
#define X(enum_n, name, ...) enum_n,
#define Y(enum_n, name, ...) struct { __VA_ARGS__; } name;
struct NAME {
    enum  { BODY(X) } tag;
    union { BODY(Y) };
};
#undef TERMS
#undef BODY
#undef NAME
#undef X
#undef Y
