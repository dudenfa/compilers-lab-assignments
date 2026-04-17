//=============================================================================
// Strength reduction pass (New Pass Manager plugin, LLVM 19+)
//
//   opt -S -load-pass-plugin=./strength-reduction.dylib -passes="strength-reduction" \
//       input.ll -o output.ll
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

        // Moltiplicazione per potenza di 2 => shift left
        if (I.getOpcode() == Instruction::Mul){

            Value *Op0 = I.getOperand(0);
            Value *Op1 = I.getOperand(1);
            auto *C0 = dyn_cast<ConstantInt>(Op0);
            auto *C1 = dyn_cast<ConstantInt>(Op1);


            ConstantInt *C = nullptr;
            Value *Var = nullptr;
            if (C0) {
            C = C0;
            Var = Op1;
            } else if (C1) {
            C = C1;
            Var = Op0;
            } else {
            continue;
            }

            IRBuilder<> Builder(&I);
            const APInt &CV = C->getValue();

            // x * 2^n => x << n   oppure   2^n * x
            if (CV.isPowerOf2()) {
            Value *ShiftConst =
                ConstantInt::get(I.getType(), CV.exactLogBase2());
            Value *Shl = Builder.CreateShl(Var, ShiftConst, "sr.shl");
            I.replaceAllUsesWith(Shl);
            Changed = true;
            }
            // x * (2^n + 1) => (x << n) + x
            else if ((CV - 1).isPowerOf2()) {
            Value *ShiftConst = ConstantInt::get(
                I.getType(), (CV - 1).exactLogBase2());
            Value *Shl = Builder.CreateShl(Var, ShiftConst, "sr.shl");
            Value *Add = Builder.CreateAdd(Shl, Var, "sr.add");
            I.replaceAllUsesWith(Add);
            Changed = true;
            }
            // x * (2^n - 1) => (x << n) - x   (es. n>=1)
            else if (!CV.isZero() && (CV + 1).isPowerOf2()) {
            Value *ShiftConst = ConstantInt::get(
                I.getType(), (CV + 1).exactLogBase2());
            Value *Shl = Builder.CreateShl(Var, ShiftConst, "sr.shl");
            Value *Sub = Builder.CreateSub(Shl, Var, "sr.sub");
            I.replaceAllUsesWith(Sub);
                Changed = true;
            }
        }
        

        // Divisione per potenza di 2 => shift right
        // Nota: nella divisione il dividendo è Op0 e il divisore è Op1 (x / C),
        // non è commutativa quindi gestiamo solo la costante a destra.
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

            // x / 2^n => x >> n   (lshr per unsigned, ashr per signed)
            if (CV.isPowerOf2()) {
            Value *ShiftConst =
                ConstantInt::get(I.getType(), CV.exactLogBase2());
            Value *Shr = IsUnsigned
                ? Builder.CreateLShr(Var, ShiftConst, "sr.lshr")
                : Builder.CreateAShr(Var, ShiftConst, "sr.ashr");
            I.replaceAllUsesWith(Shr);
            Changed = true;
            }
            // x / (2^n + 1) => (x - (x >> n)) >> n   (approssimazione per unsigned)
            else if (IsUnsigned && (CV - 1).isPowerOf2()) {
            unsigned n = (CV - 1).exactLogBase2();
            Value *ShiftConst = ConstantInt::get(I.getType(), n);
            Value *Shr = Builder.CreateLShr(Var, ShiftConst, "sr.lshr");
            Value *Sub = Builder.CreateSub(Var, Shr, "sr.sub");
            Value *Res = Builder.CreateLShr(Sub, ShiftConst, "sr.lshr2");
            I.replaceAllUsesWith(Res);
            Changed = true;
            }
            // x / (2^n - 1) => (x + (x >> n)) >> n   (approssimazione per unsigned)
            else if (IsUnsigned && !CV.isZero() && (CV + 1).isPowerOf2()) {
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
