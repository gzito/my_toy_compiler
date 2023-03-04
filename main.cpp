#include <iostream>
#include "codegen.h"
#include "node.h"

using namespace std;

extern int yyparse();
extern NBlock* programBlock;

void open_file(const char* filename) {
	// openfile
	freopen(filename, "r", stdin);
}

// defined in corefn.cpp
void createCoreFunctions(CodeGenContext& context);

// global vars
std::unique_ptr<llvm::LLVMContext> MyContext ;
std::unique_ptr<llvm::IRBuilder<>> MyBuilder ;
std::unique_ptr<llvm::Module> MyModule ;
std::unique_ptr<llvm::legacy::FunctionPassManager> MyFPM ;

void initializePassManager(void) {
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  MyFPM->add(llvm::createInstructionCombiningPass());
  // Reassociate expressions.
  MyFPM->add(llvm::createReassociatePass());
  // Eliminate Common SubExpressions.
  MyFPM->add(llvm::createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  MyFPM->add(llvm::createCFGSimplificationPass());

  MyFPM->doInitialization();
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		open_file(argv[1]);
	}

	// Open a new context and module.
	MyContext = std::make_unique<llvm::LLVMContext>();
    MyModule = std::make_unique<llvm::Module>("my cool jit", *MyContext);
	
	// Create a new pass manager attached to it.
	MyFPM = std::make_unique<llvm::legacy::FunctionPassManager>(MyModule.get());
	initializePassManager();

	MyBuilder = std::make_unique<llvm::IRBuilder<>>(*MyContext);

    // used for jit
	/*
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();*/

	yyparse();
	cout << "Program block: " << programBlock << endl;

	CodeGenContext context;
	createCoreFunctions(context);
	context.generateCode(*programBlock);
	context.runCode();
	
	return 0;
}

