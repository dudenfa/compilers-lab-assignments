//=============================================================================
// Multi instruction optimization pass (New Pass Manager plugin, LLVM 19+)
//
//   opt -S -load-pass-plugin=./multi-inst-opt.dylib -passes="multi-inst-opt" \
//       input.ll -o output.ll
//=============================================================================
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

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
            if (C0 && C0->getType() == COuter->getType() &&
                C0->getValue() == COuter->getValue()) {
              I.replaceAllUsesWith(InnerOp1);
              Changed = true;
            }
            // (x + C) - C  =>  x
            else if (C1 && C1->getType() == COuter->getType() &&
                     C1->getValue() == COuter->getValue()) {
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
            if (CInner && CInner->getType() == COuter->getType() &&
                CInner->getValue() == COuter->getValue()) {
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
