#include "node.h"
#include "codegen.h"
#include "parser.hpp"

using namespace std;

static llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* pFunction, llvm::Type* tpy, llvm::StringRef VarName)
{
	llvm::IRBuilder<> TmpB(&pFunction->getEntryBlock(), pFunction->getEntryBlock().begin()) ;
	return TmpB.CreateAlloca(tpy, nullptr, VarName);
}

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root)
{
	std::cout << "Generating code...\n";
	
	/* Create the top level interpreter function to call as entry */
	vector<llvm::Type*> argTypes;
	llvm::FunctionType *ftype = llvm::FunctionType::get(llvm::Type::getVoidTy(*MyContext), llvm::makeArrayRef(argTypes), false);
	mainFunction = llvm::Function::Create(ftype, llvm::GlobalValue::InternalLinkage, "main", module);
	llvm::BasicBlock *bblock = llvm::BasicBlock::Create(*MyContext, "entry", mainFunction, 0);
	
	/* Push a new variable/block context */
	pushBlock(bblock);
	root.codeGen(*this); /* emit bytecode for the toplevel block */
	llvm::ReturnInst::Create(*MyContext, bblock);
	popBlock();
	
	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	std::cout << "Code is generated.\n";
	// module->dump();

	llvm::legacy::PassManager pm;
	// TODO:
	pm.add(llvm::createPrintModulePass(llvm::outs()));
	pm.run(*module);
}

/* Executes the AST by running the main function */
int CodeGenContext::runCode() {
	std::cout << "Running code..." << std::endl ;

	std::string error;

	std::cout << "Constructing EngineBuilder..." << std::endl ;

	llvm::ExecutionEngine *ee = llvm::EngineBuilder( unique_ptr<llvm::Module>(module) )
							.setErrorStr(&error)
							.setEngineKind(llvm::EngineKind::Interpreter)
							.create();

	if (!ee) {
        std::cerr << "Execution Engine error: " << error << '\n';
        return 1;
    }
		
	std::cout << "Verifying module..." << std::endl ;

	llvm::raw_string_ostream error_os(error);
    if (llvm::verifyModule(*module, &error_os)) {
        std::cerr << "Module Error: [" << std::endl;
        std::cerr << error << std::endl ;
        std::cerr << "]" << std::endl ;
        return 1;
    }

	std::cout << "Finalizing object..." << std::endl ;
	ee->finalizeObject();
	vector<llvm::GenericValue> noargs;

	std::cout << "mainFunction: " << mainFunction << std::endl ;
	llvm::GenericValue v = ee->runFunction(mainFunction, noargs);
	
//	std::ios::sync_with_stdio(true);
//	std::cout << "Code was run." << std::endl ;
	return 0;
}

/* Returns an LLVM type based on the identifier */
static llvm::Type *typeOf(const NIdentifier& type) 
{
	if (type.name.compare("int") == 0) {
		return llvm::Type::getInt64Ty(*MyContext);
	}
	else if (type.name.compare("double") == 0) {
		return llvm::Type::getDoubleTy(*MyContext);
	}
	return llvm::Type::getVoidTy(*MyContext);
}

/* -- Code Generation -- */

llvm::Value* NInteger::codeGen(CodeGenContext& context)
{
	std::cout << "Creating integer: " << value << endl;
	return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*MyContext), value, true);
}

llvm::Value* NDouble::codeGen(CodeGenContext& context)
{
	std::cout << "Creating double: " << value << endl;
	return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*MyContext), value);
}

llvm::Value* NIdentifier::codeGen(CodeGenContext& context)
{
	std::cout << "Creating identifier reference: " << name << endl;
	if (context.locals().find(name) == context.locals().end()) {
		std::cerr << "undeclared variable " << name << endl;
		return NULL;
	}

	// return nullptr;
	llvm::IRBuilder<> TmpB(context.currentBlock()) ;
	return TmpB.CreateLoad(((llvm::AllocaInst*)(context.locals()[name]))->getAllocatedType(), context.locals()[name], name);
/*	return new llvm::LoadInst(	context.locals()[name]->getType(),
								context.locals()[name],
								name,
								false,
								context.currentBlock()	);*/
}

llvm::Value* NMethodCall::codeGen(CodeGenContext& context)
{
	llvm::Function *function = context.module->getFunction(id.name.c_str());
	if (function == NULL) {
		std::cerr << "no such function " << id.name << endl;
	}
	std::vector<llvm::Value*> args;
	ExpressionList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		args.push_back((**it).codeGen(context));
	}
	llvm::CallInst *call = llvm::CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());
	std::cout << "Creating method call: " << id.name << endl;
	return call;
}

