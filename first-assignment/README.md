# Primo Assignment — LLVM Optimization Passes

Implementazione di tre pass LLVM (New Pass Manager, LLVM 19+) che applicano ottimizzazioni locali su IR.

## Struttura file

```
first-assignment/
├── algebraic-identity.cpp      # Pass 1: identità algebriche (x+0, x*1, ...)
├── strength-reduction.cpp      # Pass 2: strength reduction (15*x, x/8, ...)
├── multi-inst-opt.cpp          # Pass 3: ottimizzazione multi-istruzione (b+1, a-1 => b)
├── run-tests.sh                # Script per build + test di tutti i pass
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

Oltre agli esempi richiesti dal prof (`15*x`, `x/8`), il pass generalizza le ottimizzazioni per costanti del tipo **potenza di 2** e **potenza di 2 ± 1**.

**Moltiplicazione (`MUL`):**

| Pattern | Esempio | Trasformazione |
| --- | --- | --- |
| `x * 2^n` | `x * 8` | `x << 3` |
| `x * (2^n + 1)` | `x * 9` | `(x << 3) + x` |
| `x * (2^n - 1)` | `x * 15` | `(x << 4) - x` |

La costante può stare a sinistra o a destra (`15 * x` e `x * 15` sono equivalenti).

**Divisione (`UDIV` / `SDIV`):**

| Pattern | Esempio | Trasformazione |
| --- | --- | --- |
| `x / 2^n` (unsigned) | `x / 4` | `x >> n` (`lshr`) |
| `x / 2^n` (signed) | `x / 8` | `x >> n` (`ashr`) |

Per la divisione unsigned gestiamo anche alcuni divisori `2^n ± 1`; per la divisione signed ottimizziamo solo potenze di 2 esatte.

**Non ottimizziamo:** costanti negative, divisori non potenza di 2 (es. `x * 6`, `x / 3`), e altri casi fuori da questi pattern.

### Multi-Instruction Optimization — casi coperti

Il pass elimina coppie di istruzioni consecutive che si annullano a vicenda, sostituendo il risultato finale con l'operando originale.

**Addizione / sottrazione:**

| Pattern | Esempio | Risultato |
| --- | --- | --- |
| `(x + C) - C` | `(b + 1) - 1` | `x` / `b` |
| `(C + x) - C` | `(1 + b) - 1` | `x` / `b` |
| `(x - C) + C` | `(b - 1) + 1` | `x` / `b` |

**Moltiplicazione / divisione:**

| Pattern | Esempio | Risultato | Note |
| --- | --- | --- | --- |
| `(x * C) / C` | `(x * 4) / 4` | `x` | `sdiv` e `udiv` |
| `(C * x) / C` | `(4 * x) / 4` | `x` | commutatività di `mul` |
| `(x / C) * C` | `(x / 8) * 8` | `x` | solo se la divisione in IR ha flag `exact` |

Il flag `exact` indica che la divisione non perde informazione. **Clang da C `-O0` non lo genera**, quindi nei nostri test dimostriamo i casi negativi `(x/3)*3` e `(x/8)*8` che **non** vanno ottimizzati — comportamento corretto.

**Non ottimizziamo:**
- costanti diverse (`(x + 5) - 4`)
- costante come primo operando in sub/div non invertibile (`10 - x`, `C / x`)
- `(x / C) * C` quando la divisione perde informazione (es. `7/3*3 = 6`)

Su codice C con `-O0`, usare **`mem2reg`** prima del pass per promuovere le variabili stack in SSA.


## Eseguire tutti i test

Dalla cartella `first-assignment/`:

```bash
chmod +x run-tests.sh
./run-tests.sh
```

Lo script compila i tre plugin (`.dylib`), genera l'IR dai file `.c`, applica i pass e verifica l'output **per funzione** (casi positivi e negativi).

### Cosa verificano i test


| Pass               | Casi positivi                         | Casi negativi (non devono essere ottimizzati) |
| ------------------ | ------------------------------------- | --------------------------------------------- |
| Algebraic Identity | `a±0`, `0+a`, `×1`, `/1`, nested      | `0-a`, `a*b`, `a*0`, `0/a`, `1/a`             |
| Strength Reduction | `×8/9/7/15`, `/4` (udiv), `/8` (sdiv) | `×6`, `/6`, `×-8`, `/-4`, `/3`                |
| Multi-Inst Opt     | `(x±k)∓k`, `(x*C)/C`, caso prof | costanti diverse, ordine sbagliato, `(x/3)*3`, `(x/8)*8` |


Ispirati alla struttura di [AY02/compilers-lab-assignments](https://github.com/AY02/compilers-lab-assignments/tree/main/1_assignment), adattati alle ottimizzazioni effettivamente implementate nei nostri pass.

## Comandi manuali

### 1. Algebraic Identity

```bash
# Compila il plugin
clang++ -std=c++17 -fPIC -shared -o algebraic-identity.dylib algebraic-identity.cpp \
  $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags --system-libs --libs core passes)

# Genera IR da C (-O0, senza optnone)
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
  tests/algebraic-identity/test.c -o tests/algebraic-identity/test.ll

# Applica il pass (+ dce per rimuovere istruzioni morte)
/opt/homebrew/opt/llvm@19/bin/opt -S \
  -load-pass-plugin=./algebraic-identity.dylib \
  -passes="alg-id,dce" \
  tests/algebraic-identity/test.ll \
  -o tests/algebraic-identity/alg-id.test.optimized.dce.ll
```

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

### 3. Multi-Instruction Optimization

Su codice C generato con `-O0`, clang usa variabili su stack (`alloca` + `load`/`store`).
Il pass riconosce pattern come `%a = add %b, 1; %c = sub %a, 1` solo dopo **promozione in SSA** con `mem2reg`.

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

## Note

- I file `.dylib` generati dalla compilazione non vanno committati (sono artefatti locali).
- `dce` (Dead Code Elimination) è usato dopo i pass per pulire istruzioni sostituite ma non ancora rimosse.
- Su Linux sostituire l'estensione `.dylib` con `.so` e adattare `LLVM_BIN` se necessario.

