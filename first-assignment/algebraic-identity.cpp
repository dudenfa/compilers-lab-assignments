//=============================================================================
// Algebraic identity pass (New Pass Manager plugin, LLVM 19+)
//
//   opt -S -load-pass-plugin=./algebraic-identity.dylib -passes="alg-id" \
//       input.ll -o output.ll
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

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {

        // Addizione e sottrazione con 0
        if (I.getOpcode() == Instruction::Add || I.getOpcode() == Instruction::Sub){
        Value *L = I.getOperand(0);
        Value *R = I.getOperand(1);
        auto *CL = dyn_cast<ConstantInt>(L);
        auto *CR = dyn_cast<ConstantInt>(R);

        // x + 0  =>  x
        // x - 0  =>  x
        if (CR && CR->isZero()) {
          I.replaceAllUsesWith(L);
          Changed = true;
        } 
        // controllo solo addizioni (proprietà commutativa)
        else if (I.getOpcode() == Instruction::Add && CL && CL->isZero()) {
          // 0 + x  =>  x
          I.replaceAllUsesWith(R);
          Changed = true;
        }
      }

        // Moltiplicazione e divisione per 1
        if (I.getOpcode() == Instruction::Mul || I.getOpcode() == Instruction::UDiv || I.getOpcode() == Instruction::SDiv ){
          
        Value *L = I.getOperand(0);
        Value *R = I.getOperand(1);
        auto *CL = dyn_cast<ConstantInt>(L);
        auto *CR = dyn_cast<ConstantInt>(R);
        // x * 1  =>  x
        // x / 1  =>  x
        if (CR && CR->isOne()) {
          I.replaceAllUsesWith(L);
          Changed = true;
        } 
        // controllo solo multiplicazioni (proprietà commutativa)
        else if (I.getOpcode() == Instruction::Mul && CL && CL->isOne()) {
          // 1 * x  =>  x
          I.replaceAllUsesWith(R);
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
