# Compilers Lab Assignments

Repository per i laboratori del corso di **Compilatori**: pass LLVM, analisi di dataflow e materiali di supporto.

## Assignments


| Assignment                       | Cartella                                   | Contenuto                                                                             | Stato      |
| -------------------------------- | ------------------------------------------ | ------------------------------------------------------------------------------------- | ---------- |
| **1** — LLVM Optimization Passes | `[first-assignment/](first-assignment/)`   | Tre pass LLVM: algebraic identity, strength reduction, multi-instruction optimization | Completato |
| **2** — Dataflow Analysis        | `[second-assignment/](second-assignment/)` | Appunti su dominator analysis, very busy expressions, constant propagation            | In corso   |


Per dettagli, comandi e struttura file di ogni assignment, consultare il README nella rispettiva cartella.

## Primo Assignment

```bash
cd first-assignment
chmod +x run-tests.sh
./run-tests.sh
```

Lo script compila i plugin, genera l'IR dai test e verifica che le ottimizzazioni siano applicate correttamente.

Documentazione completa: `[first-assignment/README.md](first-assignment/README.md)`

## Struttura repository

```
compilers-lab-assignments/
├── README.md                 # Questo file
├── first-assignment/         # Pass LLVM (assignment 1)
│   ├── README.md
│   ├── *.cpp                 # Implementazione pass
│   ├── run-tests.sh
│   └── tests/                # Test per ogni pass
└── second-assignment/        # Appunti dataflow (assignment 2)
    ├── dominator-analysis.md
    ├── very-busy-expressions.md
    └── constant-propagation.md
```

## Convenzioni

- Ogni assignment ha la propria cartella con README dedicato.
- I pass LLVM usano il **New Pass Manager** e si registrano come plugin dinamici (`-load-pass-plugin`).
- I test vivono in `tests/<nome-pass>/` con sorgente C, IR generato e IR ottimizzato di riferimento.
- Gli artefatti di build (`.dylib`, `.so`) non vanno committati.

