# Terzo Assignment — Loop-Invariant Code Motion

Pass LLVM che implementa la **Loop-Invariant Code Motion** come descritta nelle slide del corso (tre passi: trova invarianti → verifica condizioni → sposta nel preheader).

Nome pipeline: `**loop-inv-motion`** (non `LICM`, per evitare conflitti col passo ufficiale LLVM).

## Struttura file

```
third-assignment/
├── loop-invariant-motion.cpp   # Pass + commenti con comandi compile/test
├── commands.txt                # Comandi manuali di riferimento
└── tests/
    ├── test.c
    ├── test.ll                 # generato
    ├── test.mem2reg.ll         # generato (opzionale, per confronto)
    └── loop-inv-motion.test.optimized.ll
```

## Compilare e testare

Tutti i comandi sono in cima a `loop-invariant-motion.cpp` e in [commands.txt](commands.txt).

```bash
cd third-assignment

# compila il plugin
clang++ -std=c++17 -fPIC -shared -o loop-invariant-motion.dylib loop-invariant-motion.cpp \
  $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags --system-libs --libs core passes)

# genera IR e applica il pass
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm tests/test.c -o tests/test.ll
opt -S -load-pass-plugin=./loop-invariant-motion.dylib \
  -passes="mem2reg,loop-inv-motion" tests/test.ll \
  -o tests/loop-inv-motion.test.optimized.ll
```

Su codice C con `-O0` serve `**mem2reg**` prima del pass (promuove le variabili stack in SSA).

## Algoritmo

### 1. Trova le istruzioni loop-invariant

`A = B + C` è invariant se `B` e `C` non cambiano nel loop. Scorriamo i blocchi in ordine **RPO**.

**Non consideriamo:** PHI, terminatori, load, store, side effect.

### 2. Verifica se si possono spostare

1. Domina tutte le uscite del loop, **oppure** non è usata fuori dal loop
2. Domina tutti gli usi interni al loop
3. Le **divisioni** solo se (1) è vera

### 3. Sposta nel preheader

`I->moveBefore(preheader->getTerminator())`, rispettando l'ordine delle dipendenze.

## Come verificare l'output

Apri `tests/loop-inv-motion.test.optimized.ll` e controlla funzione per funzione:


| Funzione                | Cosa cercare                         |
| ----------------------- | ------------------------------------ |
| `test_basic_hoist`      | `mul` nel preheader, non nel corpo   |
| `test_invariant_chain`  | `add` e `mul` nel preheader          |
| `test_safe_vs_unsafe`   | `mul` fuori, `sdiv` nel loop         |
| `test_dominance_needed` | `sdiv` nel preheader (do-while)      |
| `test_memory_no_hoist`  | `load`/`store` nel loop              |
| `test_nested_hoist`     | `add` nel preheader del loop esterno |




