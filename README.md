# Compilers Lab Assignments

Repository per i laboratori del corso **Compilatori — Middle end** (Prof. Andrea Marongiu, a.a. 2025/2026).

## Assignments


| #   | Titolo                   | Cartella                                 | Tipo                  | Stato      |
| --- | ------------------------ | ---------------------------------------- | --------------------- | ---------- |
| 1   | LLVM Optimization Passes | [first-assignment/](first-assignment/)   | Pass LLVM (C++)       | Completato |
| 2   | Dataflow Analysis        | [second-assignment/](second-assignment/) | Analisi teorica (PDF) | Completato |


---

## Primo Assignment — Pass LLVM

Tre pass per il **New Pass Manager** (LLVM 19+) che applicano ottimizzazioni locali su IR:

- **Algebraic Identity** (`alg-id`)
- **Strength Reduction** (`strength-reduction`)
- **Multi-Instruction Optimization** (`multi-inst-opt`)

### Eseguire i test

```bash
cd first-assignment
chmod +x run-tests.sh
./run-tests.sh
```

Lo script compila i plugin, genera l'IR dai test e verifica che le ottimizzazioni siano applicate correttamente.

Documentazione: [first-assignment/README.md](first-assignment/README.md)

---

## Secondo Assignment — Dataflow Analysis

Soluzione teorica delle tre analisi richieste dal PDF dell'assignment. Per ciascuna:

1. formalizzazione del framework DFA (dominio, direzione, transfer, meet, boundary, initial points);
2. tabella iterativa sul CFG di esempio fino al fixed point.


| Sezione | Analisi                          |
| ------- | -------------------------------- |
| 1       | Very Busy Expressions (backward) |
| 2       | Dominator Analysis (forward)     |
| 3       | Constant Propagation (forward)   |


**Consegna:** [second-assignment/assignment-02.pdf](second-assignment/assignment-02.pdf)

Documentazione: [second-assignment/README.md](second-assignment/README.md)

---

## Struttura repository

```
compilers-lab-assignments/
├── README.md
├── first-assignment/
│   ├── README.md
│   ├── algebraic-identity.cpp
│   ├── strength-reduction.cpp
│   ├── multi-inst-opt.cpp
│   ├── run-tests.sh
│   ├── commands.txt
│   └── tests/
└── second-assignment/
    ├── README.md
    ├── assignment-02.pdf
    └── images/
```

## Convenzioni

- Ogni assignment ha una cartella dedicata con README.
- Assignment 1: pass LLVM registrati come plugin dinamici (`-load-pass-plugin`); test in `tests/<nome-pass>/`.
- Assignment 2: soluzione in PDF; le immagini dei CFG sono in `images/`.
- Gli artefatti di build (`.dylib`, `.so`) non sono committati.

