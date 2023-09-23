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
// #include "llvm/Target/TargetRegistry.h"

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
  args.push_back( "-disable-llvm-passes" );
  args.push_back( "-debug-info-kind=standalone" );
  args.push_back( "-dwarf-version=2" );
  //args.push_back( "-dwarf-column-info" );
  //args.push_back( "-mdisable-fp-elim");
  args.push_back( "-femit-all-decls" );
  args.push_back( "-O1" );
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
std::unique_ptr<llvm::Module> generatearm64(llvm::Module& inputModule) {
    // Initialize LLVM components
      llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    // Setup target-specific information
    llvm::Triple TheTriple("aarch64");
    std::string Error;
    const llvm::Target *TheTarget = llvm::TargetRegistry::lookupTarget(TheTriple.str(), Error);
    if (!TheTarget) {
        llvm::errs() << "Error: " << Error;
        return nullptr;
    }

    // Create a target machine
    llvm::TargetOptions Options;
    llvm::Reloc:: Model relocModel= llvm::Reloc:: Model:: PIC_;
    llvm::CodeModel:: Model codeModel =llvm::CodeModel::Small;
    // llvm::CodeGenOpt::Level optlevel = llvm ::CodeGenOpt ::Agressive;

    llvm:: Targetmachine* targetMachine=TheTarget->createTargetMachine(
        TheTriple.str(), "", "", Options);
    if (!targetMachine) {
        llvm::errs() << "Error: failed to create target machine\n";
        return nullptr;
    }

    // Create a new module to store the ARM64 assembly code
    llvm::LLVMContext &Context = inputModule.getContext();
    std::unique_ptr<llvm::Module> ARM64Module = std::make_unique<llvm::Module>("arm64_module", Context);

    // Pass input module through target-specific code generation pipeline
    llvm::legacy::PassManager PM;
    llvm::PassManagerBuilder PMBuilder;
    PMBuilder.OptLevel = 2;
    targetMachine->adjustPassManager(PMBuilder);

    PMBuilder.populateModulePassManager(PM);

    llvm::SmallVector<char, 0> ARM64Assembly;
    llvm::raw_svector_ostream OS(ARM64Assembly);

    if (targetMachine->addPassesToEmitFile(PM, OS, nullptr, llvm::CGFT_AssemblyFile)) {
        llvm::errs() << "Error: TargetMachine can't emit assembly for ARM64!\n";
        return nullptr;
    }

    PM.run(inputModule);

    // Create a global constant in the ARM64 module to store the assembly code
    llvm::Constant *ARM64AssemblyConstant = llvm::ConstantDataArray::getString(Context, OS.str());
    llvm::GlobalVariable *ARM64AssemblyGlobal = new llvm::GlobalVariable(
        *ARM64Module, ARM64AssemblyConstant->getType(), true, llvm::GlobalValue::PrivateLinkage,
        ARM64AssemblyConstant);

    // Set the initializer for the global constant
    ARM64AssemblyGlobal->setInitializer(ARM64AssemblyConstant);

    return ARM64Module;
}


int main(int argc, char** argv) {
  if( argc != 2 ) {
    return 1;
  }
  char* cpp_file = argv[1];

  llvm::LLVMContext llvm_ctx;

  std::unique_ptr<llvm::Module> m = c2ir( cpp_file, llvm_ctx );
  std::unique_ptr<llvm::Module> arm64 = generatearm64( *m );
  arm64->dump();


}


