// Test per loop-inv-motion
// clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm test.c -o test.ll
// opt -load-pass-plugin=../loop-invariant-motion.dylib -passes="mem2reg,loop-inv-motion" test.ll -o test.optimized.ll

// OTTIMIZZA: a * b  (invariante, nel preheader)
// NO: sum += ...   (dipende da i)
int test_basic_hoist(int a, int b, int n) {
  int sum = 0;
  for (int i = 0; i < n; i++) {
    int x = a * b;
    sum += x + i;
  }
  return sum;
}

// OTTIMIZZA: a + b, poi x * 42  (catena invariante)
// NO: sum += ...   (dipende da i)
int test_invariant_chain(int a, int b, int n) {
  int sum = 0;
  for (int i = 0; i < n; i++) {
    int x = a + b;
    int y = x * 42;
    sum += y + i;
  }
  return sum;
}

// OTTIMIZZA: a * b
// NO: a / b        (divisione, for condizionato)
// NO: sum += ...   (dipende da i)
int test_safe_vs_unsafe(int a, int b, int n) {
  int sum = 0;
  for (int i = 0; i < n; i++) {
    int x = a * b;
    int y = a / b;
    sum += x + y + i;
  }
  return sum;
}

// OTTIMIZZA: a / b  (do-while, domina le uscite)
// NO: sum += ...    (dipende da i)
int test_dominance_needed(int a, int b, int n) {
  int sum = 0;
  int i = 0;
  do {
    int x = a / b;
    sum += x + i;
    i++;
  } while (i < n);
  return sum;
}

// NO: load, store  (accesso memoria)
void test_memory_no_hoist(int *ptr, int n) {
  for (int i = 0; i < n; i++) {
    int val = *ptr;
    *ptr = 42;
    (void)val;
  }
}

// OTTIMIZZA: a + b  (prima loop interno, poi loop esterno)
// NO: sum += ...    (dipende da j)
int test_nested_hoist(int a, int b, int n) {
  int sum = 0;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      int x = a + b;
      sum += x + j;
    }
  }
  return sum;
}
