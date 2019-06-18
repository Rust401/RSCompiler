all: compiler

OBJS = grammar.o \
		token.o  \
		CodeGen.o \
		utils.o \
		main.o	 \
		ObjGen.o \
		TypeSystem.o \

LLVMCONFIG = /usr/local/opt/llvm/bin/llvm-config
CPPFLAGS = `$(LLVMCONFIG) --cppflags`  `pkg-config --cflags jsoncpp` -std=c++11
LDFLAGS = `$(LLVMCONFIG) --ldflags` -pthread -ldl -lz -lncurses -rdynamic -L/usr/local/lib -ljsoncpp
LIBS = `$(LLVMCONFIG) --libs`

clean:
	$(RM) -rf grammar.cpp grammar.hpp test compiler output.o tokens.cpp *.output $(OBJS)


ObjGen.cpp: ObjGen.h

CodeGen.cpp: CodeGen.h ASTNodes.h

grammar.cpp: grammar.y
	bison -d -o $@ $<

grammar.hpp: grammar.cpp

token.cpp: token.l grammar.hpp
	flex -o $@ $<

%.o: %.cpp
	clang++ -c $(CPPFLAGS) -o $@ $<

compiler: $(OBJS)
	clang++ $(CPPFLAGS) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

test: compiler testFile/newtest.input
	cat testFile/newtest.input | ./compiler > IR.txt
	cat IR.txt
	mv IR.txt testFile/

run: compiler test
	clang++ -o dude output.o
	mv dude bin/
	bin/dude

testlink: output.o testmain.cpp
	clang output.o testmain.cpp -o test
	./test
