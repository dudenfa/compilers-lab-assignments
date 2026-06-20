# Primo Assignment — LLVM Optimization Passes

Implementazione di tre pass LLVM (New Pass Manager, LLVM 19+) che applicano ottimizzazioni locali su IR.

## Struttura file

```
first-assignment/
├── algebraic-identity.cpp      # Pass 1: identità algebriche (x+0, x*1, ...)
├── strength-reduction.cpp      # Pass 2: strength reduction (15*x, x/8, ...)
├── multi-inst-opt.cpp          # Pass 3: ottimizzazione multi-istruzione (b+1, a-1 => b)
├── commands.txt                # Comandi manuali di riferimento
└── tests/
    ├── algebraic-identity/
    │   ├── test.c
    │   ├── test.ll
    │   └── alg-id.test.optimized.dce.ll
    ├── strength-reduction/
    │   ├── test.c
    │   ├── test.ll
    │   └── strength-reduction.test.optimized.dce.ll
    └── multi-inst-opt/
        ├── test.c
        ├── test.ll
        └── multi-inst-opt.test.optimized.dce.ll
```

## Pass implementati


| Pass                  | Nome pipeline        | Ottimizzazioni                                                     |
| --------------------- | -------------------- | ------------------------------------------------------------------ |
| Algebraic Identity    | `alg-id`             | `x+0 → x`, `0+x → x`, `x*1 → x`, `1*x → x` (+ extra: `x-0`, `x/1`) |
| Strength Reduction    | `strength-reduction` | vedi sezione dedicata sotto                                        |
| Multi-Instruction Opt | `multi-inst-opt`     | vedi sezione dedicata sotto                                        |


### Strength Reduction — casi coperti

Il pass generalizza le ottimizzazioni per costanti del tipo **potenza di 2** e **potenza di 2 ± 1**.

**Moltiplicazione (`MUL`):**


| Pattern         | Esempio  | Trasformazione |
| --------------- | -------- | -------------- |
| `x * 2^n`       | `x * 8`  | `x << 3`       |
| `x * (2^n + 1)` | `x * 9`  | `(x << 3) + x` |
| `x * (2^n - 1)` | `x * 15` | `(x << 4) - x` |


La costante può stare a sinistra o a destra (`15 * x` e `x * 15` sono equivalenti).

**Divisione (`UDIV` / `SDIV`):**


| Pattern              | Esempio | Trasformazione    |
| -------------------- | ------- | ----------------- |
| `x / 2^n` (unsigned) | `x / 4` | `x >> n` (`lshr`) |
| `x / 2^n` (signed)   | `x / 8` | `x >> n` (`ashr`) |


Per la divisione unsigned gestiamo anche alcuni divisori `2^n ± 1`; per la divisione signed ottimizziamo solo potenze di 2 esatte.

**Non ottimizziamo:** costanti negative, divisori non potenza di 2 (es. `x * 6`, `x / 3`), e altri casi fuori da questi pattern.

### Multi-Instruction Optimization — casi coperti

Il pass elimina coppie di istruzioni consecutive che si annullano a vicenda, sostituendo il risultato finale con l'operando originale.

**Addizione / sottrazione:**


| Pattern       | Esempio       | Risultato |
| ------------- | ------------- | --------- |
| `(x + C) - C` | `(b + 1) - 1` | `x` / `b` |
| `(C + x) - C` | `(1 + b) - 1` | `x` / `b` |
| `(x - C) + C` | `(b - 1) + 1` | `x` / `b` |


**Moltiplicazione / divisione:**


| Pattern       | Esempio       | Risultato | Note                                       |
| ------------- | ------------- | --------- | ------------------------------------------ |
| `(x * C) / C` | `(x * 4) / 4` | `x`       | `sdiv` e `udiv`                            |
| `(C * x) / C` | `(4 * x) / 4` | `x`       | commutatività di `mul`                     |
| `(x / C) * C` | `(x / 8) * 8` | `x`       | solo se la divisione in IR ha flag `exact` |


#### Perché `(x * C) / C` sì, ma `(x / C) * C` spesso no

**Caso sicuro — moltiplica poi dividi:**

`(x * 4) / 4` si può ottimizzare in `x`, perché moltiplicare e dividere per la stessa costante si annulla.

**Caso pericoloso — dividi poi moltiplica:**

In C la divisione intera **tronca** verso zero. Se ottimizzassimo `(x / C) * C` in `x` senza controlli, cambieremmo il significato del programma.

Esempi concreti (testati in `tests/multi-inst-opt/test.c`):


| Espressione   | Con `x = 7`                      | Con `x = 10`                       |
| ------------- | -------------------------------- | ---------------------------------- |
| `(x / 3) * 3` | `7/3 = 2`, poi `2*3 = **6`** ≠ 7 | —                                  |
| `(x / 8) * 8` | —                                | `10/8 = 1`, poi `1*8 = **8**` ≠ 10 |


Per questo il pass **non ottimizza** `(x / C) * C` quando la divisione perde informazione. Nel codice controlliamo il flag `exact` in LLVM IR: ottimizziamo solo se la divisione è garantita reversibile. Clang da C con `-O0` non genera quel flag, quindi nei test mostriamo i casi negativi sopra — comportamento corretto.

**Non ottimizziamo:**

- costanti diverse (`(x + 5) - 4`)
- costante come primo operando in sub/div non invertibile (`10 - x`, `C / x`)
- `(x / C) * C` quando la divisione perde informazione (es. `7/3*3 = 6`)

