#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

class BranchCounterPass : public PassInfoMixin<BranchCounterPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
    
private:
    void insertCounterCall(Instruction *InsertBefore, const char *funcName, Module *M);
};

void BranchCounterPass::insertCounterCall(Instruction *InsertBefore, 
                                          const char *funcName, Module *M) {
    LLVMContext &Ctx = M->getContext();
    IRBuilder<> Builder(InsertBefore);
    
    FunctionType *FuncTy = FunctionType::get(Type::getVoidTy(Ctx), false);
    FunctionCallee CounterFunc = M->getOrInsertFunction(funcName, FuncTy);
    
    Builder.CreateCall(CounterFunc);
}

PreservedAnalyses BranchCounterPass::run(Function &F, FunctionAnalysisManager &FAM) {
    Module *M = F.getParent();
    LLVMContext &Ctx = M->getContext();
    
    // Skip our instrumentation functions
    StringRef FuncName = F.getName();
    if (FuncName.starts_with("increment_") || 
        FuncName.starts_with("print_") ||
        FuncName.starts_with("reset_") ||
        FuncName.starts_with("init_") ||
        FuncName.starts_with("get_")) {
        return PreservedAnalyses::all();
    }
    
    // Get Loop Information
    LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
    
    // Collect all loop headers
    SmallPtrSet<BasicBlock*, 32> LoopHeaders;
    for (Loop *L : LI) {
        std::function<void(Loop*)> collectHeaders = [&](Loop *LP) {
            LoopHeaders.insert(LP->getHeader());
            for (Loop *SubL : LP->getSubLoops()) {
                collectHeaders(SubL);
            }
        };
        collectHeaders(L);
    }
    
    bool Modified = false;
    
    // Iterate through all basic blocks
    for (BasicBlock &BB : F) {
        // Instrument loop headers at the beginning of the block
        if (LoopHeaders.count(&BB)) {
            Instruction *FirstInst = &BB.front();
            // Skip PHI nodes
            while (isa<PHINode>(FirstInst)) {
                FirstInst = FirstInst->getNextNode();
                if (!FirstInst) break;
            }
            if (FirstInst) {
                insertCounterCall(FirstInst, "increment_loop_header", M);
                Modified = true;
            }
        }
        
        // Iterate through instructions
        std::vector<std::pair<Instruction*, const char*>> ToInstrument;
        
        for (Instruction &Inst : BB) {
            // Count branches
            if (BranchInst *BI = dyn_cast<BranchInst>(&Inst)) {
                if (BI->isConditional()) {
                    ToInstrument.push_back({BI, "increment_cond_branch"});
                } else {
                    ToInstrument.push_back({BI, "increment_uncond_branch"});
                }
            }
            
            // Count direct calls
            else if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
                Function *Callee = CI->getCalledFunction();
                if (Callee && !Callee->isIntrinsic()) {
                    StringRef CalleeName = Callee->getName();
                    // Don't instrument our own functions
                    if (!CalleeName.starts_with("increment_") &&
                        !CalleeName.starts_with("print_") &&
                        !CalleeName.starts_with("reset_") &&
                        !CalleeName.starts_with("init_") &&
                        !CalleeName.starts_with("get_") &&
                        !CalleeName.starts_with("llvm.")) {
                        ToInstrument.push_back({CI, "increment_direct_call"});
                    }
                }
            }
            
            // Count returns
            else if (isa<ReturnInst>(&Inst)) {
                ToInstrument.push_back({&Inst, "increment_return"});
            }
        }
        
        // Insert instrumentation calls
        for (auto &Pair : ToInstrument) {
            insertCounterCall(Pair.first, Pair.second, M);
            Modified = true;
        }
    }
    
    return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end anonymous namespace

// New PM Registration
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "BranchCounter",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            // Register for optimization pipeline
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "branch-counter") {
                        FPM.addPass(BranchCounterPass());
                        return true;
                    }
                    return false;
                });
            
            // Also register at pipeline start for automatic instrumentation
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    FunctionPassManager FPM;
                    FPM.addPass(BranchCounterPass());
                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
        }
    };
}