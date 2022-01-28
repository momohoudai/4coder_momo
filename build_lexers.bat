@echo off

copy .\setup_cl_generic.bat ..\bin\setup_cl_generic.bat
call ..\bin\build_one_time .\4coder_momo_lexer_gen_cpp.cpp ..\..\
..\..\one_time.exe
copy ..\generated\lexer_cpp.h .\generated\4coder_momo_lexer_cpp.h
copy ..\generated\lexer_cpp.cpp .\generated\4coder_momo_lexer_cpp.cpp

call ..\bin\build_one_time .\4coder_momo_lexer_gen_cs.cpp ..\..\
..\..\one_time.exe
copy ..\generated\lexer_cs.h .\generated\4coder_momo_lexer_cs.h
copy ..\generated\lexer_cs.cpp .\generated\4coder_momo_lexer_cs.cpp

call ..\bin\build_one_time .\4coder_momo_lexer_gen_go.cpp ..\..\
..\..\one_time.exe
copy ..\generated\lexer_go.h .\generated\4coder_momo_lexer_go.h
copy ..\generated\lexer_go.cpp .\generated\4coder_momo_lexer_go.cpp

