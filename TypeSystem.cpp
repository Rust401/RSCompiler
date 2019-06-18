#include "TypeSystem.h"
#include "CodeGen.h"

string TypeSystem::llvmTypeToStr(Type *value) {
    Type::TypeID typeID;
    if( value )
        typeID = value->getTypeID();
    else
        return "Value is nullptr";

    switch (typeID){
        case Type::IntegerTyID:
            return "IntegerTyID";
        case Type::FunctionTyID:
            return "FunctionTyID";
        case Type::StructTyID:
            return "StructTyID";
        case Type::ArrayTyID:
            return "ArrayTyID";
        case Type::PointerTyID:
            return "PointerTyID";
        case Type::VectorTyID:
            return "VectorTyID";
        case Type::VoidTyID:
            return "VoidTyID";
        case Type::HalfTyID:
            return "HalfTyID";
        case Type::FloatTyID:
            return "FloatTyID";
        case Type::DoubleTyID:
            return "DoubleTyID";
        default:
            return "Unknown";
    }
}


string TypeSystem::llvmTypeToStr(Value *value) {
    if( value )
        return llvmTypeToStr(value->getType());
    else
        return "Value is nullptr";
}

TypeSystem::TypeSystem(LLVMContext &context): llvmContext(context){
    addCast(floatTy, doubleTy, llvm::CastInst::FPExt);
    addCast(floatTy, intTy, llvm::CastInst::FPToSI);
    addCast(doubleTy, intTy, llvm::CastInst::FPToSI);
    addCast(intTy, intTy, llvm::CastInst::SExt);
    addCast(intTy, floatTy, llvm::CastInst::SIToFP);
    addCast(intTy, doubleTy, llvm::CastInst::SIToFP);
    addCast(boolTy, doubleTy, llvm::CastInst::SIToFP);
}

void TypeSystem::addStructMember(string structName, string memType, string memName) {
    if( this->structTypes.find(structName) == this->structTypes.end() ){
        LogError("Unknown struct name");
    }
    this->structMembers[structName].push_back(std::make_pair(memType, memName));
}

void TypeSystem::addStructType(string name, llvm::StructType *type) {
    this->structTypes[name] = type;
    this->structMembers[name] = std::vector<TypeNamePair>();
}

Type *TypeSystem::getVarType(const NIdentifier& type) {
    assert(type.isType);
    if( type.isArray ){  
        return PointerType::get(getVarType(type.name), 0);
    }

    return getVarType(type.name);

    return 0;
}



Value* TypeSystem::getDefaultValue(string typeStr, LLVMContext &context) {
    Type* type = this->getVarType(typeStr);
    if( type == this->intTy ){
        return ConstantInt::get(type, 0, true);
    }else if( type == this->doubleTy || type == this->floatTy ){
        return ConstantFP::get(type, 0);
    }
    return nullptr;
}

//add the valid cast in the context
void TypeSystem::addCast(Type *from, Type *to, CastInst::CastOps op) {
    if( castTable.find(from) == castTable.end() ){
        castTable[from] = std::map<Type*, CastInst::CastOps>();
    }
    castTable[from][to] = op;
}

//Cast the type of a value in the current block
Value* TypeSystem::cast(Value *value, Type *type, BasicBlock *block) {
    Type* from = value->getType();
    if( from == type )
        return value;
    if( castTable.find(from) == castTable.end() ){
        LogError("Type has no cast");
        return value;
    }
    //find the type to be cast in the second map get from the first map
    if( castTable[from].find(type) == castTable[from].end() ){
        //Error handle
        string error = "Unable to cast from ";
        error += llvmTypeToStr(from) + " to " + llvmTypeToStr(type);
        LogError(error.c_str());
        return value;
    }

    //The real transfer
    //Insert a instruction to tranfer type in the positon it should be
    return CastInst::Create(castTable[from][type], value, type, "cast", block);
}

bool TypeSystem::isStruct(string typeStr) const {
    return this->structTypes.find(typeStr) != this->structTypes.end();
}

int32_t TypeSystem::getStructMemberIndex(string structName, string memberName) {
    if( this->structTypes.find(structName) == this->structTypes.end() ){
        LogError("Unknown struct name");
        return 0;
    }
    auto& members = this->structMembers[structName];
    for(auto it=members.begin(); it!=members.end(); it++){
        if( it->second == memberName ){
            return std::distance(members.begin(), it);
        }
    }

    LogError("Unknown struct member");

    return 0;
}

Type *TypeSystem::getVarType(string typeStr) {
    if( typeStr.compare("bool") == 0 ){
        return this->boolTy;
    }
    if( typeStr.compare("char") == 0 ){
        return this->charTy;
    }
    if( typeStr.compare("void") == 0 ){
        return this->voidTy;
    }
    if( typeStr.compare("int") == 0 ){
        return this->intTy;
    }
    if( typeStr.compare("float") == 0 ){
        return this->floatTy;
    }
    if( typeStr.compare("double") == 0 ){
        return this->doubleTy;
    }
    if( typeStr.compare("string") == 0 ){
        return this->stringTy;
    }

    if( this->structTypes.find(typeStr) != this->structTypes.end() )
        return this->structTypes[typeStr];

    return nullptr;
}

