#include <memory>
#include <stack>
#include <typeinfo>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Bitstream/BitstreamReader.h>
#include <llvm/Bitstream/BitstreamWriter.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>

// function pass optimization
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

//using namespace llvm;
extern std::unique_ptr<llvm::LLVMContext> MyContext ;
extern std::unique_ptr<llvm::IRBuilder<>> MyBuilder ;
extern std::unique_ptr<llvm::legacy::FunctionPassManager> MyFPM ;

class NBlock;

class CodeGenBlock {
public:
    llvm::BasicBlock *block;
    llvm::Value *returnValue;
    std::map<std::string, llvm::Value*> locals;
};

class CodeGenContext {
    std::stack<CodeGenBlock *> blocks;
    llvm::Function *mainFunction;

public:
    llvm::Module *module;

    // ctor
    CodeGenContext(llvm::Module* module_ptr) {
        this->module = module_ptr;
    }
    
    void generateCode(NBlock& root);
    void optimizeCode();
    void dumpCode();
    int runCode();

    std::map<std::string, llvm::Value*>& locals() { return blocks.top()->locals; }

    llvm::BasicBlock *currentBlock() { return blocks.top()->block; }

    void pushBlock(llvm::BasicBlock *block) {
        blocks.push(new CodeGenBlock());
        blocks.top()->returnValue = NULL;
        blocks.top()->block = block;
    }
    
    void popBlock() {
        CodeGenBlock *top = blocks.top();
        blocks.pop();
        delete top;
    }
    
    void setCurrentReturnValue(llvm::Value *value) { blocks.top()->returnValue = value; }
    
    llvm::Value* getCurrentReturnValue() { return blocks.top()->returnValue; }
};
