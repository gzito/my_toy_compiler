all: parser

OUTPUT_FOLDER = ./build

OBJS = $(OUTPUT_FOLDER)/parser.o  \
       $(OUTPUT_FOLDER)/codegen.o \
       $(OUTPUT_FOLDER)/main.o    \
       $(OUTPUT_FOLDER)/tokens.o  \
       $(OUTPUT_FOLDER)/corefn.o  \
	   $(OUTPUT_FOLDER)/native.o  \

LLVMCONFIG = llvm-config
CPPFLAGS = `$(LLVMCONFIG) --cppflags` -std=c++14
LDFLAGS = `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS = `$(LLVMCONFIG) --libs`

clean:
	$(RM) -rf parser.cpp parser.hpp parser tokens.cpp $(OBJS) 

parser.cpp: parser.y
	bison -d -o $@ $^
	
parser.hpp: parser.cpp

tokens.cpp: tokens.l parser.hpp
	flex -o $@ $^

$(OUTPUT_FOLDER)/%.o: %.cpp
	@mkdir -p $(@D)
	clang++ -gfull -c $(CPPFLAGS) -o $@ $<


parser: $(OBJS)
	clang++  -gfull -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

test: parser example.txt
	cat example.txt | ./parser
