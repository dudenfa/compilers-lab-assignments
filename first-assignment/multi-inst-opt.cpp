//=============================================================================
// PASS 3 — Multi Instruction Optimization (New Pass Manager, LLVM 19+)
//=============================================================================
//
// COSA FA 
//   Trova coppie di istruzioni consecutive che si annullano e semplifica:
//     a = b + 1; c = a - 1  =>  c = b        (caso del prof)
//     a = b * 4; c = a / 4  =>  c = b        (mul poi div)
//   Non ottimizza (x/3)*3 perche' la divisione intera perde informazione:
//     con x=7: 7/3=2, 2*3=6 != 7
//
// COME FUNZIONA
//   1. Per ogni istruzione I, controlla se un operando e' il risultato di
//      un'altra istruzione (Inner) con operazione "opposta"
//   2. Verifica che le costanti siano uguali (constantsMatch per valore)
//   3. replaceAllUsesWith() sostituisce I con l'operando originale di Inner
//   4. Scope: solo istruzioni consecutive nello stesso basic block in SSA
//
// PERCHE' SERVE mem2reg (IMPORTANTE per i test)
//   Clang -O0 genera variabili su stack (alloca + load/store):
//     store a; load a; sub ...   --> il pass NON vede il pattern
//   mem2reg promuove in SSA:
//     %a = add ...; %c = sub %a ...  --> il pass vede il pattern
//   Pipeline test: mem2reg,multi-inst-opt,dce
//
// COME COMPILARE IL PLUGIN
//   clang++ -std=c++17 -fPIC -shared -o multi-inst-opt.dylib \
//     multi-inst-opt.cpp \
//     $(/opt/homebrew/opt/llvm@19/bin/llvm-config --cxxflags --ldflags \
//       --system-libs --libs core passes)
//
// COME TESTARE MANUALMENTE
//   cd first-assignment/
//
//   clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
//     tests/multi-inst-opt/test.c -o tests/multi-inst-opt/test.ll
//
//   opt -S -load-pass-plugin=./multi-inst-opt.dylib \
//     -passes="mem2reg,multi-inst-opt,dce" \
//     tests/multi-inst-opt/test.ll \
//     -o tests/multi-inst-opt/multi-inst-opt.test.optimized.dce.ll
//
//   Cosa verificare:
//     @test_assignment: ret i32 %0 (ritorna b direttamente, niente sub)
//     @test_mul_sdiv:   ret i32 %0 (ritorna x, niente sdiv)
//     @test_div_mul_no_opt: mul ancora presente (7/3*3 non ottimizzato)
//
// NOME PIPELINE: multi-inst-opt
//=============================================================================
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

// Confronta il valore numerico di due costanti (non i puntatori in memoria).
// Due ConstantInt con valore 5 ma creati separatamente devono matchare.
bool constantsMatch(ConstantInt *A, ConstantInt *B) {
  return A && B && A->getType() == B->getType() &&
         A->getValue() == B->getValue();
}

