#ifndef ASTNODES_H
#define ASTNODES_H
//#define PRINT_AND_JOSONGEN
//#define PRINT_VALID_NODE_NUM
#include <llvm/IR/Value.h>
#include <json/json.h>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <stdint.h>

class CodeGenContext;
class NBlock;
class NStatement;
class NExpression;
class NVariableDeclaration;

using std::cout;
using std::endl;
using std::string;
using std::shared_ptr;
using std::make_shared;

typedef std::vector<shared_ptr<NStatement>> StatementList;
typedef std::vector<shared_ptr<NExpression>> ExpressionList;
typedef std::vector<shared_ptr<NVariableDeclaration>> VariableList;

static uint64_t nodeCount=0;
static uint64_t intCount=0;
static uint64_t doubleCount=0;
static uint64_t methodCount=0;
static uint64_t exprCount=0;
static uint64_t blockCount=0;

class Node {
protected:
	const char m_DELIM = ':';
	const char* m_PREFIX = "----";
public:
    Node(){
        ++nodeCount;
#ifdef PRINT_AND_JOSONGEN       
        std::cout<<nodeCount<<std::endl;
#endif
    }
	virtual ~Node() {}
	virtual std::string getTypeName() const = 0;	
	virtual llvm::Value *codeGen(CodeGenContext &context) { return (llvm::Value *)0; }
#ifdef PRINT_AND_JOSONGEN
    virtual void print(std::string prefix) const{}
	virtual Json::Value jsonGen() const { return Json::Value(); }
#endif

};


class NStatement : public Node {
public:
    NStatement(){}

	std::string getTypeName() const override {
		return "NStatement";
	}
#ifdef PRINT_AND_JOSONGEN
    virtual void print(std::string prefix) const override{
        std::cout << prefix << getTypeName() << std::endl;
    }

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        return root;
    }
#endif
};

class NExpression : public Node {
public:
    NExpression(){}

	std::string getTypeName() const override {
		return "NExpression";
	}
#ifdef PRINT_AND_JOSONGEN
    virtual void print(std::string prefix) const override{
        std::cout << prefix << getTypeName() << std::endl;
    }

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        return root;
    }
#endif

};



class NDouble : public NExpression {
public:
	double value;
    uint64_t index;

    NDouble(){++doubleCount;}

	NDouble(double value)
		: value(value) {
            ++doubleCount;
	}

	std::string getTypeName() const override {
#ifdef GET_TYPE_NAME
		return "NDouble";
#else
        return "";
#endif
	}
#ifdef PRINT_AND_JOSONGEN
	void print(std::string prefix) const override{
		cout << prefix << getTypeName() << this->m_DELIM << value << endl;
	}

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName() + this->m_DELIM + std::to_string(value);
        return root;
    }
