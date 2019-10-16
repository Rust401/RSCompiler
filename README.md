# RSCompiler

## 序

1. 简介
	
	是一个基于`flex`，`bison`和`llvm`，用C++编写的类C语言编译器。flex进行词法分析，bison进行语法分析生成**AST**（抽象语法树），从AST的根节点开始分析，利用LLVM的API将AST翻译成`LLVM IR`，最后将`LLVM IR`利用`LLVM Back ends`模块的接口，根据目标机器的指令集和操作系统，将中间代码编译为二进制代码，生成可执行文件。

	其语法是C语言的一个子集，做了部分修改，会在后续详细说明  

	支持的类型：
	* void
	* int
	* double
	* char
	* bool
	* 自定义结构体
	* 数组

	支持的语法：
	* 变量的声明、初始化
	* 自定义函数声明、调用
	* 外部函数的声明、调用
	* 控制流（if-else,for,while)
	* 变量的赋值
	* 全局变量
	* 参数类型隐式类型转换

	其中根据语法生成AST以及礼用LLVM的API对AST分析生成目标代码是本项目核心工作量，控制流（**if-else,for loop,while**)，数组，结构体的实现则是本项目的难点，会在后续着重分析讲解。

2. 代码结构
```
参见源码
```

3. 运行环境
```txt
OS:	MacOS 10.13 High Sierra || Ubuntu LTS 18.04	
flex:	2.6.4
bison:	3.0.4
llvm:	6.0.0
clang:	6.0.0
c++:	std=c++11
```

4. 使用说明
* 编译编译器，得到`compiler`
```sh
make
```
* 编译整个项目并将测试文件的中间代码输出,得到`IR.txt`
```sh
make test
```
* 将中间代码转换成可执行文件并运行
```
make run
```


