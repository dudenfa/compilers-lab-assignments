#include "llvm/IR/PassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

struct MultiInstructionOptPass : public PassInfoMixin<MultiInstructionOptPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        
        for (BasicBlock &BB : F) {
            for (Instruction &Inst : BB) {
                // Inst rappresenta la nostra "seconda operazione" (ovvero l'assegnamento a 'c')
                if (BinaryOperator *seconda_istruzione = dyn_cast<BinaryOperator>(&Inst)) {
                    
                    // Prendiamo il primo operando di 'c', che dovrebbe essere il risultato di 'a'
                    if (BinaryOperator *prima_istruzione = dyn_cast<BinaryOperator>(seconda_istruzione->getOperand(0))) {
                        
                        unsigned prima_operazione = prima_istruzione->getOpcode();
                        unsigned seconda_operazione = seconda_istruzione->getOpcode();

                        // 1. PRIMO IF: controlla se sono somma e sottrazione alternate
                        if ((prima_operazione == Instruction::Add && seconda_operazione == Instruction::Sub) ||
                            (prima_operazione == Instruction::Sub && seconda_operazione == Instruction::Add)) {
                            
                            // 2. SECONDO IF: controlla che la costante sia il SECONDO operatore (indice 1 in LLVM).
                            // Questo garantisce che abbiamo "b +/- k1" e "a -/+ k2", scartando i casi come "k - b".
                            if (isa<ConstantInt>(prima_istruzione->getOperand(1)) && 
                                isa<ConstantInt>(seconda_istruzione->getOperand(1))) {
                                
                                ConstantInt *k1 = cast<ConstantInt>(prima_istruzione->getOperand(1));
                                ConstantInt *k2 = cast<ConstantInt>(seconda_istruzione->getOperand(1));

                                // 3. TERZO IF: controlla se le costanti sono uguali
                                // In LLVM non possiamo fare semplicemente "k1 == k2" sui puntatori, 
                                // dobbiamo comparare il loro valore effettivo.
                                if (k1->getValue() == k2->getValue()) {
                                    
                                    Value *b = prima_istruzione->getOperand(0);

                                    // Sostituisce tutti gli utilizzi di 'c' direttamente con 'b'
                                    seconda_istruzione->replaceAllUsesWith(b);

                                    // L'istruzione Ã¨ stata ottimizzata, ritorniamo come richiesto interrompendo l'albero
                                    return PreservedAnalyses::none();
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Se non trova nessuna ottimizzazione, preserva le analisi
        return PreservedAnalyses::all();
    }
};

} // namespace

llvm::PassPluginLibraryInfo getMultiInstructionOptPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "MultiInstructionOpt", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "multi-instruction-opt") {
                            FPM.addPass(MultiInstructionOptPass());
                            return true;
                        }
                        return false;
                    });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getMultiInstructionOptPluginInfo();
}