@echo off
call ..\bin\build_one_time .\4coder_momo_cpp_lexer_gen.cpp ..\..\
..\..\one_time.exe
copy ..\generated\lexer_cpp.h .\generated\4coder_momo_lexer_cpp.h
copy ..\generated\lexer_cpp.cpp .\generated\4coder_momo_lexer_cpp.cpp

call ..\bin\build_one_time .\4coder_fleury_jai_lexer_gen.cpp ..\..\
..\..\one_time.exe
copy ..\generated\lexer_jai.h .\generated\4coder_fleury_lexer_jai.h
copy ..\generated\lexer_jai.cpp .\generated\4coder_fleury_lexer_jai.cpp