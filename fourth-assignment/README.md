# Quarto Assignment — Loop Fusion

Pass LLVM che implementa la **Loop Fusion**.

Nome pipeline: `**loop-fusion-opt`** (non `loop-fusion`: conflitto col passo ufficiale LLVM).

## Struttura file

```
fourth-assignment/
├── loop-fusion.cpp   # Pass + commenti con comandi compile/test
├── commands.txt      # Comandi manuali di riferimento
└── tests/
    ├── test.c
    ├── test.ll                          # generato
    └── loop-fusion.test.optimized.ll    # generato
```

## Compilare e testare

Tutti i comandi sono in cima a `loop-fusion.cpp` e in [commands.txt](commands.txt).

```bash
cd fourth-assignment

# compila il plugin
clang++ -std=c++17 -fPIC -shared -o loop-fusion.dylib loop-fusion.cpp \
  $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags --system-libs --libs core passes)

# genera IR e applica il pass
clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm tests/test.c -o tests/test.ll
opt -S -load-pass-plugin=./loop-fusion.dylib \
  -passes="mem2reg,loop-simplify,loop-fusion-opt,simplifycfg" tests/test.ll \
  -o tests/loop-fusion.test.optimized.ll
```

Su codice C con `-O0` servono `**mem2reg**` (SSA), `**loop-simplify**` (forma canonica) e opzionalmente `**simplifycfg**` dopo il pass per rimuovere lo scheletro morto del secondo loop.

## Algoritmo

Due loop consecutivi `L0` e `L1` possono fondersi solo se:


| #   | Condizione                  | Analisi LLVM                                            |
| --- | --------------------------- | ------------------------------------------------------- |
| 1   | Adiacenza                   | `LoopInfo` — nessun codice tra exit di L0 e entry di L1 |
| 2   | Stesso trip count           | `ScalarEvolution::getBackedgeTakenCount`                |
| 3   | Control flow equivalent     | `DominatorTree` + `PostDominatorTree`                   |
| 4   | Nessuna dipendenza negativa | `DependenceInfo` + `ScalarEvolution`                    |


### Trasformazione

1. Sostituire gli usi della variabile di induzione di L1 con quella di L0
2. Spostare le istruzioni del corpo di L1 nel corpo di L0 e aggiornare il CFG (L0 esce dove usciva L1)

### Semplificazioni

Gestiamo solo:

- loop **innermost** e **fratelli** (stesso parent)
- loop in **loop-simplify form**
- loop **semplici** (≤ 3 blocchi, un solo PHI nell'header)

## Come verificare l'output

Apri `tests/loop-fusion.test.optimized.ll` e controlla funzione per funzione:


| Funzione                    | Atteso                   |
| --------------------------- | ------------------------ |
| `test_simple_fuse`          | Un solo loop, corpo fuso |
| `test_positive_dependence`  | Fuso                     |
| `test_negative_dependence`  | Due loop separati        |
| `test_different_trip_count` | Due loop separati        |
| `test_not_adjacent`         | Due loop separati        |
| `test_write_after_write`    | Due loop separati        |
| `test_constant_access`      | Fuso                     |
| `test_while_fuse`           | Fuso (due `while`)       |
| `test_dowhile_no_fuse`      | Due loop separati (`do-while`, dip. negativa) |




