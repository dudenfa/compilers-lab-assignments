//=============================================================================
// PASS 1 — Algebraic Identity (New Pass Manager, LLVM 19+)
//=============================================================================
//
// COSA FA
//   Riconosce operazioni binarie con elementi neutri e le semplifica:
//     x + 0 => x,  0 + x => x,  x - 0 => x
//     x * 1 => x,  1 * x => x,  x / 1 => x
//   Non tocca casi come 0-x, x*0, 1/x (non sono identita' algebriche).
//
// COME FUNZIONA
//   1. Scorre ogni istruzione di ogni basic block
//   2. Se trova una costante neutra (0 o 1), chiama replaceAllUsesWith()
//      per sostituire tutti gli usi del risultato con l'altro operando
//   3. L'istruzione originale resta nell'IR (codice morto) finche' dce
//      non la rimuove
//   Scope: solo dentro un singolo basic block (analisi locale)
//
// COME COMPILARE IL PLUGIN
//   clang++ -std=c++17 -fPIC -shared -o algebraic-identity.dylib \
//     algebraic-identity.cpp \
//     $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags \
//       --system-libs --libs core passes)
//
// COME TESTARE MANUALMENTE
//   cd first-assignment/
//
//   # 1) Genera IR grezzo da C (-O0, senza optnone)
//   clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
//     tests/algebraic-identity/test.c -o tests/algebraic-identity/test.ll
//
//   # 2) Applica il pass + dce (Dead Code Elimination)
//   opt -S -load-pass-plugin=./algebraic-identity.dylib \
//     -passes="alg-id,dce" \
//     tests/algebraic-identity/test.ll \
//     -o tests/algebraic-identity/alg-id.test.optimized.dce.ll
//
//   # 3) Confronta i due file: in quello ottimizzato non devono esserci
//      piu' add/mul/sdiv con 0 o 1 (tranne casi non ottimizzabili)
//
// NOME PIPELINE: alg-id
//=============================================================================
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

struct AlgebraicIdentity : PassInfoMixin<AlgebraicIdentity> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool Changed = false;

    // Visita ogni basic block e ogni istruzione (analisi locale)
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {

        // --- Addizione e sottrazione con 0 ---
        if (I.getOpcode() == Instruction::Add ||
            I.getOpcode() == Instruction::Sub) {

          Value *L = I.getOperand(0);  // operando sinistro
          Value *R = I.getOperand(1);  // operando destro
          auto *CL = dyn_cast<ConstantInt>(L);
          auto *CR = dyn_cast<ConstantInt>(R);

          // x + 0 => x,  x - 0 => x  (costante zero a destra)
          if (CR && CR->isZero()) {
            I.replaceAllUsesWith(L);  // chi usa I, ora usa L
            Changed = true;
          }
          // 0 + x => x  (solo add: la somma e' commutativa)
          else if (I.getOpcode() == Instruction::Add && CL && CL->isZero()) {
            I.replaceAllUsesWith(R);
            Changed = true;
          }
          // NOTA: 0 - x NON viene ottimizzato (non e' identita')
        }

        // --- Moltiplicazione e divisione per 1 ---
        if (I.getOpcode() == Instruction::Mul ||
            I.getOpcode() == Instruction::UDiv ||
            I.getOpcode() == Instruction::SDiv) {

          Value *L = I.getOperand(0);
          Value *R = I.getOperand(1);
          auto *CL = dyn_cast<ConstantInt>(L);
          auto *CR = dyn_cast<ConstantInt>(R);

          // x * 1 => x,  x / 1 => x  (costante uno a destra)
          if (CR && CR->isOne()) {
            I.replaceAllUsesWith(L);
            Changed = true;
          }
          // 1 * x => x  (solo mul: commutativa)
          else if (I.getOpcode() == Instruction::Mul && CL && CL->isOne()) {
            I.replaceAllUsesWith(R);
            Changed = true;
          }
          // NOTA: x * 0, 0 / x, 1 / x NON vengono ottimizzati
        }
      }
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  // Necessario per far girare il pass anche su codice compilato con -O0
  // (clang marca le funzioni con attributo optnone senza questo)
  static bool isRequired() { return true; }
};

} // namespace

// Registrazione del pass nel New Pass Manager.
// Permette di invocarlo con: opt -passes="alg-id" ...
llvm::PassPluginLibraryInfo getAlgebraicIdentityPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "AlgebraicIdentity", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "alg-id") {
                    FPM.addPass(AlgebraicIdentity());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getAlgebraicIdentityPluginInfo();
}
