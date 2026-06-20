//=============================================================================
// QUARTO ASSIGNMENT — Loop Fusion (New Pass Manager, LLVM 19+)
//=============================================================================
//
// COSA FA
//   Fonde due loop consecutivi L0 e L1 in un unico loop, se rispettano le
//   quattro condizioni delle slide:
//     1. adiacenza       — nessun codice tra la fine di L0 e l'inizio di L1
//     2. stesso trip count — stesso numero di iterazioni (ScalarEvolution)
//     3. control flow equivalent — L0 domina L1 e L1 post-domina L0
//     4. nessuna dipendenza negativa — niente accessi memoria "all'indietro"
//
// ANALISI LLVM
//   - LoopInfo           : struttura dei loop
//   - DominatorTree      : dominanza
//   - PostDominatorTree  : post-dominanza
//   - ScalarEvolution    : trip count e indirizzi di memoria
//   - DependenceInfo     : dipendenze tra load/store
//
// TRASFORMAZIONE (due passi)
//   1. Sostituire gli usi della IV di L1 con quella di L0 (in SSA sono distinte)
//   2. Spostare il corpo di L1 subito dopo il corpo di L0 e aggiornare il CFG
//
//
// COME COMPILARE IL PLUGIN
//   cd fourth-assignment/
//   clang++ -std=c++17 -fPIC -shared -o loop-fusion.dylib loop-fusion.cpp \
//     $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags \
//       --system-libs --libs core passes)
//
// COME TESTARE MANUALMENTE
//   cd fourth-assignment/
//
//   clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
//     tests/test.c -o tests/test.ll
//
//   opt -S -load-pass-plugin=./loop-fusion.dylib \
//     -passes="mem2reg,loop-simplify,loop-fusion-opt,simplifycfg" \
//     tests/test.ll \
//     -o tests/loop-fusion.test.optimized.ll
//
//
// NOME PIPELINE: loop-fusion-opt
//=============================================================================

#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

//===----------------------------------------------------------------------===//
// Condizione 1 — Adiacenza
//===----------------------------------------------------------------------===//

/// Due loop sono adiacenti se l'uscita di L0 coincide con l'ingresso di L1.
/// - Loop non guarded: exit block di L0 == preheader di L1
/// - Loop guarded:     successore dell'exit dedicato di L0 == blocco guard di L1
static bool areAdjacent(Loop *L0, Loop *L1) {
  BasicBlock *Exit0 = L0->getExitBlock();
  if (!Exit0)
    return false;

  // Ingresso di L1: guard block se guarded, altrimenti preheader.
  BasicBlock *Entry1 = L1->isGuarded() ? L1->getLoopGuardBranch()->getParent()
                                       : L1->getLoopPreheader();
  if (!Entry1)
    return false;

  // Uscita effettiva di L0 verso L1.
  BasicBlock *Out0 = L0->isGuarded() ? Exit0->getTerminator()->getSuccessor(0)
                                     : Exit0;

  if (Out0 != Entry1)
    return false;

  // Il preheader di L1 non deve contenere altro che il salto verso l'header.
  if (BasicBlock *Pre1 = L1->getLoopPreheader()) {
    if (Pre1->size() != 1)
      return false;
  }

  // Se L1 e' guarded, il blocco guard deve contenere solo compare + branch.
  if (BranchInst *Guard1 = L1->getLoopGuardBranch()) {
    if (Guard1->getParent()->size() > 2)
      return false;
  }

  return true;
}

//===----------------------------------------------------------------------===//
// Condizione 2 — Stesso trip count (ScalarEvolution)
//===----------------------------------------------------------------------===//

static bool haveSameTripCount(Loop *L0, Loop *L1, ScalarEvolution &SE) {
  const SCEV *Trip0 = SE.getBackedgeTakenCount(L0);
  const SCEV *Trip1 = SE.getBackedgeTakenCount(L1);

  // SCEVCouldNotCompute: non sappiamo quante iterazioni fare.
  if (isa<SCEVCouldNotCompute>(Trip0) || isa<SCEVCouldNotCompute>(Trip1))
    return false;

  return Trip0 == Trip1;
}

//===----------------------------------------------------------------------===//
// Condizione 3 — Control flow equivalence
//===----------------------------------------------------------------------===//

/// L0 domina L1 e L1 post-domina L0.
/// Se entrambi guarded, le condizioni del guard devono essere identiche.
static bool areControlFlowEquivalent(Loop *L0, Loop *L1, DominatorTree &DT,
                                   PostDominatorTree &PDT) {
  // Uno guarded e l'altro no: non possono essere equivalenti.
  if (L0->isGuarded() != L1->isGuarded())
    return false;

  BasicBlock *Entry0 = L0->isGuarded() ? L0->getLoopGuardBranch()->getParent()
                                       : L0->getLoopPreheader();
  BasicBlock *Entry1 = L1->isGuarded() ? L1->getLoopGuardBranch()->getParent()
                                       : L1->getLoopPreheader();

  if (!Entry0 || !Entry1)
    return false;

  // Stessa guard condition se entrambi guarded.
  if (L0->isGuarded()) {
    BranchInst *G0 = L0->getLoopGuardBranch();
    BranchInst *G1 = L1->getLoopGuardBranch();
    if (G0->isConditional() && G1->isConditional()) {
      if (auto *C0 = dyn_cast<Instruction>(G0->getCondition())) {
        if (auto *C1 = dyn_cast<Instruction>(G1->getCondition())) {
          if (!C0->isIdenticalTo(C1))
            return false;
        }
      }
    }
  }

  return DT.dominates(Entry0, Entry1) && PDT.dominates(Entry1, Entry0);
}

