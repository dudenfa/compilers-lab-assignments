// Test per Multi Instruction Optimization Pass
// Tutti i casi partono da codice C reale (-O0 + mem2reg nel pipeline di test).

// ---------------------------------------------------------------------------
// CASO DEL PROF
//   a = b + 1
//   c = a - 1
//   => c = b
// ---------------------------------------------------------------------------
int test_assignment(int b) {
    int a = b + 1;
    int c = a - 1;
    return c;
}

// ---------------------------------------------------------------------------
// ADD / SUB — operazioni inverse consecutive
// ---------------------------------------------------------------------------

// (x + k) - k = x
int test_add_sub(int x) {
    int tmp = x + 5;
    return tmp - 5;
}

// (k + x) - k = x   (addizione commutativa)
int test_add_sub_comm(int x) {
    int tmp = 5 + x;
    return tmp - 5;
}

// (x - k) + k = x
int test_sub_add(int x) {
    int tmp = x - 10;
    return tmp + 10;
}

// ---------------------------------------------------------------------------
// MUL / DIV — moltiplica poi dividi (caso sicuro)
//   (x * k) / k = x
// ---------------------------------------------------------------------------

// signed
int test_mul_sdiv(int x) {
    int tmp = x * 4;
    return tmp / 4;
}

// signed, costante a sinistra
int test_mul_sdiv_comm(int x) {
    int tmp = 4 * x;
    return tmp / 4;
}

// unsigned
unsigned test_mul_udiv(unsigned x) {
    unsigned tmp = x * 8;
    return tmp / 8;
}

// ---------------------------------------------------------------------------
// CASI NEGATIVI — il pass NON deve ottimizzare
// ---------------------------------------------------------------------------

// costanti diverse: (x + 5) - 4 != x
int test_wrong_constants(int x) {
    int tmp = x + 5;
    return tmp - 4;
}

// ordine non invertibile: (10 - x) + 10 != x
int test_sub_wrong_order(int x) {
    int tmp = 10 - x;
    return tmp + 10;
}

// dividi poi moltiplica: (x / 3) * 3 != x in generale
// Esempio: x = 7  =>  7 / 3 = 2,  2 * 3 = 6  (non 7)
// Il pass non ottimizza perche la divisione intera perde informazione.
int test_div_mul_no_opt(int x) {
    int tmp = x / 3;
    return tmp * 3;
}

// stesso problema anche con potenze di 2: (x / 8) * 8 != x
// Esempio: x = 10  =>  10 / 8 = 1,  1 * 8 = 8  (non 10)
int test_div_mul_power2_no_opt(int x) {
    int tmp = x / 8;
    return tmp * 8;
}
