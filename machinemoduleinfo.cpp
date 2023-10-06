#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/MCParser/MCTargetAsmParser.h>
#include <llvm/MC/MCParser/MCAsmParser.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/CodeGen/MachineModuleInfo.h>

using namespace llvm;

int main(int argc, char **argv) {
    if (argc < 2) {
        errs() << "Usage: " << argv[0] << " <assembly file>\n";
        return 1;
    }

    // Initialize all targets that LLVM knows about.
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllAsmParsers();

    // Setup
    std::string Error;
    SMDiagnostic SMError;
    auto TripleStr = sys::getDefaultTargetTriple();
    auto Triple = Triple(TripleStr);
    const Target *TheTarget = TargetRegistry::lookupTarget(TripleStr, Error);

    if (!TheTarget) {
        errs() << Error;
        return 1;
    }

    std::unique_ptr<MemoryBuffer> Buffer = MemoryBuffer::getFileOrSTDIN(argv[1], Error);
    if (!Buffer) {
        errs() << Error;
        return 1;
    }

    SourceMgr SrcMgr;
    SrcMgr.AddNewSourceBuffer(std::move(Buffer), SMLoc());

    std::unique_ptr<MCRegisterInfo> MRI(TheTarget->createMCRegInfo(TripleStr));
    std::unique_ptr<MCAsmInfo> MAI(TheTarget->createMCAsmInfo(*MRI, TripleStr));
    MCObjectFileInfo MOFI;
    MCContext Ctx(MAI.get(), MRI.get(), &MOFI);
    MOFI.InitMCObjectFileInfo(Triple, false, Ctx);
    std::unique_ptr<MCInstrInfo> MII(TheTarget->createMCInstrInfo());

    // Setup the assembly parser
    std::unique_ptr<MCSubtargetInfo> STI(TheTarget->createMCSubtargetInfo(TripleStr, "", ""));
    std::unique_ptr<MCTargetAsmParser> TAP(TheTarget->createMCAsmParser(*STI, *MII, *MRI, Ctx));
    MCAsmParser *Parser = createMCAsmParser(SrcMgr, Ctx, *TAP, *MAI);

    // Parsing assembly into a MachineModuleInfo representation
    MachineModuleInfo MMI(TheTarget);
    if (Parser->Run(false)) {
        errs() << "Error parsing assembly\n";
        return 1;
    }

    // From here, you can explore the MachineModuleInfo, which represents the assembly as machine instructions.
    // ... do something with MMI ...

    return 0;
}