//===----------------------------------------------------------------------===//
// Condizione 4 — Assenza di dipendenze negative
//===----------------------------------------------------------------------===//

/// Puntatore di un load/store (nullptr se I non e' load/store).
static Value *getMemPointer(Instruction *I) {
  if (auto *S = dyn_cast<StoreInst>(I))
    return S->getPointerOperand();
  if (auto *L = dyn_cast<LoadInst>(I))
    return L->getPointerOperand();
  return nullptr;
}

/// Verifica che non ci siano dipendenze con distanza negativa tra L0 e L1.
/// Usiamo DependenceAnalysis + SCEV (come slide: getSCEVAtScope per la load).
static bool hasNoNegativeDependence(Loop *L0, Loop *L1, DependenceInfo &DI,
                                    ScalarEvolution &SE) {
  for (BasicBlock *BB0 : L0->blocks()) {
    for (Instruction &I0 : *BB0) {
      Value *Ptr0 = getMemPointer(&I0);
      if (!Ptr0)
        continue;

      for (BasicBlock *BB1 : L1->blocks()) {
        for (Instruction &I1 : *BB1) {
          Value *Ptr1 = getMemPointer(&I1);
          if (!Ptr1)
            continue;

          // Load-load: nessuna dipendenza obbligatoria.
          if (isa<LoadInst>(&I0) && isa<LoadInst>(&I1))
            continue;

          std::unique_ptr<Dependence> Dep = DI.depends(&I0, &I1, true);
          if (!Dep)
            continue;

          // Espressioni SCEV nel rispettivo loop (slide: ricalcolare per L1).
          const SCEV *S0 = SE.getSCEVAtScope(Ptr0, L0);
          const SCEV *S1 = SE.getSCEVAtScope(Ptr1, L1);

          if (isa<SCEVCouldNotCompute>(S0) || isa<SCEVCouldNotCompute>(S1))
            return false;

          const SCEVAddRecExpr *AR0 = dyn_cast<SCEVAddRecExpr>(S0);
          const SCEVAddRecExpr *AR1 = dyn_cast<SCEVAddRecExpr>(S1);

          // Accessi non lineari (es. A[B[i]]): non fondiamo.
          if (!AR0 || !AR1)
            return false;

          // Step diverso: la distanza dipende dall'indice i.
          if (AR0->getStepRecurrence(SE) != AR1->getStepRecurrence(SE))
            return false;

          const SCEV *Dist = SE.getMinusSCEV(AR0->getStart(), AR1->getStart());
          if (Dist->isZero())
            continue;

          const SCEV *Step = AR0->getStepRecurrence(SE);

          // Step positivo + distanza negativa => dipendenza backward.
          if (SE.isKnownPositive(Step) && SE.isKnownNegative(Dist))
            return false;

          // Step negativo (loop decrescente): simmetrico.
          if (SE.isKnownNegative(Step) && SE.isKnownPositive(Dist))
            return false;
        }
      }
    }
  }

  return true;
}

/// Tutte e quattro le condizioni delle slide.
static bool canFuse(Loop *L0, Loop *L1, DominatorTree &DT,
                    PostDominatorTree &PDT, ScalarEvolution &SE,
                    DependenceInfo &DI) {
  if (!L0->isLoopSimplifyForm() || !L1->isLoopSimplifyForm())
    return false;

  // Solo loop innermost e fratelli.
  if (!L0->isInnermost() || !L1->isInnermost())
    return false;
  if (L0->getParentLoop() != L1->getParentLoop())
    return false;

  // Un solo exiting block ciascuno (altrimenti trip count ambiguo).
  if (!L0->getExitingBlock() || !L1->getExitingBlock())
    return false;

  return areAdjacent(L0, L1) && haveSameTripCount(L0, L1, SE) &&
         areControlFlowEquivalent(L0, L1, DT, PDT) &&
         hasNoNegativeDependence(L0, L1, DI, SE);
}

//===----------------------------------------------------------------------===//
// Trasformazione — fondere L1 dentro L0
//===----------------------------------------------------------------------===//