Su codice C con `-O0`, usare `**mem2reg`** prima del pass per promuovere le variabili stack in SSA.

## Test manuali

Tutti i comandi sono anche in [commands.txt](commands.txt). Eseguirli dalla cartella `first-assignment/`.

### Flusso comune

1. **Compila** il plugin `.dylib` con `clang++` e `llvm-config`
2. **Genera IR** da `tests/<pass>/test.c` con `clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm`
3. **Applica** il pass con `opt -load-pass-plugin=... -passes="..."` (aggiungi `dce` e, se serve, `mem2reg`)
4. **Verifica** l'output `.ll` aprendo il file o estraendo una singola funzione (vedi sotto)

Per estrarre una funzione dall'IR:

```bash
awk '/^define .+ @nome_funzione\(/,/^}/' tests/algebraic-identity/alg-id.test.optimized.dce.ll
```

Confronto rapido input vs output:

```bash
diff -u tests/algebraic-identity/test.ll tests/algebraic-identity/alg-id.test.optimized.dce.ll
```

### 1. Algebraic Identity

```bash
clang++ -std=c++17 -fPIC -shared -o algebraic-identity.dylib algebraic-identity.cpp \
  $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags --system-libs --libs core passes)

clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
  tests/algebraic-identity/test.c -o tests/algebraic-identity/test.ll

/opt/homebrew/opt/llvm@19/bin/opt -S \
  -load-pass-plugin=./algebraic-identity.dylib \
  -passes="alg-id,dce" \
  tests/algebraic-identity/test.ll \
  -o tests/algebraic-identity/alg-id.test.optimized.dce.ll
```

**Cosa verificare** (funzione per funzione in `alg-id.test.optimized.dce.ll`):


| Funzione    | Deve                            | Non deve                       |
| ----------- | ------------------------------- | ------------------------------ |
| `@add`      | conservare `a+b`                | `add ... 0` / `add 0, ...`     |
| `@sub`      | conservare `0-a` e `a-b`        | `sub ... 0`                    |
| `@mul`      | conservare `a*b`, `a*0`         | `mul ... 1` / `mul 1, ...`     |
| `@div_test` | conservare `0/a`, `1/a`, `a/b`  | `sdiv ... 1` (solo divisore 1) |
| `@nested`   | semplificare add con 0 annidate | —                              |


### 2. Strength Reduction

```bash
clang++ -std=c++17 -fPIC -shared -o strength-reduction.dylib strength-reduction.cpp \
  $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags --system-libs --libs core passes)

clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
  tests/strength-reduction/test.c -o tests/strength-reduction/test.ll

/opt/homebrew/opt/llvm@19/bin/opt -S \
  -load-pass-plugin=./strength-reduction.dylib \
  -passes="strength-reduction,dce" \
  tests/strength-reduction/test.ll \
  -o tests/strength-reduction/strength-reduction.test.optimized.dce.ll
```

**Cosa verificare:**


| Funzione       | Deve                            | Non deve (o ancora presente)     |
| -------------- | ------------------------------- | -------------------------------- |
| `@test_mul`    | `shl`, `add`, `sub` per ×8/9/15 | `mul ... 6` (×6 non ottimizzato) |
| `@test_udiv`   | `lshr` per `/4`                 | `udiv ... 6`                     |
| `@test_sdiv`   | `ashr` per `/8`                 | `sdiv ... 3`                     |
| `@test_no_opt` | `mul ... -8`, `sdiv ... -4`     | — (costanti negative)            |


### 3. Multi-Instruction Optimization

Su codice C con `-O0` serve `**mem2reg`** prima del pass (promuove stack in SSA).

```bash
clang++ -std=c++17 -fPIC -shared -o multi-inst-opt.dylib multi-inst-opt.cpp \
  $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags --system-libs --libs core passes)

clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
  tests/multi-inst-opt/test.c -o tests/multi-inst-opt/test.ll

/opt/homebrew/opt/llvm@19/bin/opt -S \
  -load-pass-plugin=./multi-inst-opt.dylib \
  -passes="mem2reg,multi-inst-opt,dce" \
  tests/multi-inst-opt/test.ll \
  -o tests/multi-inst-opt/multi-inst-opt.test.optimized.dce.ll
```

**Cosa verificare:**


| Funzione                                                                   | Deve                                         | Non deve              |
| -------------------------------------------------------------------------- | -------------------------------------------- | --------------------- |
| `@test_add_sub`, `@test_add_sub_comm`, `@test_sub_add`, `@test_assignment` | `ret i32 %0` (ritorna l'argomento originale) | `sub`/`add` annullati |
| `@test_mul_sdiv`, `@test_mul_sdiv_comm`, `@test_mul_udiv`                  | `ret i32 %0`                                 | `sdiv`/`udiv`         |
| `@test_wrong_constants`                                                    | `sub` ancora presente                        | ottimizzazione        |
| `@test_sub_wrong_order`                                                    | `add` ancora presente                        | ottimizzazione        |
| `@test_div_mul_no_opt`, `@test_div_mul_power2_no_opt`                      | `mul` ancora presente                        | `(x/C)*C` non sicuro  |


## Note

- Comandi copiabili anche in [commands.txt](commands.txt).
- I file `.dylib` generati dalla compilazione non vanno committati (sono artefatti locali).
- `dce` (Dead Code Elimination) è usato dopo i pass per pulire istruzioni sostituite ma non ancora rimosse.
- Su Linux sostituire l'estensione `.dylib` con `.so` e adattare `LLVM_BIN` se necessario.

