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
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/LegacyPassManagers.h"

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




using namespace llvm;

std::unique_ptr<Module> compileIRToARM64Assembly(Module& InputModule) {
    // Initialize LLVM components.
    LLVMContext Context;
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();

    // Create a target machine for ARM64.
    Triple TheTriple("aarch64-unknown-linux-gnu");
    std::string Error;
    const Target* TheTarget = TargetRegistry::lookupTarget(TheTriple.getTriple(), Error);
    if (!TheTarget) {
        errs() << "Error: " << Error << "\n";
        return nullptr;
    }

    TargetOptions Options;
    Options.MCOptions.AsmVerbose = true; // Enable verbose assembly output

    CodeGenOpt::Level OptLevel = CodeGenOpt::Default;
    TargetMachine* TM = TheTarget->createTargetMachine(TheTriple.getTriple(), "generic", "", Options, None, None, OptLevel);

    // Create a pass manager and target information.
    PassManager PM;
    TargetLibraryInfo TLII(TheTriple);
    PM.add(new TargetLibraryInfoWrapperPass(TLII));

    // Add passes to the pass manager for code generation.
    TM->adjustPassManager(PM); // Use adjustPassManager instead of addAnalysisPasses
    PM.add(createPrintModulePass(outs())); // Print the assembly to stdout

    // Run the pass manager to generate the assembly.
    PM.run(InputModule);

    // Set the data layout on the input module.
    InputModule.setDataLayout(TM->createDataLayout());

    // The assembly code is printed to stdout using createPrintModulePass.
    // You can capture it if needed.

    return nullptr; // No need to return a modified module since we printed to stdout.
}




int main(int argc, char** argv) {
  if( argc != 2 ) {
    return 1;
  }
  char* cpp_file = argv[1];

  llvm::LLVMContext llvm_ctx;

  std::unique_ptr<llvm::Module> m = c2ir( cpp_file, llvm_ctx );
  std::unique_ptr<llvm::Module> arm64 = compileIRToARM64Assembly( *m );
  //  m->dump();
return 0;

}