static bool fuseLoops(Loop *L0, Loop *L1) {
  // Header: un solo PHI (la variabile di induzione).
  int PhiCount0 = 0, PhiCount1 = 0;
  for (PHINode &PHI : L0->getHeader()->phis())
    PhiCount0++;
  for (PHINode &PHI : L1->getHeader()->phis())
    PhiCount1++;
  if (PhiCount0 > 1 || PhiCount1 > 1)
    return false;

  // Loop semplici: al massimo 3 blocchi oltre preheader/exit (no if interni).
  if (L0->getBlocks().size() > 3 || L1->getBlocks().size() > 3)
    return false;

  BasicBlock *Latch0 = L0->getLoopLatch();
  BasicBlock *Latch1 = L1->getLoopLatch();
  BasicBlock *Body0 = Latch0->getSinglePredecessor();
  BasicBlock *Body1 = Latch1->getSinglePredecessor();
  if (!Body0 || !Body1)
    return false;

  PHINode *IV0 = &*L0->getHeader()->phis().begin();
  PHINode *IV1 = &*L1->getHeader()->phis().begin();

  Value *Inc0 = IV0->getIncomingValueForBlock(Latch0);
  Value *Inc1 = IV1->getIncomingValueForBlock(Latch1);

  // Passo 1 (slide): sostituire la IV di L1 con quella di L0 nel corpo.
  Instruction *InsertPt = Body0->getTerminator();

  // Istruzione di confronto nel blocco exiting di L1 (non spostiamola).
  Instruction *ExitCond = nullptr;
  if (BasicBlock *Exiting1 = L1->getExitingBlock()) {
    if (auto *BI = dyn_cast<BranchInst>(Exiting1->getTerminator())) {
      if (BI->isConditional())
        ExitCond = dyn_cast<Instruction>(BI->getCondition());
    }
  }

  for (auto It = Body1->begin(); It != Body1->end();) {
    Instruction &Inst = *It++;
    if (Inst.isTerminator())
      break;
    if (isa<PHINode>(&Inst) || &Inst == Inc1 || &Inst == ExitCond)
      continue;

    Inst.replaceUsesOfWith(IV1, IV0);
    Inst.moveBefore(InsertPt);
  }

  // Aggiorna usi di IV1 e del suo incremento fuori dal "scheletro" di L1.
  for (auto It = IV1->use_begin(); It != IV1->use_end();) {
    Use &U = *It++;
    if (auto *UserI = dyn_cast<Instruction>(U.getUser())) {
      if (L1->contains(UserI))
        continue;
    }
    U.set(IV0);
  }

  for (auto It = Inc1->use_begin(); It != Inc1->use_end();) {
    Use &U = *It++;
    if (auto *UserI = dyn_cast<Instruction>(U.getUser())) {
      if (L1->contains(UserI))
        continue;
    }
    U.set(Inc0);
  }

  // Passo 2: L0 esce dove usciva L1.
  BasicBlock *Exiting0 = L0->getExitingBlock();
  BasicBlock *Exiting1 = L1->getExitingBlock();
  BasicBlock *Exit0 = L0->getExitBlock();
  BasicBlock *Exit1 = L1->getExitBlock();
  if (!Exiting0 || !Exiting1 || !Exit0 || !Exit1)
    return false;

  auto *Term0 = cast<BranchInst>(Exiting0->getTerminator());
  if (Term0->getSuccessor(0) == Exit0)
    Term0->setSuccessor(0, Exit1);
  else
    Term0->setSuccessor(1, Exit1);

  // Rimuovi Exiting1 come predecessore di Exit1 e reindirizzalo all'header L1
  // (L1 diventa codice morto ma il CFG resta valido).
  Exit1->removePredecessor(Exiting1);
  auto *Term1 = cast<BranchInst>(Exiting1->getTerminator());
  BasicBlock *HeaderSucc =
      Term1->getSuccessor(0) == Exit1 ? Term1->getSuccessor(1)
                                      : Term1->getSuccessor(0);
  BranchInst::Create(HeaderSucc, Term1);
  Term1->eraseFromParent();

  return true;
}

//===----------------------------------------------------------------------===//
// Pass principale
//===----------------------------------------------------------------------===//

/// Raccoglie i loop innermost in ordine di sorgente (come top-level, invertiti
/// per avere il dominante prima del dominato).
static void collectInnermostLoops(LoopInfo &LI, SmallVectorImpl<Loop *> &Out) {
  for (Loop *Top : LI) {
    SmallVector<Loop *, 8> Siblings;
    for (Loop *L : depth_first(Top)) {
      if (L->isInnermost())
        Siblings.push_back(L);
    }
    std::reverse(Siblings.begin(), Siblings.end());
    Out.append(Siblings.begin(), Siblings.end());
  }
}

struct LoopFusion : PassInfoMixin<LoopFusion> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
    DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
    PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
    DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

    if (LI.empty())
      return PreservedAnalyses::all();

    SmallVector<Loop *, 8> Loops;
    collectInnermostLoops(LI, Loops);

    bool Changed = false;

    // Confronta coppie adiacenti nell'ordine sorgente: L0 domina L1.
    for (unsigned i = 0; i + 1 < Loops.size(); ++i) {
      Loop *L0 = Loops[i + 1];
      Loop *L1 = Loops[i];

      if (!canFuse(L0, L1, DT, PDT, SE, DI))
        continue;

      if (fuseLoops(L0, L1))
        Changed = true;
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // namespace

static llvm::PassPluginLibraryInfo getLoopFusionPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "LoopFusion", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "loop-fusion-opt") {
                    FPM.addPass(LoopFusion());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getLoopFusionPluginInfo();
}