#endif

	virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NInteger : public NExpression {
public:
    uint64_t value;
    uint64_t index;

    NInteger(){++intCount;}

    NInteger(uint64_t value)
            : value(value) {
                ++intCount;
    }

    std::string getTypeName() const override {
        return "NInteger";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override{
        cout << prefix << getTypeName() << this->m_DELIM << value << endl;
    }

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName() + this->m_DELIM + std::to_string(value);
        return root;
    }
#endif

    operator NDouble(){
        return NDouble(value);
    }

    virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NIdentifier : public NExpression {
public:
	std::string name;
    bool isType = false;
    bool isArray = false;

    std::shared_ptr<ExpressionList> arraySize = std::make_shared<ExpressionList>();

    NIdentifier(){++exprCount;}

	NIdentifier(const std::string &name)
		: name(name) {
            ++exprCount;
	}

	std::string getTypeName() const override {
		return "NIdentifier";
	}
#ifdef PRINT_AND_JOSONGEN
    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName() + this->m_DELIM + name + (isArray ? "(Array)" : "");
        for(auto it=arraySize->begin(); it!=arraySize->end(); it++){
            root["children"].append((*it)->jsonGen());
        }
        return root;
    }

	void print(std::string prefix) const override{
        std::string nextPrefix = prefix+this->m_PREFIX;
		cout << prefix << getTypeName() << this->m_DELIM << name << (isArray ? "(Array)" : "") << endl;
        if( isArray && arraySize->size() > 0 ){
//            assert(arraySize != nullptr);
            for(auto it=arraySize->begin(); it!=arraySize->end(); it++){
                (*it)->print(nextPrefix);
            }
        }
	}
#endif
	virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NMethodCall: public NExpression {
public:
	const shared_ptr<NIdentifier> id;
	shared_ptr<ExpressionList> arguments = make_shared<ExpressionList>();

    NMethodCall(){
        ++methodCount;
    }

	NMethodCall(const shared_ptr<NIdentifier> id, shared_ptr<ExpressionList> arguments)
		: id(id), arguments(arguments) {
            ++methodCount;
	}

	NMethodCall(const shared_ptr<NIdentifier> id)
		: id(id) {
            ++methodCount;
	}

	std::string getTypeName() const override {
		return "NMethodCall";
	}
#ifdef PRINT_AND_JOSONGEN
    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(this->id->jsonGen());
        for(auto it=arguments->begin(); it!=arguments->end(); it++){
            root["children"].append((*it)->jsonGen());
        }
        return root;
    }

	void print(std::string prefix) const override{
		std::string nextPrefix = prefix+this->m_PREFIX;
		cout << prefix << getTypeName() << this->m_DELIM << endl;
		this->id->print(nextPrefix);
		for(auto it=arguments->begin(); it!=arguments->end(); it++){
			(*it)->print(nextPrefix);
		}
	}
#endif
	virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NBinaryOperator : public NExpression {
public:
	int op;
	shared_ptr<NExpression> lhs;
	shared_ptr<NExpression> rhs;

    NBinaryOperator(){}

    NBinaryOperator(shared_ptr<NExpression> lhs, int op, shared_ptr<NExpression> rhs)
            : lhs(lhs), rhs(rhs), op(op) {
    }

	std::string getTypeName() const override {
		return "NBinaryOperator";
	}
#ifdef PRINT_AND_JOSONGEN
    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName() + this->m_DELIM + std::to_string(op);

        root["children"].append(lhs->jsonGen());
        root["children"].append(rhs->jsonGen());

        return root;
    }

	void print(std::string prefix) const override{
		std::string nextPrefix = prefix+this->m_PREFIX;
		std::cout << prefix << getTypeName() << this->m_DELIM << op << std::endl;

		lhs->print(nextPrefix);
		rhs->print(nextPrefix);
	}
#endif

	virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NAssignment : public NExpression {
public:
	shared_ptr<NIdentifier> lhs;
	shared_ptr<NExpression> rhs;

    NAssignment(){}

	NAssignment(shared_ptr<NIdentifier> lhs, shared_ptr<NExpression> rhs)
		: lhs(lhs), rhs(rhs) {
	}

	std::string getTypeName() const override {
		return "NAssignment";
	}
#ifdef PRINT_AND_JOSONGEN
	void print(std::string prefix) const override{
		std::string nextPrefix = prefix+this->m_PREFIX;
		std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;
		lhs->print(nextPrefix);
		rhs->print(nextPrefix);
	}

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(lhs->jsonGen());
        root["children"].append(rhs->jsonGen());
        return root;
    }
#endif

	virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NBlock : public NExpression {
public:
	shared_ptr<StatementList> statements = make_shared<StatementList>();

    NBlock(){
        ++blockCount;
    }

	std::string getTypeName() const override {
		return "NBlock";
	}
#ifdef PRINT_AND_JOSONGEN
	void print(std::string prefix) const override{
		std::string nextPrefix = prefix+this->m_PREFIX;
		std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;
		for(auto it=statements->begin(); it!=statements->end(); it++){
			(*it)->print(nextPrefix);
		}
	}

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        for(auto it=statements->begin(); it!=statements->end(); it++){
            root["children"].append((*it)->jsonGen());
        }
        return root;
    }
#endif

	virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NExpressionStatement : public NStatement {
public:
	shared_ptr<NExpression> expression;

    NExpressionStatement(){}

	NExpressionStatement(shared_ptr<NExpression> expression)
		: expression(expression) {
	}

	std::string getTypeName() const override {
		return "NExpressionStatement";
	}
#ifdef PRINT_AND_JOSONGEN
	void print(std::string prefix) const override{
		std::string nextPrefix = prefix+this->m_PREFIX;
		std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;
		expression->print(nextPrefix);
	}

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(expression->jsonGen());
        return root;
    }
#endif

	virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NVariableDeclaration : public NStatement {
public:
	const shared_ptr<NIdentifier> type;
	shared_ptr<NIdentifier> id;
	shared_ptr<NExpression> assignmentExpr = nullptr;
    int32_t index;

    NVariableDeclaration(){}

	NVariableDeclaration(const shared_ptr<NIdentifier> type, shared_ptr<NIdentifier> id, shared_ptr<NExpression> assignmentExpr = nullptr)
		: type(type), id(id), assignmentExpr(assignmentExpr) {
            //commit this line to get the clean output
            //std::cout << "isArray = " << type->isArray << std::endl;
            assert(type->isType);
            assert(!type->isArray || (type->isArray && type->arraySize != nullptr));
	}

	std::string getTypeName() const override {
		return "NVariableDeclaration";
	}
#ifdef PRINT_AND_JOSONGEN
	void print(std::string prefix) const override{
		std::string nextPrefix = prefix+this->m_PREFIX;
		std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;
		type->print(nextPrefix);
		id->print(nextPrefix);
        if( assignmentExpr != nullptr ){
            assignmentExpr->print(nextPrefix);
        }
	}

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(type->jsonGen());
        root["children"].append(id->jsonGen());
        if( assignmentExpr != nullptr ){
            root["children"].append(assignmentExpr->jsonGen());
        }
        return root;
    }
#endif
	virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NFunctionDeclaration : public NStatement {
public:
	shared_ptr<NIdentifier> type;
    shared_ptr<NIdentifier> id;
	shared_ptr<VariableList> arguments = make_shared<VariableList>();
	shared_ptr<NBlock> block;
    bool isExternal = false;

    NFunctionDeclaration(){}

	NFunctionDeclaration(shared_ptr<NIdentifier> type, shared_ptr<NIdentifier> id, shared_ptr<VariableList> arguments, shared_ptr<NBlock> block, bool isExt = false)
		: type(type), id(id), arguments(arguments), block(block), isExternal(isExt) {
        assert(type->isType);
	}

	std::string getTypeName() const override {
		return "NFunctionDeclaration";
	}
#ifdef PRINT_AND_JOSONGEN
	void print(std::string prefix) const override{
		std::string nextPrefix = prefix+this->m_PREFIX;
		std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;

		type->print(nextPrefix);
		id->print(nextPrefix);

		for(auto it=arguments->begin(); it!=arguments->end(); it++){
			(*it)->print(nextPrefix);
		}

        assert(isExternal || block != nullptr);
        if( block )
		    block->print(nextPrefix);
	}

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(type->jsonGen());
        root["children"].append(id->jsonGen());

        for(auto it=arguments->begin(); it!=arguments->end(); it++){
            root["children"].append((*it)->jsonGen());
        }

        assert(isExternal || block != nullptr);
        if( block ){
            root["children"].append(block->jsonGen());
        }

        return root;
    }
#endif

	virtual llvm::Value* codeGen(CodeGenContext&) override ;
};

