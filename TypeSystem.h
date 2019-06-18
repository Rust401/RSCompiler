#ifndef TYPESYSTEM_H
#define TYPESYSTEM_H

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <stdint.h>

#include "ASTNodes.h"


using std::string;
using namespace llvm;

#define TypeNamePair std::pair<std::string,std::string>
#define ENABLE 1
#define DISABLE 2

class TypeSystem{
private:
    LLVMContext& llvmContext;

    std::map<std::string, std::vector<TypeNamePair>> structMembers;

    std::map<std::string, llvm::StructType*> structTypes;

    std::map<Type*, std::map<Type*, CastInst::CastOps>> castTable;

    void addCast(Type* from, Type* to, CastInst::CastOps op);

    bool flag=0;
    uint8_t state=ENABLE;

public:
    Type* intTy = Type::getInt32Ty(llvmContext);
    Type* charTy = Type::getInt8Ty(llvmContext);
    Type* floatTy = Type::getFloatTy(llvmContext);
    Type* doubleTy = Type::getDoubleTy(llvmContext);
    Type* stringTy = Type::getInt8PtrTy(llvmContext);
    Type* voidTy = Type::getVoidTy(llvmContext);
    Type* boolTy = Type::getInt1Ty(llvmContext);


    TypeSystem(LLVMContext& context);

    void addStructType(string structName, llvm::StructType*);
    void addStructMember(string structName, string memType, string memName);

    int32_t getStructMemberIndex(string structName, string memberName);

    Type* getVarType(const NIdentifier& type) ;
    Type* getVarType(string typeStr) ;

    Value* getDefaultValue(string typeStr, LLVMContext &context) ;
    Value* cast(Value* value, Type* type, BasicBlock* block) ;

    bool isStruct(string typeStr) const;

    static string llvmTypeToStr(Value* value) ;
    static string llvmTypeToStr(Type* type) ;
};


#endif //TYPESYSTEM_H
