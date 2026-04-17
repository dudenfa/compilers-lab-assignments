# Algebraic Identity

### Commands

```bash
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm test.c -o test.ll

clang++ -std=c++17 -fPIC -shared -o algebraic-identity.dylib algebraic-identity.cpp \
  $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags --system-libs --libs core passes)

opt -S -load-pass-plugin=./algebraic-identity.dylib -passes="alg-id,dce" \           
  ./test.ll -o ./alg-id.test.optimized.dce.ll
```

### Files

`algebraic-identity.cpp`
`/tests/algrebraic-identity/test.c`
`/tests/algrebraic-identity/test.ll`
`/tests/algrebraic-identity/alg-id.test.optimized.dce.ll`


# Strength Reduction

### Commands

```bash
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm test.c -o test.ll


```

### Files

`strength-reduction.cpp`