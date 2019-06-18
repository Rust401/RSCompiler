#ifndef CODEGEN_H
#define CODEGEN_H

//#define PRINT_SYMBOL_TABLE

#include <llvm/IR/Value.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <json/json.h>

#include <stack>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include "ASTNodes.h"
#include "grammar.hpp"
#include "TypeSystem.h"

using namespace llvm;
using std::unique_ptr;
using std::string;

using SymTable = std::map<std::string, Value*>;

class CodeGenBlock{
public:
    BasicBlock * block;
    Value * returnValue;
    std::map<std::string, Value*> locals;
    std::map<std::string, shared_ptr<NIdentifier>> types;     
    std::map<std::string, bool> isFuncArg;
    std::map<std::string, std::vector<uint64_t>> arraySizes;
};

class CodeGenContext{
private:
    std::vector<CodeGenBlock*> theBlockStack;
public:
    LLVMContext llvmContext;
    IRBuilder<> builder;
    unique_ptr<Module> theModule;
    SymTable globalVars;
    TypeSystem typeSystem;

    CodeGenContext(): builder(llvmContext), typeSystem(llvmContext){
        theModule = unique_ptr<Module>(new Module("main", this->llvmContext));
    }

    Value* getSymbolValue(std::string name) const{
        for(auto it=theBlockStack.rbegin(); it!=theBlockStack.rend(); it++){
            if( (*it)->locals.find(name) != (*it)->locals.end() ){
                return (*it)->locals[name];
            }
        }
        return nullptr;
    }

    shared_ptr<NIdentifier> getSymbolType(std::string name) const{
        for(auto it=theBlockStack.rbegin(); it!=theBlockStack.rend(); it++){
            if( (*it)->types.find(name) != (*it)->types.end() ){
                return (*it)->types[name];
            }
        }
        return nullptr;
    }

    bool isFuncArg(std::string name) const{

        for(auto it=theBlockStack.rbegin(); it!=theBlockStack.rend(); it++){
            if( (*it)->isFuncArg.find(name) != (*it)->isFuncArg.end() ){
                return (*it)->isFuncArg[name];
            }
        }
        return false;
    }

    void setSymbolValue(std::string name, Value* value){
        theBlockStack.back()->locals[name] = value;
    }

    void setSymbolType(std::string name, shared_ptr<NIdentifier> value){
        theBlockStack.back()->types[name] = value;
    }

    void setFuncArg(std::string name, bool value){
        //std::cout << "Set " << name << " as func arg" << std::endl;
        theBlockStack.back()->isFuncArg[name] = value;
    }

    BasicBlock* currentBlock() const{
        return theBlockStack.back()->block;
    }

    void pushBlock(BasicBlock * block){
        CodeGenBlock * codeGenBlock = new CodeGenBlock();
        codeGenBlock->block = block;
        codeGenBlock->returnValue = nullptr;
        theBlockStack.push_back(codeGenBlock);
    }

    void popBlock(){
        CodeGenBlock * codeGenBlock = theBlockStack.back();
        theBlockStack.pop_back();
        delete codeGenBlock;
    }

    void setCurrentReturnValue(Value* value){
        theBlockStack.back()->returnValue = value;
    }

    Value* getCurrentReturnValue(){
        return theBlockStack.back()->returnValue;
    }

    void setArraySize(std::string name, std::vector<uint64_t> value){
        //std::cout << "setArraySize: " << name << ": " << value.size() << std::endl;
        theBlockStack.back()->arraySizes[name] = value;
    }

    std::vector<uint64_t> getArraySize(std::string name){
        for(auto it=theBlockStack.rbegin(); it!=theBlockStack.rend(); it++){
            if( (*it)->arraySizes.find(name) != (*it)->arraySizes.end() ){
                return (*it)->arraySizes[name];
            }
        }
        return theBlockStack.back()->arraySizes[name];
    }

    void PrintSymTable() const{
    #ifdef PRINT_SYMBOL_TABLE
        std::cout << "======= Print Symbol Table ==================" << std::endl;
        std::string prefix = "";
        for(auto it=theBlockStack.begin(); it!=theBlockStack.end(); it++){
            for(auto it2=(*it)->locals.begin(); it2!=(*it)->locals.end(); it2++){
                std::cout << prefix << it2->first << " = " << it2->second << ": " << this->getSymbolType(it2->first) << std::endl;
            }
            prefix += "\t";
        }
        std::cout << "=============================================" << std::endl;
    #endif
    }

    void generateCode(NBlock& );
};

Value* LogErrorV(const char* err);
Value* LogErrorV(std::string err);

#endif //CODEGEN_H