// Test per Algebraic Identity Pass

int add(int a, int b) {
    int x1, x2, x3;

    x1 = a + 0;      // should optimize --> a
    x2 = 0 + a;      // should optimize --> a
    x3 = a + b;      // no opt

    return x1 + x2 + x3;
}

int sub(int a, int b) {
    int x1, x2, x3;

    x1 = a - 0;      // should optimize --> a
    x2 = 0 - a;      // no opt (non e' identita')
    x3 = a - b;      // no opt

    return x1 + x2 + x3;
}

int mul(int a, int b) {
    int x1, x2, x3, x4;

    x1 = a * 1;      // should optimize --> a
    x2 = 1 * a;      // should optimize --> a
    x3 = a * 0;      // no opt (non implementato nel nostro pass)
    x4 = a * b;      // no opt

    return x1 + x2 + x3 + x4;
}

int div_test(int a, int b) {
    int x1, x2, x3, x4;

    x1 = a / 1;      // should optimize --> a
    x2 = 0 / a;      // no opt (non implementato nel nostro pass)
    x3 = 1 / a;      // no opt
    x4 = a / b;      // no opt

    return x1 + x2 + x3 + x4;
}

// verifica che ottimizzazioni annidate funzionino in un solo passaggio
int nested(int a) {
    int x = a + 0;
    int y = x + 0;
    int z = y + 0;
    return z;
}
