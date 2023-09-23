 #include <iostream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Vectorize.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"

#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/TargetInfo.h>
#include <vector>


std::unique_ptr<llvm::Module> c2ir( char * filename,
                                    llvm::LLVMContext& llvm_ctx ) {

  std::vector<const char *> args;
  args.push_back( "-emit-llvm" );
  // args.push_back( "-disable-llvm-passes" );
  // args.push_back( "-debug-info-kind=standalone" );
  // args.push_back( "-dwarf-version=2" );
  //args.push_back( "-dwarf-column-info" );
  //args.push_back( "-mdisable-fp-elim");
  args.push_back( "-femit-all-decls" );
  args.push_back( "-O2" );
  args.push_back( "-disable-O0-optnone" );
  args.push_back( filename );

  clang::CompilerInstance Clang;
  Clang.createDiagnostics();

  std::shared_ptr<clang::CompilerInvocation> CI(new clang::CompilerInvocation());
  // Use ArrayRef constructor to create ArrayRef<const char *>
    llvm::ArrayRef<const char *> argsRef(args.data(), args.size());
    
  clang::CompilerInvocation::CreateFromArgs( *CI.get(), argsRef,
                                            Clang.getDiagnostics());
  Clang.setInvocation(CI);
  clang::CodeGenAction *Act = new clang::EmitLLVMOnlyAction(&llvm_ctx);

  if (!Clang.ExecuteAction(*Act))
    return nullptr;

  std::unique_ptr<llvm::Module> module = Act->takeModule();

  return std::move(module);
}




std::unique_ptr<llvm::Module> generateAssemblyModule (llvm::Module& module, llvm:: LLVMContext& llvm_ctx) 
{ 
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    // Get the target machine for the ARM64 architecture.
    llvm:: Triple triple("aarch64");
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget (triple.str(), error);
    if (!target) {
        llvm::outs() << "Error looking up target: " << error << "\n"; 
        return nullptr;
    }

// Set target-specific options.

    llvm::TargetOptions options;
    llvm::Reloc::Model relocModel = llvm::Reloc::Model::PIC_;
    llvm::CodeModel::Model codeModel = llvm::CodeModel::Small;
    llvm::CodeGenOpt:: Level optLevel = llvm::CodeGenOpt::Aggressive;


// Create a target machine.
    llvm::TargetMachine* targetMachine = target->createTargetMachine(triple.str(),"","",options, relocModel, codeModel, optLevel);

// Set up the output stream. 
    std::string assembly;
    llvm::raw_string_ostream output (assembly);

// Generate assembly code.
    llvm:: legacy::PassManager passManager;
    targetMachine->addPassesToEmitFile(passManager, llvm::outs(), nullptr,
    llvm::CGFT_AssemblyFile);
    passManager.run(module);
    
    // Create a new function within the module.
llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx), false);
llvm::Function* assemblyFunction = llvm::Function::Create(funcType, llvm::Function::InternalLinkage, "embeddedAssemblyFunction", &module);
llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(llvm_ctx, "entry", assemblyFunction);

// Add the assembly as inline assembly within the function.
llvm::IRBuilder<> builder(entryBlock);
llvm::InlineAsm* inlineAsm = llvm::InlineAsm::get(funcType, assembly, "", true, false);
builder.CreateCall(inlineAsm);

// Set the module's target triple and data layout based on the original module.
module.setTargetTriple("aarch64");
module.setDataLayout("e-m:e-i64:64-i128:128-n32:64-S128");

return std::make_unique<llvm::Module>(module);


}



int main(int argc, char** argv) {
  if( argc != 2 ) {
    return 1;
  }
  char* cpp_file = argv[1];

  llvm::LLVMContext llvm_ctx;

  std::unique_ptr<llvm::Module> m = c2ir( cpp_file, llvm_ctx );
  std::unique_ptr<llvm::Module> arm64 = generateAssemblyModule( *m,llvm_ctx );
   m->dump();
return 0;

}