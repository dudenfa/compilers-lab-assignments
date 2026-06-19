//=============================================================================
// TERZO ASSIGNMENT — Loop-Invariant Code Motion (New Pass Manager, LLVM 19+)
//=============================================================================
//
// COSA FA
//   Sposta fuori dal loop le istruzioni che non cambiano ad ogni iterazione.
//   Il pass si chiama "loop-inv-motion" (NON "LICM": conflitto col passo LLVM).
//
// ANALISI LLVM (come nell'esercitazione sui loop)
//   - LoopInfo       : trova i loop, il preheader, i blocchi del ciclo
//   - DominatorTree  : verifica se un blocco viene eseguito prima di un altro
//
// ALGORITMO — tre passi
//
//   1. Trova le istruzioni loop-invariant
//      Un'istruzione A = B + C e' invariant se B e C sono:
//        - definiti fuori dal loop, oppure
//        - costanti / argomenti, oppure
//        - definiti da istruzioni gia' marcate invarianti.
//      Scorriamo i blocchi in ordine RPO (definizioni prima degli usi).
//
//   2. Verifica se si possono spostare (code motion)
//      Condizioni (dalla slide):
//        a) il blocco domina tutte le uscite del loop, OPPURE
//           la variabile non e' usata fuori dal loop ("morta" all'uscita);
//        b) il blocco domina tutti gli usi interni al loop;
//        c) le divisioni si spostano SOLO se (a) e' vera (evita trap con n==0).
//      Escludiamo load, store e istruzioni con side effect.
//
//   3. Sposta nel preheader
//      moveBefore(terminator) — subito prima del salto verso l'header del loop.
//      Spostiamo in ordine, solo se le dipendenze nel loop sono gia' state mosse.
//
// LOOP ANNIDATI
//   Prima i loop interni, poi quelli esterni (post-order sull'albero dei loop).
//
// COME COMPILARE IL PLUGIN
//   cd third-assignment/
//   clang++ -std=c++17 -fPIC -shared -o loop-invariant-motion.dylib \
//     loop-invariant-motion.cpp \
//     $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags \
//       --system-libs --libs core passes)
//
// COME TESTARE MANUALMENTE
//   cd third-assignment/
//
//   # 1) Genera IR grezzo da C (-O0, senza optnone)
//   clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
//     tests/test.c -o tests/test.ll
//
//   # 2) (opzionale) IR solo con mem2reg, per confronto "prima"
//   opt -S -passes=mem2reg tests/test.ll -o tests/test.mem2reg.ll
//
//   # 3) Applica mem2reg + il nostro pass
//   opt -S -load-pass-plugin=./loop-invariant-motion.dylib \
//     -passes="mem2reg,loop-inv-motion" \
//     tests/test.ll \
//     -o tests/loop-inv-motion.test.optimized.ll
//
//   # 4) Confronta i due file: in @test_basic_hoist, "mul" deve stare
//      nel preheader (prima del br verso l'header) e non piu' nel corpo.
//      Vedi anche commands.txt per altri esempi di verifica.
//
// NOME PIPELINE: loop-inv-motion
//=============================================================================

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

//===----------------------------------------------------------------------===//
// Passo 1 — Quali istruzioni sono loop-invariant?
//===----------------------------------------------------------------------===//

/// Un operando e' invariant se e' costante/argomento/globale, definito fuori
/// dal loop, oppure definito da un'istruzione gia' nell'InvariantSet.
static bool isOperandInvariant(Value *V, Loop *L,
                               const SmallSetVector<Instruction *, 16> &InvariantSet) {
  if (isa<Constant>(V) || isa<Argument>(V) || isa<GlobalValue>(V))
    return true;

  if (auto *I = dyn_cast<Instruction>(V)) {
    if (!L->contains(I->getParent()))
      return true;
    return InvariantSet.contains(I);
  }
  return false;
}

/// Istruzione loop-invariant? 
static bool isLoopInvariantInst(Instruction &I, Loop *L,
                                const SmallSetVector<Instruction *, 16> &InvariantSet) {
  if (I.isTerminator())
    return false;

  // I PHI nel loop dipendono dal back-edge: non li consideriamo invarianti.
  if (isa<PHINode>(&I))
    return false;

  // Load/store e printf non si spostano: alterano o leggono la memoria.
  if (I.mayHaveSideEffects() || I.mayReadFromMemory())
    return false;

  for (Value *Op : I.operands()) {
    if (!isOperandInvariant(Op, L, InvariantSet))
      return false;
  }
  return true;
}

