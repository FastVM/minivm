
typedef __UINT8_TYPE__ u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
typedef __UINT64_TYPE__ u64;

extern "C" int putchar(int c);

template<class R1, class R2, class R3, class R4, class R5, class R6, class R7>
auto f7(R1 r1, R2 r2, R3 r3, R4 r4, R5 r5, R6 r6, R7 r7) -> int {
    putchar(97);
    return 1;
}

template<class R1, class R2, class R3, class R4, class R5, class R6>
auto f6(R1 r1, R2 r2, R3 r3, R4 r4, R5 r5, R6 r6) -> int {
    putchar(98);
    f7<u8, R1, R2, R3, R4, R5, R6>(8, r1, r2, r3, r4, r5, r6);
    f7<u16, R1, R2, R3, R4, R5, R6>(16, r1, r2, r3, r4, r5, r6);
    f7<u32, R1, R2, R3, R4, R5, R6>(32, r1, r2, r3, r4, r5, r6);
    f7<u64, R1, R2, R3, R4, R5, R6>(64, r1, r2, r3, r4, r5, r6);
    return 5;
}

template<class R1, class R2, class R3, class R4, class R5>
auto f5(R1 r1, R2 r2, R3 r3, R4 r4, R5 r5) -> int {
    putchar(99);
    f6<u8, R1, R2, R3, R4, R5>(8, r1, r2, r3, r4, r5);
    f6<u16, R1, R2, R3, R4, R5>(16, r1, r2, r3, r4, r5);
    f6<u32, R1, R2, R3, R4, R5>(32, r1, r2, r3, r4, r5);
    f6<u64, R1, R2, R3, R4, R5>(64, r1, r2, r3, r4, r5);
    return 24;
}

template<class R1, class R2, class R3, class R4>
auto f4(R1 r1, R2 r2, R3 r3, R4 r4) -> int {
    putchar(100);
    f5<u8, R1, R2, R3, R4>(8, r1, r2, r3, r4);
    f5<u16, R1, R2, R3, R4>(16, r1, r2, r3, r4);
    f5<u32, R1, R2, R3, R4>(32, r1, r2, r3, r4);
    f5<u64, R1, R2, R3, R4>(64, r1, r2, r3, r4);
    return 100;
}

template<class R1, class R2, class R3>
auto f3(R1 r1, R2 r2, R3 r3) -> int {
    putchar(101);
    f4<u8, R1, R2, R3>(8, r1, r2, r3);
    f4<u16, R1, R2, R3>(16, r1, r2, r3);
    f4<u32, R1, R2, R3>(32, r1, r2, r3);
    f4<u64, R1, R2, R3>(64, r1, r2, r3);
    return 404;
}

template<class R1, class R2>
auto f2(R1 r1, R2 r2) -> int {
    putchar(102);
    f3<u8, R1, R2>(8, r1, r2);
    f3<u16, R1, R2>(16, r1, r2);
    f3<u32, R1, R2>(32, r1, r2);
    f3<u64, R1, R2>(64, r1, r2);
    return 1620;
}

template<class R1>
auto f1(R1 r1) -> int {
    putchar(103);
    f2<u8, R1>(8, r1);
    f2<u16, R1>(16, r1);
    f2<u32, R1>(32, r1);
    f2<u64, R1>(64, r1);
    return 6484;
}

auto main() -> int {
    putchar(104);
    f1<u8>(8);
    f1<u16>(16);
    f1<u32>(32);
    f1<u64>(64);
    return 0;
}
