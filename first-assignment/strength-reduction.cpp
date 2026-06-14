//=============================================================================
// PASS 2 — Strength Reduction (New Pass Manager, LLVM 19+)
//=============================================================================
//
// COSA FA
//   Sostituisce moltiplicazioni/divisioni costose con shift e add/sub:
//     15 * x  =>  (x << 4) - x     perche' 15 = 2^4 - 1
//     x / 8   =>  x >> 3           (ashr signed, lshr unsigned)
//   Generalizza anche ad altre costanti 2^n, 2^n+1, 2^n-1 (moltiplicazione).
//
// COME FUNZIONA
//   1. Cerca istruzioni Mul/UDiv/SDiv con una costante intera
//   2. Se la costante e' potenza di 2 (o 2^n +/- 1), crea nuove istruzioni
//      con IRBuilder (shl, lshr, ashr, add, sub)
//   3. replaceAllUsesWith() collega i clienti alla nuova espressione
//   4. L'istruzione vecchia resta finche' dce non la rimuove
//
// lshr vs ashr (signed vs unsigned):
//   - udiv (unsigned, solo >= 0) => lshr (shift logico, riempie con 0)
//   - sdiv (signed, anche negativi) => ashr (shift aritmetico, copia segno)
//   Limite: su sdiv negativi non multipli, ashr puo' differire da sdiv C.
//   Per l'assignment va bene; per correttezza completa servirebbe un bias.
//
// COME COMPILARE IL PLUGIN
//   clang++ -std=c++17 -fPIC -shared -o strength-reduction.dylib \
//     strength-reduction.cpp \
//     $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags \
//       --system-libs --libs core passes)
//
// COME TESTARE MANUALMENTE
//   cd first-assignment/
//
//   clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
//     tests/strength-reduction/test.c -o tests/strength-reduction/test.ll
//
//   opt -S -load-pass-plugin=./strength-reduction.dylib \
//     -passes="strength-reduction,dce" \
//     tests/strength-reduction/test.ll \
//     -o tests/strength-reduction/strength-reduction.test.optimized.dce.ll
//
//   Cosa verificare nell'IR ottimizzato (@test_mul):
//     - x*8  => shl,  x*9 => shl+add,  x*15 => shl+sub
//     - x*6  => mul ancora presente (non ottimizzato)
//   In @test_sdiv: x/8 => ashr.  In @test_udiv: x/4 => lshr.
//
//   ./run-tests.sh
//
// NOME PIPELINE: strength-reduction
//=============================================================================
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace {

struct StrengthReduction : PassInfoMixin<StrengthReduction> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool Changed = false;

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {

        // --- MOLTIPLICAZIONE: x * C => shift (+/- x se C = 2^n +/- 1) ---
        if (I.getOpcode() == Instruction::Mul) {

          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);
          auto *C0 = dyn_cast<ConstantInt>(Op0);
          auto *C1 = dyn_cast<ConstantInt>(Op1);

          // Identifica quale operando e' la costante e quale la variabile
          ConstantInt *C = nullptr;
          Value *Var = nullptr;
          if (C0) {
            C = C0;
            Var = Op1;
          } else if (C1) {
            C = C1;
            Var = Op0;  // commutativita': 15 * x == x * 15
          } else {
            continue;   // nessuna costante, salta
          }

          IRBuilder<> Builder(&I);
          const APInt &CV = C->getValue();

          // x * 2^n => x << n
          if (CV.isPowerOf2()) {
            Value *ShiftConst =
                ConstantInt::get(I.getType(), CV.exactLogBase2());
            Value *Shl = Builder.CreateShl(Var, ShiftConst, "sr.shl");
            I.replaceAllUsesWith(Shl);
            Changed = true;
          }
          // x * (2^n + 1) => (x << n) + x     es. x*9 = (x<<3)+x
          else if ((CV - 1).isPowerOf2()) {
            Value *ShiftConst =
                ConstantInt::get(I.getType(), (CV - 1).exactLogBase2());
            Value *Shl = Builder.CreateShl(Var, ShiftConst, "sr.shl");
            Value *Add = Builder.CreateAdd(Shl, Var, "sr.add");
            I.replaceAllUsesWith(Add);
            Changed = true;
          }
          // x * (2^n - 1) => (x << n) - x     es. x*15 = (x<<4)-x
          else if (!CV.isZero() && (CV + 1).isPowerOf2()) {
            Value *ShiftConst =
                ConstantInt::get(I.getType(), (CV + 1).exactLogBase2());
            Value *Shl = Builder.CreateShl(Var, ShiftConst, "sr.shl");
            Value *Sub = Builder.CreateSub(Shl, Var, "sr.sub");
            I.replaceAllUsesWith(Sub);
            Changed = true;
          }
        }

        // --- DIVISIONE: x / C => shift right ---
        // La divisione NON e' commutativa: costante solo a destra (Op1)
        if (I.getOpcode() == Instruction::SDiv ||
            I.getOpcode() == Instruction::UDiv) {

          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);
          auto *C = dyn_cast<ConstantInt>(Op1);

          if (!C)
            continue;

          Value *Var = Op0;
          IRBuilder<> Builder(&I);
          const APInt &CV = C->getValue();
          bool IsUnsigned = (I.getOpcode() == Instruction::UDiv);

          // x / 2^n => x >> n
          if (CV.isPowerOf2()) {
            Value *ShiftConst =
                ConstantInt::get(I.getType(), CV.exactLogBase2());
            Value *Shr = IsUnsigned
                             ? Builder.CreateLShr(Var, ShiftConst, "sr.lshr")
                             : Builder.CreateAShr(Var, ShiftConst, "sr.ashr");
            I.replaceAllUsesWith(Shr);
            Changed = true;
          }
          // Extra unsigned: x / (2^n + 1) e x / (2^n - 1)
          else if (IsUnsigned && (CV - 1).isPowerOf2()) {
            unsigned n = (CV - 1).exactLogBase2();
            Value *ShiftConst = ConstantInt::get(I.getType(), n);
            Value *Shr = Builder.CreateLShr(Var, ShiftConst, "sr.lshr");
            Value *Sub = Builder.CreateSub(Var, Shr, "sr.sub");
            Value *Res = Builder.CreateLShr(Sub, ShiftConst, "sr.lshr2");
            I.replaceAllUsesWith(Res);
            Changed = true;
          } else if (IsUnsigned && !CV.isZero() && (CV + 1).isPowerOf2()) {
            unsigned n = (CV + 1).exactLogBase2();
            Value *ShiftConst = ConstantInt::get(I.getType(), n);
            Value *Shr = Builder.CreateLShr(Var, ShiftConst, "sr.lshr");
            Value *Add = Builder.CreateAdd(Var, Shr, "sr.add");
            Value *Res = Builder.CreateLShr(Add, ShiftConst, "sr.lshr2");
            I.replaceAllUsesWith(Res);
            Changed = true;
          }
        }
      }
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // namespace

llvm::PassPluginLibraryInfo getStrengthReductionPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "StrengthReduction", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "strength-reduction") {
                    FPM.addPass(StrengthReduction());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getStrengthReductionPluginInfo();
}
