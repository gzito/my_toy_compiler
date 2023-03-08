# What changes are in this version
I tested it with LLVM V15.0.7 on Windows 10 with mingw64 (mingw-w64-x86_64-llvm and mingw-w64-x86_64-clang).  
And I also added function level optimization passes.

# my_toy_compiler
Source code for "My Toy Compiler". Read about how I did on my blog:

http://gnuu.org/2009/09/18/writing-your-own-toy-compiler

## llvm 15 compatibility
Yes, it works! :)

## run 
./parser example.txt

## debug

lldb ./parser
breakpoint set --file codegen.cpp --line 80
run example.txt
