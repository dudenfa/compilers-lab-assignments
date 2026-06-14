// Test per Strength Reduction Pass

int test_mul(int a) {
    int x1, x2, x3, x4, x5;

    x1 = a * 8;      // 2^3       --> a << 3
    x2 = a * 9;      // 2^3 + 1   --> (a << 3) + a
    x3 = a * 7;      // 2^3 - 1   --> (a << 3) - a
    x4 = 15 * a;     // 2^4 - 1   --> (a << 4) - a  (commutativity)
    x5 = a * 6;      // no opt

    return x1 + x2 + x3 + x4 + x5;
}

unsigned int test_udiv(unsigned int a) {
    unsigned int x1, x2;

    x1 = a / 4;      // 2^2 --> lshr
    x2 = a / 6;      // no opt (non potenza di 2 ne adiacente nel nostro pass unsigned)

    return x1 + x2;
}

int test_sdiv(int a) {
    int x1, x2;

    x1 = a / 8;      // 2^3 --> ashr (caso del prof)
    x2 = a / 3;      // no opt

    return x1 + x2;
}

int test_no_opt(int a) {
    int x1, x2;

    x1 = a * -8;     // costante negativa --> no opt
    x2 = a / -4;     // divisore negativo --> no opt

    return x1 + x2;
}