/// Raccoglie le invarianti in ordine RPO (definizioni prima degli usi).
static void collectInvariants(Loop *L, LoopInfo &LI,
                              SmallSetVector<Instruction *, 16> &InvariantSet) {
  LoopBlocksRPO Order(L);
  Order.perform(&LI);

  for (BasicBlock *BB : Order) {
    if (LI.getLoopFor(BB) != L)
      continue;
    for (Instruction &I : *BB) {
      if (isLoopInvariantInst(I, L, InvariantSet))
        InvariantSet.insert(&I);
    }
  }
}

//===----------------------------------------------------------------------===//
// Passo 2 — Si puo' spostare l'istruzione?
//===----------------------------------------------------------------------===//

/// Il blocco di I domina tutti i blocchi "exiting" (da cui si esce dal loop)?
static bool dominatesAllExits(Instruction *I, Loop *L, DominatorTree &DT) {
  BasicBlock *BB = I->getParent();
  SmallVector<BasicBlock *, 8> Exiting;
  L->getExitingBlocks(Exiting);

  for (BasicBlock *ExitBB : Exiting) {
    if (!DT.dominates(BB, ExitBB))
      return false;
  }
  return true;
}

/// Tutti gli usi di I sono dentro il loop? (variabile "morta" fuori dal loop)
static bool isDeadOutsideLoop(Instruction *I, Loop *L) {
  for (User *U : I->users()) {
    if (auto *UserI = dyn_cast<Instruction>(U)) {
      if (!L->contains(UserI))
        return false;
    }
  }
  return true;
}

/// I domina tutti gli usi interni al loop? (domina i blocchi che usano la variabile)
static bool dominatesAllUsesInLoop(Instruction *I, Loop *L, DominatorTree &DT) {
  BasicBlock *DefBB = I->getParent();
  for (User *U : I->users()) {
    if (auto *UserI = dyn_cast<Instruction>(U)) {
      if (L->contains(UserI) && !DT.dominates(DefBB, UserI->getParent()))
        return false;
    }
  }
  return true;
}

static bool isDivision(Instruction *I) {
  unsigned Op = I->getOpcode();
  return Op == Instruction::SDiv || Op == Instruction::UDiv;
}

/// Candidata alla code motion?
static bool canMove(Instruction *I, Loop *L, DominatorTree &DT) {
  if (!dominatesAllUsesInLoop(I, L, DT))
    return false;

  bool domExits = dominatesAllExits(I, L, DT);

  // Divisioni: solo se domina le uscite (altrimenti rischiamo trap con n==0).
  if (isDivision(I))
    return domExits;

  // Altre istruzioni: slide — domina uscite OPPURE non usata fuori dal loop.
  return domExits || isDeadOutsideLoop(I, L);
}

/// Le istruzioni da cui dipende I nel loop sono gia' state spostate?
static bool depsMoved(Instruction *I, Loop *L,
                      const SmallSetVector<Instruction *, 16> &Moved) {
  for (Value *Op : I->operands()) {
    if (auto *OpI = dyn_cast<Instruction>(Op)) {
      if (L->contains(OpI) && !Moved.contains(OpI))
        return false;
    }
  }
  return true;
}

//===----------------------------------------------------------------------===//
// Passo 3 — Sposta nel preheader
//===----------------------------------------------------------------------===//

static bool processLoop(Loop *L, LoopInfo &LI, DominatorTree &DT) {
  BasicBlock *Preheader = L->getLoopPreheader();
  if (!Preheader)
    return false;

  SmallSetVector<Instruction *, 16> Invariants;
  collectInvariants(L, LI, Invariants);
  if (Invariants.empty())
    return false;

  Instruction *InsertBefore = Preheader->getTerminator();
  bool Changed = false;
  SmallSetVector<Instruction *, 16> Moved;

  for (Instruction *I : Invariants) {
    if (!canMove(I, L, DT) || !depsMoved(I, L, Moved))
      continue;

    I->moveBefore(InsertBefore);
    Moved.insert(I);
    Changed = true;
  }
  return Changed;
}

/// Post-order sui loop: prima i figli, poi il padre.
static bool processLoopTree(Loop *L, LoopInfo &LI, DominatorTree &DT) {
  bool Changed = false;
  for (Loop *Sub : *L)
    Changed |= processLoopTree(Sub, LI, DT);
  Changed |= processLoop(L, LI, DT);
  return Changed;
}

struct LoopInvariantMotion : PassInfoMixin<LoopInvariantMotion> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
    DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);

    if (LI.empty())
      return PreservedAnalyses::all();

    bool Changed = false;
    for (Loop *Top : LI)
      Changed |= processLoopTree(Top, LI, DT);

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // namespace

static llvm::PassPluginLibraryInfo getLoopInvariantMotionPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "LoopInvariantMotion", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "loop-inv-motion") {
                    FPM.addPass(LoopInvariantMotion());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getLoopInvariantMotionPluginInfo();
}