class NStructDeclaration: public NStatement{
public:
    std::shared_ptr<NIdentifier> name;
    std::shared_ptr<VariableList> members = std::make_shared<VariableList>();

    NStructDeclaration(){}

    NStructDeclaration(shared_ptr<NIdentifier>  id, shared_ptr<VariableList> arguments)
            : name(id), members(arguments){

    }

    std::string getTypeName() const override {
        return "NStructDeclaration";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override {
        std::string nextPrefix = prefix+this->m_PREFIX;
        std::cout << prefix << getTypeName() << this->m_DELIM << this->name->name << std::endl;

        for(auto it=members->begin(); it!=members->end(); it++){
            (*it)->print(nextPrefix);
        }
    }


    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName() + this->m_DELIM + this->name->name;

        for(auto it=members->begin(); it!=members->end(); it++){
            root["children"].append((*it)->jsonGen());
        }

        return root;
    }
#endif

    virtual llvm::Value* codeGen(CodeGenContext& context) override ;
};

class NReturnStatement: public NStatement{
public:
    shared_ptr<NExpression> expression;

    NReturnStatement(){}

    NReturnStatement(shared_ptr<NExpression>  expression)
            : expression(expression) {

    }

    std::string getTypeName() const override {
        return "NReturnStatement";
    }
#ifdef PRINT_AND_JOSONGEN
    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(expression->jsonGen());
        return root;
    }

    void print(std::string prefix) const override {
        std::string nextPrefix = prefix + this->m_PREFIX;
        std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;

        expression->print(nextPrefix);
    }