llvm::Value* NBinaryOperator::codeGen(CodeGenContext& context)
{
	std::cout << "Creating binary operation " << op << endl;
	
	llvm::Value* L = lhs.codeGen(context) ;
	llvm::Value* R = rhs.codeGen(context) ;

	llvm::IRBuilder<> TmpB(context.currentBlock()) ;

	switch (op) {
		case TPLUS:
			return TmpB.CreateAdd(L,R, "addtmp");

		case TMINUS:
			return TmpB.CreateSub(L,R, "subtmp");

		case TMUL:
			return TmpB.CreateMul(L,R, "multmp");

		case TDIV:
			return TmpB.CreateSDiv(L,R, "sdivtmp");
				
		/* TODO comparison */
	}

	return NULL ;
}

llvm::Value* NAssignment::codeGen(CodeGenContext& context)
{
	std::cout << "Creating assignment for " << lhs.name << endl;
	if (context.locals().find(lhs.name) == context.locals().end()) {
		std::cerr << "undeclared variable " << lhs.name << endl;
		return NULL;
	}
	
	llvm::IRBuilder<> TmpB(context.currentBlock()) ;
	return TmpB.CreateStore(rhs.codeGen(context), context.locals()[lhs.name]);
	/*return new llvm::StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());*/
}

llvm::Value* NBlock::codeGen(CodeGenContext& context)
{
	StatementList::const_iterator it;
	llvm::Value *last = NULL;
	for (it = statements.begin(); it != statements.end(); it++) {
		std::cout << "Generating code for " << typeid(**it).name() << endl;
		last = (**it).codeGen(context);
	}
	std::cout << "Creating block" << endl;
	return last;
}

llvm::Value* NExpressionStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating code for " << typeid(expression).name() << endl;
	return expression.codeGen(context);
}

llvm::Value* NReturnStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating return code for " << typeid(expression).name() << endl;
	llvm::Value *returnValue = expression.codeGen(context);
	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

llvm::Value* NVariableDeclaration::codeGen(CodeGenContext& context)
{
	std::cout << "Creating variable declaration " << type.name << " " << id.name << endl;

	/* llvm::AllocaInst *alloc = new llvm::AllocaInst(typeOf(type), 4, id.name.c_str(), context.currentBlock()); */
	llvm::AllocaInst *alloc = CreateEntryBlockAlloca(context.currentBlock()->getParent(), typeOf(type), id.name);
	context.locals()[id.name] = alloc ;
	if (assignmentExpr != NULL) {
		NAssignment assn(id, *assignmentExpr);
		assn.codeGen(context);
	}
	return alloc;
}

llvm::Value* NExternDeclaration::codeGen(CodeGenContext& context)
{
    vector<llvm::Type*> argTypes;
    VariableList::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); it++) {
        argTypes.push_back(typeOf((**it).type));
    }
    llvm::FunctionType *ftype = llvm::FunctionType::get(typeOf(type), llvm::makeArrayRef(argTypes), false);
    llvm::Function *function = llvm::Function::Create(ftype, llvm::GlobalValue::ExternalLinkage, id.name.c_str(), context.module);
    return function;
}

llvm::Value* NFunctionDeclaration::codeGen(CodeGenContext& context)
{
	vector<llvm::Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		argTypes.push_back(typeOf((**it).type));
	}
	llvm::FunctionType *ftype = llvm::FunctionType::get(typeOf(type), llvm::makeArrayRef(argTypes), false);
	llvm::Function *function = llvm::Function::Create(ftype, llvm::GlobalValue::InternalLinkage, id.name.c_str(), context.module);
	llvm::BasicBlock *bblock = llvm::BasicBlock::Create(*MyContext, "entry", function, 0);
	context.pushBlock(bblock);

	llvm::Function::arg_iterator argsValues = function->arg_begin();
    llvm::Value* argumentValue;

	llvm::IRBuilder<> TmpB(&function->getEntryBlock(), function->getEntryBlock().begin()) ;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		(**it).codeGen(context);
		
		argumentValue = &*argsValues++;
		argumentValue->setName((*it)->id.name.c_str());
		llvm::StoreInst *inst = TmpB.CreateStore(argumentValue, context.locals()[(*it)->id.name]);
		/*llvm::StoreInst *inst = new llvm::StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);*/
	}
	
	block.codeGen(context);
	llvm::ReturnInst::Create(*MyContext, context.getCurrentReturnValue(), bblock);

	context.popBlock();
	std::cout << "Creating function: " << id.name << std::endl;

	std::cout << "Verifying function: " << id.name << std::endl;
	// Validate the generated code, checking for consistency.
	// TODO: check return code
	bool success = llvm::verifyFunction(*function);

	// Optimize the function.
	std::cout << "Optimizing function: " << id.name << std::endl;
  	MyFPM->run(*function);

	return function;
}
