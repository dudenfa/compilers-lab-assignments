// Test per loop-fusion
// clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm test.c -o test.ll
// opt -load-pass-plugin=../loop-fusion.dylib -passes="mem2reg,loop-simplify,loop-fusion-opt,simplifycfg" test.ll -o test.optimized.ll

#include <stddef.h>

// FONDE: due loop consecutivi, stesso trip count, nessuna dipendenza negativa
void test_simple_fuse(int *restrict A, int *restrict B, size_t N) {
  for (size_t i = 0; i < N; i++)
    A[i] = (int)i;
  for (size_t i = 0; i < N; i++)
    B[i] = A[i];
}

// FONDE: dipendenza positiva (L1 legge A[i-1], scritto in iterazione precedente)
void test_positive_dependence(int *restrict A, int b, size_t N) {
  for (size_t i = 0; i < N; i++)
    A[i] = (int)i;
  for (size_t i = 0; i < N; i++)
    b = A[i - 1];
}

// NO: dipendenza negativa (L1 usa A[i+1], scritto in iterazione futura di L0)
void test_negative_dependence(int *restrict A, int b, size_t N) {
  for (size_t i = 0; i < N; i++)
    A[i] = (int)i;
  for (size_t i = 0; i < N; i++)
    b = A[i + 1];
}

// NO: trip count diverso
void test_different_trip_count(int a, int b, size_t N, size_t M) {
  for (size_t i = 0; i < N; i++)
    a = 1;
  for (size_t i = 0; i < M; i++)
    b = 2;
}

// NO: codice tra i due loop (non adiacenti)
void test_not_adjacent(int a, int b, int c, size_t N) {
  for (size_t i = 0; i < N; i++)
    a = 1;
  c = a + b;
  for (size_t i = 0; i < N; i++)
    b = 2;
}

// NO: store-store su stesso array con offset negativo
void test_write_after_write(int *restrict A, int c, size_t N) {
  for (size_t i = 0; i < N; i++)
    A[i] = 1;
  for (size_t i = 0; i < N; i++)
    A[i + 1] = c;
}

// NO: load-store con dipendenza negativa
void test_write_after_read(int *restrict A, int b, int c, size_t N) {
  for (size_t i = 0; i < N; i++)
    b = A[i];
  for (size_t i = 0; i < N; i++)
    A[i + 1] = c;
}

// FONDE: accessi a indici costanti su array diverso (nessuna dipendenza)
void test_constant_access(int *restrict A, int b, size_t N) {
  for (size_t i = 0; i < N; i++)
    A[0] = 1;
  for (size_t i = 0; i < N; i++)
    b = A[1];
}

// NO: accesso misto costante + ricorrente (A[0] vs A[i])
void test_mixed_access(int *restrict A, int b, size_t N) {
  for (size_t i = 0; i < N; i++)
    A[0] = 1;
  for (size_t i = 0; i < N; i++)
    b = A[i];
}

// FONDE: due while consecutivi (stessa struttura IR di un for semplice)
void test_while_fuse(int *restrict A, int *restrict B, size_t N) {
  size_t i = 0;
  while (i < N) {
    A[i] = (int)i;
    i++;
  }
  i = 0;
  while (i < N) {
    B[i] = A[i];
    i++;
  }
}

// NO: do-while con dipendenza negativa (A[i+1] non ancora scritto)
void test_dowhile_no_fuse(int *restrict A, int b, size_t N) {
  size_t i = 0;
  do {
    A[i] = (int)i;
    i++;
  } while (i < N);
  i = 0;
  do {
    b = A[i + 1];
    i++;
  } while (i < N);
}