#endif
    virtual llvm::Value* codeGen(CodeGenContext& context) override ;

};

class NIfStatement: public NStatement{
public:

    shared_ptr<NExpression>  condition;
    shared_ptr<NBlock> trueBlock;          // must not null
    shared_ptr<NBlock> falseBlock;         // could null


    NIfStatement(){}

    NIfStatement(shared_ptr<NExpression>  cond, shared_ptr<NBlock> blk, shared_ptr<NBlock> blk2 = nullptr)
            : condition(cond), trueBlock(blk), falseBlock(blk2){

    }

    std::string getTypeName() const override {
        return "NIfStatement";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override{
        std::string nextPrefix = prefix + this->m_PREFIX;
        cout << prefix << getTypeName() << this->m_DELIM << endl;

        condition->print(nextPrefix);

        trueBlock->print(nextPrefix);

        if( falseBlock ){
            falseBlock->print(nextPrefix);
        }

    }

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();
        root["children"].append(condition->jsonGen());
        root["children"].append(trueBlock->jsonGen());
        if( falseBlock ){
            root["children"].append(falseBlock->jsonGen());
        }
        return root;
    }
#endif

    llvm::Value *codeGen(CodeGenContext&) override ;


};

class NForStatement: public NStatement{
public:
    shared_ptr<NExpression> initial, condition, increment;
    shared_ptr<NBlock>  block;

    NForStatement(){}

    NForStatement(shared_ptr<NBlock> b, shared_ptr<NExpression> init = nullptr, shared_ptr<NExpression> cond = nullptr, shared_ptr<NExpression> incre = nullptr)
            : block(b), initial(init), condition(cond), increment(incre){
        if( condition == nullptr ){
            condition = make_shared<NInteger>(1);
        }
    }

    std::string getTypeName() const override{
        return "NForStatement";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override{

        std::string nextPrefix = prefix + this->m_PREFIX;
        std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;

        if( initial )
            initial->print(nextPrefix);
        if( condition )
            condition->print(nextPrefix);
        if( increment )
            increment->print(nextPrefix);

        block->print(nextPrefix);
    }


    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();

        if( initial )
            root["children"].append(initial->jsonGen());
        if( condition )
            root["children"].append(condition->jsonGen());
        if( increment )
            root["children"].append(increment->jsonGen());

        return root;
    }
#endif
    llvm::Value *codeGen(CodeGenContext&) override ;

};

class NStructMember: public NExpression{
public:
	shared_ptr<NIdentifier> id;
	shared_ptr<NIdentifier> member;

    NStructMember(){}
    
    NStructMember(shared_ptr<NIdentifier> structName, shared_ptr<NIdentifier>member)
            : id(structName),member(member) {
    }

    std::string getTypeName() const override{
        return "NStructMember";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override{

        std::string nextPrefix = prefix + this->m_PREFIX;
        cout << prefix << getTypeName() << this->m_DELIM << endl;

        id->print(nextPrefix);
        member->print(nextPrefix);
    }


    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();

        root["children"].append(id->jsonGen());
        root["children"].append(member->jsonGen());

        return root;
    }
#endif
    llvm::Value *codeGen(CodeGenContext&) override ;

};

class NArrayIndex: public NExpression{
public:
    std::shared_ptr<NIdentifier>  arrayName;
    std::shared_ptr<ExpressionList> expressions = std::make_shared<ExpressionList>();
    int32_t aSize;

    NArrayIndex(){}

    NArrayIndex(shared_ptr<NIdentifier>  name, shared_ptr<NExpression>  exp)
            : arrayName(name){
        expressions->push_back(exp);
    }