5. 参考资料
	目前国内系统性的优质教程较少，近年api更新的速度也很快，参考资料相对有限。整个数据结构的定义和运行框架参考第三个网站，LLVM的概念理解及API的初步学习参考了第二个网站，api的细节参考了第一个网站，也就是LLVM的官方文档。  
	  [LLVM Language Reference Manual](http://llvm.org/docs/LangRef.html)  
    [LLVM Tutorial](http://releases.llvm.org/6.0.1/docs/tutorial/index.html)  
    [gnuu - writing your own toy compiler](http://gnuu.org/2009/09/18/writing-your-own-toy-compiler/)

## 语法定义

基本语法和控制流的用法
```c
extern int printf(string format)
extern int puts(string s)

int fib(int n){
    int result
    if(n<3){
		result=1
	}else{
		result=fib(n-1)+fib(n-2)
	}
    return result
}

int main(int argc){
    int i
    argc = 15
    for( i = 1 ; i<argc; i=i+1){
        printf("i=%d, func=%d", i, fib(i))
        puts("")
    }
    return 0
}
```
首先声明了两个外部函数,`printf(string str)`用来打印正常的字符串，`puts`用来打印回车换行。定义了一个递归求斐波那契数列的第n项的函数，然后从主函数中调用调用之打印前15项

再看下结构体怎么用
```c
struct Dude{
    int x
    int y
}

int func(struct Dude p){
    return p.x
}

int testDude(){
    struct Dude p
    p.x = 1
    p.y = 3
    func(p)
    return 0
}
```
数组的用法
```c
int testArray(){
    int[10] oneDim = [1,2,3,4]
    int[3][4] twoDim
    int i, j
    for(i=0; i<3; i=i+1){
        for(j=0; j<4; j=j+1){
            twoDim[i][j] = 0
        }
    }
    return 0
}
```
## 词法分析
定义好各类token(关键字，符号，用户自定义标识符，数字常量等)，在中定义好扫到之后的规则。碰到**类型，标识符，数字常量，字符串常量**除了返回token之外，还得把token的值存起来用于语法分析。

## 语法分析
我们把语句分为两类：有返回值的**Expression**,没有返回值的**Statement**。合法的语句均可规约。定义抽象语法树的根节点`Node`类，以及两种语法对应的`NStatement`以及`NExpression`类，作为所有语法的抽象基类,然后实现相关的接口即可。  
为了增强程序的鲁棒性，避免出现内存泄漏，本项目使用C++11提供的shared_ptr智能指针代替所有成员的裸指针，用于维护本语法特有的成员字段。此外，每类AST节点除了要维护这些字段外，还需要实现一个codeGen函数，作为每种语法树生成相应LLVM中间代码的接口。
Node类的定义
```cpp
class Node {
    public:
        Node(){}
        virtual ~Node() {}
        virtual string getTypeName() const = 0;
        virtual llvm::Value *codeGen(CodeGenContext &context) { return (llvm::Value *)0; }
    };
}
```
需要在bison中为各类的规约定义各种规则，以下是if-stmt的规约规则(仅作为例子)
```c
	if_stmt : TIF expr block 
    { $$ = new NIfStatement(shared_ptr<NExpression>($2), shared_ptr<NBlock>($3)); }
	| TIF expr block TELSE block
    { $$ = new NIfStatement(shared_ptr<NExpression>($2), shared_ptr<NBlock>($3), shared_ptr<NBlock>($5)); }
	| TIF expr block TELSE if_stmt 
    { 
        auto blk = new NBlock(); 
        blk->statements->push_back(shared_ptr<NStatement>($5)); 
        $$ = new NIfStatement(shared_ptr<NExpression>($2), shared_ptr<NBlock>($3), shared_ptr<NBlock>(blk)); 
    }
```
规约发生时便做括号里的动作。在这里遇到 `TIF expr block TELSE block` 类型的语法时，说明解析到一个标准的if-else语法，这时候便调用NIfStatement类的构造函数，同时将所需要的条件表达式（$2)以及true和false所要执行的两个block作为函数参数传递进去，生成一个NIfStatement结点的对象后返回给上一层级。
这个过程递归从叶子节点执行到根节点，最后变成一个大的语法树
## 语义分析
语义分析通常的任务:  
 * 判定每一个表达式的声明类型  
 * 判定每一个字段、形式参数、变量声明的类型  
 * 判断每一次赋值、传参数时，是否存在合法的隐式类型转换  
 * 判断一元和二元运算符左右两侧的类型是否合法（类型相同或存在隐式转换）  
 * 将所有要发生的隐式类型转换明确化  


这边举个例子，我们用`NIdentifier`来存储类型信息，除了保存其名字之外，还有两个`bool`去判断这个标识符是否是否为类型或者数组标识符。
NIdentifier类的定义:
```cpp
    class NIdentifier : public NExpression {
    public:
        std::string name;
        bool isType = false;
        bool isArray = false;

        shared_ptr<NInteger> arraySize;
        NIdentifier(){}
        NIdentifier(const std::string &name)
            : name(name), arraySize(nullptr) {
        }
        string getTypeName() const override {
            return "NIdentifier";
        }
        virtual llvm::Value* codeGen(CodeGenContext& context) override ;
    };
```
### 中间代码生成
拿到前端生成的AST的根节点之后就可以用LLVM实现中间代码的生成，将AST中Node的值转换成llvm封装好的数据结构，再把结构化语句插到合适的地方即可。  
在LLVM提供的接口下实现中间代码的生成比较优雅，因为LLVM将真实的指令抽象成了类似AST的指令，因此我们能够很方便的将AST的树形结构生成线性的中间代码。  
由于各个结点实现codeGen方法的方式大同小异，因此这里只用几个具有代表性的实现来解释生成方法。
举个例子：
* 生成NBlock的过程如下所示：
```cpp
llvm::Value* NBlock::codeGen(CodeGenContext &context) {
    cout << "Generating block" << endl;
    Value* last = nullptr;
    for(auto it=this->statements->begin(); it!=this->statements->end(); it++){
        last = (*it)->codeGen(context);
    }
    return last;
}
```
父节点处理好环境之后调用子节点的`codeGen()`函数就完事，子节点也一样，递归调用。  

* 生成Identifier
    ```c++
    llvm::Value* NIdentifier::codeGen(CodeGenContext &context) {
        cout << "Generating identifier " << this->name << endl;
        Value* value = context.getSymbolValue(this->name);
        if( !value ){
            return LogErrorV("Unknown variable name " + this->name);
        }
        if( value->getType()->isPointerTy() ){
            auto arrayPtr = context.builder.CreateLoad(value, "arrayPtr");
            if( arrayPtr->getType()->isArrayTy() ){
                cout << "(Array Type)" << endl;
                std::vector<Value*> indices;
                indices.push_back(ConstantInt::get(context.typeSystem.intTy, 0, false));
                auto ptr = context.builder.CreateInBoundsGEP(value, indices, "arrayPtr");
                return ptr;
            }
        }
        return context.builder.CreateLoad(value, false, "");
    }
    ```
标识符表达式主要用于引用有变量名的表达式中，其语义即为取出变量对应的值。对于普通变量则是直接从当前block的符号表中通过`getSymbolValue`先取出其存储地址，然后通过`CreateLoad`创建一条Load指令将变量值取出来并返回寄存器的地址给调用者

* 生成if-stmt
用llvm的接口创建三个block分别为thenBlock,falseBlock,mergeBlock，将其插入上下文中合适的位置，根据if之后的条件去判断跳转到哪个block，最终到mergeBlock中去找到SSA的值去判断先前跳转的究竟是哪个部分。
```cpp
llvm::Value* NIfStatement::codeGen(CodeGenContext &context) {
    cout << "Generating if statement" << endl;
    Value* condValue = this->condition->codeGen(context);
    if( !condValue )
        return nullptr;

    condValue = CastToBoolean(context, condValue);

    Function* theFunction = context.builder.GetInsertBlock()->getParent();     

    BasicBlock *thenBB = BasicBlock::Create(context.llvmContext, "then", theFunction);
    BasicBlock *falseBB = BasicBlock::Create(context.llvmContext, "else");
    BasicBlock *mergeBB = BasicBlock::Create(context.llvmContext, "ifcont");

    if( this->falseBlock ){
        context.builder.CreateCondBr(condValue, thenBB, falseBB);
    } else{
        context.builder.CreateCondBr(condValue, thenBB, mergeBB);
    }

    context.builder.SetInsertPoint(thenBB);

    context.pushBlock(thenBB);

    this->trueBlock->codeGen(context);

    context.popBlock();

    thenBB = context.builder.GetInsertBlock();

    if( thenBB->getTerminator() == nullptr ){       
        context.builder.CreateBr(mergeBB);
    }

    if( this->falseBlock ){
        theFunction->getBasicBlockList().push_back(falseBB);    
        context.builder.SetInsertPoint(falseBB);            

        context.pushBlock(thenBB);

        this->falseBlock->codeGen(context);

        context.popBlock();

        context.builder.CreateBr(mergeBB);
    }

    theFunction->getBasicBlockList().push_back(mergeBB);        
    context.builder.SetInsertPoint(mergeBB);        

    return nullptr;
}
```
另外还有数组、结构体、函数调用的代码生成部分部分，均在`codegen.cpp`文件中实现，篇幅过长这里不再赘述。

最终生成的中间代码的样例
```c
int main(){
    int a
    int i
    for(i=0;i<10;i=i+1){
        a=a*2
    }
    return 0
}
```
对应的中间代码
```
; ModuleID = 'main'
source_filename = "main"

define i32 @main() {
entry:
  %0 = alloca i32
  %1 = alloca i32
  store i32 0, i32* %1
  %arrayPtr = load i32, i32* %1
  %2 = load i32, i32* %1
  %cmptmp = icmp ult i32 %2, 10
  %3 = icmp ne i1 %cmptmp, false
  br i1 %3, label %forloop, label %forcont

forloop:                                          ; preds = %forloop, %entry
  %arrayPtr1 = load i32, i32* %0
  %4 = load i32, i32* %0
  %multmp = mul i32 %4, 2
  store i32 %multmp, i32* %0
  %arrayPtr2 = load i32, i32* %1
  %5 = load i32, i32* %1
  %addtmp = add i32 %5, 1
  store i32 %addtmp, i32* %1
  %arrayPtr3 = load i32, i32* %1
  %6 = load i32, i32* %1
  %cmptmp4 = icmp ult i32 %6, 10
  %7 = icmp ne i1 %cmptmp4, false
  br i1 %7, label %forloop, label %forcont

forcont:                                          ; preds = %forloop, %entry
  ret i32 0
}
```
## 目标代码生成

这边很套路，可以对着官网给的说明根据自己的机器一通初始化之后就完事了。

  1. （根据本地运行环境）初始化生成目标代码的TargetMachine  

```cpp
InitializeAllTargetInfos();
InitializeAllTargets();
InitializeAllTargetMCs();
InitializeAllAsmParsers();
InitializeAllAsmPrinters();
```

  2. 获取并设置当前环境的target triple
  ```cpp
  auto targetTriple = sys::getDefaultTargetTriple();
  context.theModule->setTargetTriple(targetTriple);
  ```
  
  3. 获取并设置TargetMachine信息
  ```cpp
  auto CPU = "generic";
  auto features = "";

  TargetOptions opt;
  auto RM = Optional<Reloc::Model>();
  auto theTargetMachine = Target->createTargetMachine(targetTriple, CPU, features, opt, RM);

  context.theModule->setDataLayout(theTargetMachine->createDataLayout());
  context.theModule->setTargetTriple(targetTriple);

  ```

  4. 将目标代码输出到文件
  ```cpp
  std::error_code EC;
  raw_fd_ostream dest(filename.c_str(), EC, sys::fs::F_None);

  legacy::PassManager pass;
  auto fileType = TargetMachine::CGFT_ObjectFile;

  if( theTargetMachine->addPassesToEmitFile(pass, dest, fileType) ){
      errs() << "theTargetMachine can't emit a file of this type";
      return;
  }

  pass.run(*context.theModule.get());
  dest.flush();
  ```
## 测试
测试代码
```c
extern int printf(string format)
extern int puts(string s)

int fib(int n){
    int result
    if(n<3){
		result=1
	}else{
		result=fib(n-1)+fib(n-2)
	}
    return result
}

int main(int argc){
    int i
    argc = 15
    for( i = 1 ; i<argc; i=i+1){
        printf("i=%d, func=%d", i, fib(i))
        puts("")
    }
    return 0
}
```

中间代码
```
; ModuleID = 'main'
source_filename = "main"

@string = private unnamed_addr constant [14 x i8] c"i=%d, func=%d\00"
@string.1 = private unnamed_addr constant [1 x i8] zeroinitializer

declare i32 @printf(i8*)

declare i32 @puts(i8*)

define i32 @fib(i32 %n) {
entry:
  %0 = alloca i32
  store i32 %n, i32* %0
  %1 = alloca i32
  %arrayPtr = load i32, i32* %0
  %2 = load i32, i32* %0
  %cmptmp = icmp ult i32 %2, 3
  %3 = icmp ne i1 %cmptmp, false
  br i1 %3, label %then, label %else

then:                                             ; preds = %entry
  store i32 1, i32* %1
  br label %ifcont

else:                                             ; preds = %entry
  %arrayPtr1 = load i32, i32* %0
  %4 = load i32, i32* %0
  %subtmp = sub i32 %4, 1
  %calltmp = call i32 @fib(i32 %subtmp)
  %arrayPtr2 = load i32, i32* %0
  %5 = load i32, i32* %0
  %subtmp3 = sub i32 %5, 2
  %calltmp4 = call i32 @fib(i32 %subtmp3)
  %addtmp = add i32 %calltmp, %calltmp4
  store i32 %addtmp, i32* %1
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %arrayPtr5 = load i32, i32* %1
  %6 = load i32, i32* %1
  ret i32 %6
}

define i32 @main(i32 %argc) {
entry:
  %0 = alloca i32
  store i32 %argc, i32* %0
  %1 = alloca i32
  store i32 15, i32* %0
  store i32 1, i32* %1
  %arrayPtr = load i32, i32* %1
  %2 = load i32, i32* %1
  %arrayPtr1 = load i32, i32* %0
  %3 = load i32, i32* %0
  %cmptmp = icmp ult i32 %2, %3
  %4 = icmp ne i1 %cmptmp, false
  br i1 %4, label %forloop, label %forcont

forloop:                                          ; preds = %forloop, %entry
  %arrayPtr2 = load i32, i32* %1
  %5 = load i32, i32* %1
  %arrayPtr3 = load i32, i32* %1
  %6 = load i32, i32* %1
  %calltmp = call i32 @fib(i32 %6)
  %calltmp4 = call i32 @printf([14 x i8]* @string, i32 %5, i32 %calltmp)
  %calltmp5 = call i32 @puts([1 x i8]* @string.1)
  %arrayPtr6 = load i32, i32* %1
  %7 = load i32, i32* %1
  %addtmp = add i32 %7, 1
  store i32 %addtmp, i32* %1
  %arrayPtr7 = load i32, i32* %1
  %8 = load i32, i32* %1
  %arrayPtr8 = load i32, i32* %0
  %9 = load i32, i32* %0
  %cmptmp9 = icmp ult i32 %8, %9
  %10 = icmp ne i1 %cmptmp9, false
  br i1 %10, label %forloop, label %forcont

forcont:                                          ; preds = %forloop, %entry
  ret i32 0
}
```


运行结果：
```sh
rust401@ubuntu:~/RSCompiler$ make run
clang++ -o dude output.o
./dude
i=1, func=1
i=2, func=1
i=3, func=2
i=4, func=3
i=5, func=5
i=6, func=8
i=7, func=13
i=8, func=21
i=9, func=34
i=10, func=55
i=11, func=89
i=12, func=144
i=13, func=233
i=14, func=377
```

