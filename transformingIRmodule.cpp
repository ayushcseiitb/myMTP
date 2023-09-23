#include <iostream>
#include<fstream>
#include<sstream>
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




void generateAssemblyInModule(llvm::Module& module, llvm::LLVMContext& llvm_ctx)
{
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    // Get the target machine for the ARM64 architecture.
    llvm::Triple triple("aarch64");
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple.str(), error);
    if (!target) {
        llvm::outs() << "Error looking up target: " << error << "\n";
        return;
    }

    // Set target-specific options.
    llvm::TargetOptions options;
    llvm::Reloc::Model relocModel = llvm::Reloc::Model::PIC_;
    llvm::CodeModel::Model codeModel = llvm::CodeModel::Small;
    llvm::CodeGenOpt::Level optLevel = llvm::CodeGenOpt::Aggressive;

    // Create a target machine.
    llvm::TargetMachine* targetMachine = target->createTargetMachine(triple.str(), "", "", options, relocModel, codeModel, optLevel);

    // Set up the output stream.
    std::error_code EC;
    llvm::SmallVector<char, 256> asmBuffer;
    llvm::raw_svector_ostream asmStream(asmBuffer);

    // Generate assembly code.
    llvm::legacy::PassManager passManager;
    targetMachine->addPassesToEmitFile(passManager, asmStream, nullptr, llvm::CGFT_AssemblyFile);
    passManager.run(module);

    // Convert the assembly code to a string.
    std::string assembly(asmBuffer.begin(), asmBuffer.end());

    // Remove or replace the existing global variable named "@assembly."
    llvm::GlobalVariable* existingAssembly = module.getGlobalVariable("@assembly", true);
    if (existingAssembly) {
        existingAssembly->eraseFromParent();
    }

    // Add the assembly code as the text of the module.
    llvm::Constant* assemblyConstant = llvm::ConstantDataArray::getString(llvm_ctx, assembly);
    llvm::GlobalVariable* AssemblyGlobal = new llvm::GlobalVariable(
        module, assemblyConstant->getType(), true,
        llvm::GlobalValue::InternalLinkage, assemblyConstant, "assembly");

    // Set the module's target triple and data layout.
    module.setTargetTriple("aarch64");
    module.setDataLayout("e-m:e-i64:64-i128:128-n32:64-S128");
}

void printInstructions(llvm::Module& module) {
    for (llvm::Function& function : module) {
        std::cout << "Function: " << function.getName().str() << "\n";
        
        for (llvm::BasicBlock& basicBlock : function) {
            std::cout << "Basic Block: " << basicBlock.getName().str() << "\n";
            
            for (llvm::Instruction& instruction : basicBlock) {
                // Print the instruction.
                llvm::outs() << instruction << "\n";
            }
        }
    }
}



int main(int argc, char** argv) {
  if( argc != 2 ) {
    return 1;
  }
  char* cpp_file = argv[1];

  llvm::LLVMContext llvm_ctx;

  std::unique_ptr<llvm::Module> m = c2ir( cpp_file, llvm_ctx );
  generateAssemblyInModule( *m,llvm_ctx );
  // m->dump();
  printInstructions(*m);
return 0;

}