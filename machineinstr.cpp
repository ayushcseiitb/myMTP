struct MachineInstruction {
    std::string opcodeName;
    std::vector<std::string> operands;
};

struct MachineFunction {
    std::string name;
    std::vector<MachineInstruction> instructions;
};

struct MachineModule {
    std::vector<MachineFunction> functions;
};


MachineModule convertIRToMachineModule(Module &M, TargetMachine &TM) {
    MachineModule machineModule;

    // Generate machine code for the module.
    legacy::PassManager PM;
    MachineModuleInfo *MMI = new MachineModuleInfo(&TM);
    PM.add(MMI);
    TM.addPassesToEmitFile(PM, errs(), nullptr, CGFT_Null, true);
    PM.run(M);

    for (auto &F : M) {
        MachineFunction machineFunction;
        machineFunction.name = F.getName().str();

        if (auto *MF = MMI->getMachineFunction(F)) {
            for (auto &MBB : *MF) {
                for (auto &MI : MBB) {
                    MachineInstruction instruction;
                    instruction.opcodeName = MI.getOpcodeName();
                    for (unsigned i = 0; i < MI.getNumOperands(); i++) {
                        MachineOperand &MO = MI.getOperand(i);
                        if (MO.isReg()) {
                            instruction.operands.push_back(TargetRegistry::getRegClassName(MO.getReg()));
                        } else if (MO.isImm()) {
                            instruction.operands.push_back(std::to_string(MO.getImm()));
                        }
                        // Handle other operand types as needed...
                    }
                    machineFunction.instructions.push_back(instruction);
                }
            }
        }

        machineModule.functions.push_back(machineFunction);
    }

    return machineModule;
}




 std::unique_ptr<TargetMachine> TM = ...;

    // Convert IR to custom machine module.
    MachineModule machineMod = convertIRToMachineModule(*Mod, *TM);
