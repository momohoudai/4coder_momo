@echo off
call ..\bin\buildsuper_x64-win.bat .\4coder_momo.cpp debug
copy .\custom_4coder.dll ..\..\custom_4coder.dll
copy .\custom_4coder.pdb ..\..\custom_4coder.pdb
copy .\bindings.4coder ..\..\bindings.4coder
copy .\theme-momo.4coder ..\..\themes\theme-momo.4coder