    NArrayIndex(shared_ptr<NIdentifier>  name, shared_ptr<ExpressionList> list)
            : arrayName(name), expressions(list){
    }

    std::string getTypeName() const override{
        return "NArrayIndex";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override{
        std::string nextPrefix = prefix + this->m_PREFIX;
        cout << prefix << getTypeName() << this->m_DELIM << endl;

        arrayName->print(nextPrefix);
        for(auto it=expressions->begin(); it!=expressions->end(); it++){
            (*it)->print(nextPrefix);
        }
    }

    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();

        root["children"].append(arrayName->jsonGen());
        for(auto it=expressions->begin(); it!=expressions->end(); it++){
            root["children"].append((*it)->jsonGen());
        }
        return root;
    }
#endif

    llvm::Value *codeGen(CodeGenContext&) override ;

};

class NArrayAssignment: public NExpression{
public:
    std::shared_ptr<NArrayIndex> arrayIndex;
    std::shared_ptr<NExpression>  expression;

    NArrayAssignment(){}

    NArrayAssignment(shared_ptr<NArrayIndex> index, shared_ptr<NExpression>  exp)
            : arrayIndex(index), expression(exp){

    }

    std::string getTypeName() const override{
        return "NArrayAssignment";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override{

        std::string nextPrefix = prefix + this->m_PREFIX;
        std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;

        arrayIndex->print(nextPrefix);
        expression->print(nextPrefix);
    }


    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();

        root["children"].append(arrayIndex->jsonGen());
        root["children"].append(expression->jsonGen());

        return root;
    }
#endif

    llvm::Value *codeGen(CodeGenContext&) override ;

};

class NArrayInitialization: public NStatement{
public:

    NArrayInitialization(){}

    std::shared_ptr<NVariableDeclaration> declaration;
    std::shared_ptr<ExpressionList> expressionList = std::make_shared<ExpressionList>();

    NArrayInitialization(shared_ptr<NVariableDeclaration> dec, shared_ptr<ExpressionList> list)
            : declaration(dec), expressionList(list){

    }

    std::string getTypeName() const override{
        return "NArrayInitialization";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override{

        std::string nextPrefix = prefix + this->m_PREFIX;
        std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;

        declaration->print(nextPrefix);
        for(auto it=expressionList->begin(); it!=expressionList->end(); it++){
            (*it)->print(nextPrefix);
        }
    }


    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();

        root["children"].append(declaration->jsonGen());
        for(auto it=expressionList->begin(); it!=expressionList->end(); it++)
            root["children"].append((*it)->jsonGen());

        return root;
    }
#endif

    llvm::Value *codeGen(CodeGenContext &context) override ;

};

class NStructAssignment: public NExpression{
public:
    std::shared_ptr<NStructMember> structMember;
    std::shared_ptr<NExpression>  expression;

    NStructAssignment(){}

    NStructAssignment(shared_ptr<NStructMember> member, shared_ptr<NExpression>  exp)
            : structMember(member), expression(exp){

    }

    std::string getTypeName() const override{
        return "NStructAssignment";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override{

        std::string nextPrefix = prefix + this->m_PREFIX;
        std::cout << prefix << getTypeName() << this->m_DELIM << std::endl;

        structMember->print(nextPrefix);
        expression->print(nextPrefix);
    }


    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName();

        root["children"].append(structMember->jsonGen());
        root["children"].append(expression->jsonGen());

        return root;
    }
#endif
    llvm::Value *codeGen(CodeGenContext&) override;

};

class NLiteral: public NExpression{
public:
    std::string value;

    NLiteral(){}

    NLiteral(const std::string &str) {
        value = str.substr(1, str.length()-2);
    }

    std::string getTypeName() const override{
        return "NLiteral";
    }
#ifdef PRINT_AND_JOSONGEN
    void print(std::string prefix) const override{
        std::cout << prefix << getTypeName() << this->m_DELIM << value << std::endl;
    }
    Json::Value jsonGen() const override {
        Json::Value root;
        root["name"] = getTypeName() + this->m_DELIM + value;
        return root;
    }
#endif

    llvm::Value *codeGen(CodeGenContext&) override;

};
std::unique_ptr<NExpression> LogError(const char* str);

#endif
