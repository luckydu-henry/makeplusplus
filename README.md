# makeplusplus
Makeplusplus is a tool that allows you to generate vcxproj/makefile with C++ (both syntax and performance)
This project now only supports visual studio and still have many bugs but I will fix them entirely in the future.

makeplusplus contains a generation program "makexx" which is very tiny program to generate visual studio solution/makefile
to build "`makexx`", run `./makexx -gh` under `makexx` folder and then run `./makexx -gv ../makexx.make.cpp` to generate visual studio solution.

Yes, this program itself is descripted with makeplusplus.

And if you want to have more details about how to use makeplusplus in your own application development, please check `makexx.make.cpp` for more usages which is the project descriptor.

Please send me messages or pull requrests to make this project better.