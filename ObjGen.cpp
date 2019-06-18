#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/ADT/Optional.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include "CodeGen.h"
#include "ObjGen.h"

using namespace llvm;


void doInit(){
    // The function here is just a routine of the llvm
    // Initialize the target registry etc.
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();
}

void ObjGen(CodeGenContext & context, const string& filename){
    doInit();
    auto targetTriple = sys::getDefaultTargetTriple();
    context.theModule->setTargetTriple(targetTriple);

    std::string error;
    auto Target = TargetRegistry::lookupTarget(targetTriple, error);

    if( !Target ){
        errs() << error;
        return;
    }

    TargetOptions tOptions;
    auto RM = Optional<Reloc::Model>();

    const char* features = "";
    const char* CPU = "generic";

    llvm::TargetMachine* theTargetMachine = Target->createTargetMachine(targetTriple, CPU, features, tOptions, RM);

    context.theModule->setDataLayout(theTargetMachine->createDataLayout());
    context.theModule->setTargetTriple(targetTriple);

    std::error_code ErrorCode;
    raw_fd_ostream dest(filename.c_str(), ErrorCode, sys::fs::F_None);

    legacy::PassManager pass;
    auto fileType = TargetMachine::CGFT_ObjectFile;

    if( theTargetMachine->addPassesToEmitFile(pass, dest, fileType) ){
        errs() << "This Type can't be emited";
        return;
    }
    pass.run(*context.theModule.get());
    dest.flush();

    outs() << "Write OBJ code to : " << filename.c_str() << "\n";

    return;
}

