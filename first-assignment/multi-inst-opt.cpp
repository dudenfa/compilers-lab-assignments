//=============================================================================
// Multi instruction optimization pass (New Pass Manager plugin, LLVM 19+)
//
//   opt -S -load-pass-plugin=./multi-inst-opt.dylib \
//       -passes="mem2reg,multi-inst-opt" input.ll -o output.ll
//
// Su IR generato da C con -O0, usare mem2reg prima del pass per promuovere
// le variabili stack in SSA e rendere visibili pattern come:
//   a = b + C; c = a - C  =>  c = b
//   a = b * C; c = a / C  =>  c = b
//   a = b / C; c = a * C  =>  c = b  (solo se la divisione e' exact)
//=============================================================================
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

// Confronta il valore numerico di due costanti intere (non i puntatori).
bool constantsMatch(ConstantInt *A, ConstantInt *B) {
  return A && B && A->getType() == B->getType() &&
         A->getValue() == B->getValue();
}

struct MultiInstOpt : PassInfoMixin<MultiInstOpt> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    bool Changed = false;

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {

        // Caso 1:
        //   a = b + C
        //   c = a - C
        // diventa:
        //   c = b
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

            // (C + x) - C  =>  x
            if (constantsMatch(C0, COuter)) {
              I.replaceAllUsesWith(InnerOp1);
              Changed = true;
            }
            // (x + C) - C  =>  x
            else if (constantsMatch(C1, COuter)) {
              I.replaceAllUsesWith(InnerOp0);
              Changed = true;
            }
          }
        }

        // Caso 2:
        //   a = b - C
        //   c = a + C
        // diventa:
        //   c = b
        if (I.getOpcode() == Instruction::Add) {

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

          if (Inner && COuter && Inner->getOpcode() == Instruction::Sub) {
            Value *InnerOp0 = Inner->getOperand(0);
            Value *InnerOp1 = Inner->getOperand(1);

            auto *CInner = dyn_cast<ConstantInt>(InnerOp1);

            // (x - C) + C  =>  x
            // Nota: la costante deve essere il secondo operando della sub (x - C),
            // non (C - x), che non e' invertibile in forma lineare.
            if (constantsMatch(CInner, COuter)) {
              I.replaceAllUsesWith(InnerOp0);
              Changed = true;
            }
          }
        }

        // Caso 3:
        //   a = b * C
        //   c = a / C
        // diventa:
        //   c = b
        // Vale per sdiv e udiv. La moltiplicazione e' commutativa (C * b ok).
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

            // (C * x) / C  =>  x
            if (constantsMatch(C0, COuter)) {
              I.replaceAllUsesWith(InnerOp1);
              Changed = true;
            }
            // (x * C) / C  =>  x
            else if (constantsMatch(C1, COuter)) {
              I.replaceAllUsesWith(InnerOp0);
              Changed = true;
            }
          }
        }

        // Caso 4:
        //   a = b / C
        //   c = a * C
        // diventa:
        //   c = b
        // Solo se la divisione originale e' "exact" (nessun resto/troncamento).
        // Esempio: (7 / 2) * 2 = 6 != 7  -->  NON ottimizziamo.
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

            // (x / C) * C  =>  x
            // Richiede isExact(): la divisione intera deve essere reversibile.
            // La costante C deve essere il divisore (secondo operando), non (C / x).
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
