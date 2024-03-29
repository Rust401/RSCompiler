#include <iostream>
#include <fstream>
#include "ASTNodes.h"
#include "CodeGen.h"
#include "ObjGen.h"

extern shared_ptr<NBlock> programBlock;
extern int yyparse();
int main(int argc, char **argv) {
    //Use the token stream to build a AST whose root is programBlock
    yyparse();
    
    #ifdef PRINT_AND_JOSONGEN
        programBlock->print("--");
        auto root = programBlock->jsonGen();
    #endif
    
    //innitial the llvm context
    CodeGenContext context;
    //Use the root Node of the AST to do the code generation
    context.generateCode(*programBlock);
    //Output the target
    ObjGen(context);

#ifdef PRINT_AND_JOSONGEN
    std::string outPutJsonFile = "visual/Tree.json";
    std::ofstream astJson(outPutJsonFile);
    if( astJson.is_open() ){
        astJson << root;
        astJson.close();
        std::cout << "json file output" << outPutJsonFile << std::endl;
    }
#endif

    return 0;
}