struct MultiInstOpt : PassInfoMixin<MultiInstOpt> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool Changed = false;

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {

        // Caso 1: (x + C) - C => x
        //   Inner = add,  I = sub,  Op0(I) == risultato di Inner
        if (I.getOpcode() == Instruction::Sub) {

          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);

          auto *Inner = dyn_cast<Instruction>(Op0);
          auto *COuter = dyn_cast<ConstantInt>(Op1);

          if (Inner && COuter && Inner->getOpcode() == Instruction::Add) {
            Value *InnerOp0 = Inner->getOperand(0);
            Value *InnerOp1 = Inner->getOperand(1);

            auto *C0 = dyn_cast<ConstantInt>(InnerOp0);
            auto *C1 = dyn_cast<ConstantInt>(InnerOp1);

            // (C + x) - C => x
            if (constantsMatch(C0, COuter)) {
              I.replaceAllUsesWith(InnerOp1);
              Changed = true;
            }
            // (x + C) - C => x   <-- caso del prof: (b+1)-1 => b
            else if (constantsMatch(C1, COuter)) {
              I.replaceAllUsesWith(InnerOp0);
              Changed = true;
            }
          }
        }

        // Caso 2: (x - C) + C => x
        if (I.getOpcode() == Instruction::Add) {

          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);

          Instruction *Inner = nullptr;
          ConstantInt *COuter = nullptr;

          // La costante puo' stare a sinistra o a destra dell'add esterna
          if (auto *C0 = dyn_cast<ConstantInt>(Op0)) {
            COuter = C0;
            Inner = dyn_cast<Instruction>(Op1);
          } else if (auto *C1 = dyn_cast<ConstantInt>(Op1)) {
            COuter = C1;
            Inner = dyn_cast<Instruction>(Op0);
          }

          if (Inner && COuter && Inner->getOpcode() == Instruction::Sub) {
            Value *InnerOp0 = Inner->getOperand(0);
            Value *InnerOp1 = Inner->getOperand(1);
            auto *CInner = dyn_cast<ConstantInt>(InnerOp1);

            // (x - C) + C => x
            // NOTA: (C - x) + C NON matcha (costante e' Op1 della sub, non Op0)
            if (constantsMatch(CInner, COuter)) {
              I.replaceAllUsesWith(InnerOp0);
              Changed = true;
            }
          }
        }

        // Caso 3: (x * C) / C => x   (caso sicuro: moltiplica poi dividi)
        if (I.getOpcode() == Instruction::SDiv ||
            I.getOpcode() == Instruction::UDiv) {

          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);

          auto *Inner = dyn_cast<Instruction>(Op0);
          auto *COuter = dyn_cast<ConstantInt>(Op1);

          if (Inner && COuter && Inner->getOpcode() == Instruction::Mul) {
            Value *InnerOp0 = Inner->getOperand(0);
            Value *InnerOp1 = Inner->getOperand(1);

            auto *C0 = dyn_cast<ConstantInt>(InnerOp0);
            auto *C1 = dyn_cast<ConstantInt>(InnerOp1);

            // (C * x) / C => x
            if (constantsMatch(C0, COuter)) {
              I.replaceAllUsesWith(InnerOp1);
              Changed = true;
            }
            // (x * C) / C => x
            else if (constantsMatch(C1, COuter)) {
              I.replaceAllUsesWith(InnerOp0);
              Changed = true;
            }
          }
        }

        // Caso 4: (x / C) * C => x   SOLO se la divisione e' "exact"
        // Esempio perche' NON ottimizziamo da C: (7/2)*2 = 3*2 = 6 != 7
        // Clang -O0 non genera il flag exact, quindi nei test C mostriamo
        // i casi negativi (x/3)*3 e (x/8)*8 in test.c
        if (I.getOpcode() == Instruction::Mul) {

          Value *Op0 = I.getOperand(0);
          Value *Op1 = I.getOperand(1);

          Instruction *Inner = nullptr;
          ConstantInt *COuter = nullptr;

          if (auto *C0 = dyn_cast<ConstantInt>(Op0)) {
            COuter = C0;
            Inner = dyn_cast<Instruction>(Op1);
          } else if (auto *C1 = dyn_cast<ConstantInt>(Op1)) {
            COuter = C1;
            Inner = dyn_cast<Instruction>(Op0);
          }

          if (Inner && COuter &&
              (Inner->getOpcode() == Instruction::SDiv ||
               Inner->getOpcode() == Instruction::UDiv)) {

            auto *Div = cast<BinaryOperator>(Inner);
            Value *InnerOp0 = Div->getOperand(0);
            Value *InnerOp1 = Div->getOperand(1);
            auto *CInner = dyn_cast<ConstantInt>(InnerOp1);

            // isExact() = LLVM garantisce che (x/C)*C == x (divisione reversibile)
            if (Div->isExact() && constantsMatch(CInner, COuter)) {
              I.replaceAllUsesWith(InnerOp0);
              Changed = true;
            }
          }
        }
      }
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  static bool isRequired() { return true; }
};

} // namespace

llvm::PassPluginLibraryInfo getMultiInstOptPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "MultiInstOpt", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "multi-inst-opt") {
                    FPM.addPass(MultiInstOpt());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getMultiInstOptPluginInfo();
}
