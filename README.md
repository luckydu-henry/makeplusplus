# makeplusplus
Makeplusplus is a tool that allows you to generate vcxproj/makefile with C++ (both syntax and performance)
This project now only supports visual studio and still have many bugs but I will fix them entirely in the future.

## Features
The main benefits of this project are "*simple*", "*small*" and "*efficiency*".

The most "programmable" build system out there is `Sharpmake` by UBISOFT, but it requires C# and dotnet stuff, even though it's
fast and flexible but it's too big (with lots of dlls). However `makexx` can provide most features `Sharpmake` can provide in
a simple C++ language with some macros, its IDE support is also very well.

Now this program has reached 375kb in MSVC's 'O1 MinSpace' build
and 155kb after '`upx`' compressed. `Premake5` requires a 1200+kb exe and even raw lua5.4 vclib requires 370+kb. Thus you can see this binary
is really really tiny.

Since this binary is written by C++ and using only tinyxml2 and some STL, so its runtime speed can be very fast,
also it didn't provide any addition check or advanced features (fetch content...) so its generation speed is also very fast (much faster than CMake).

## Usage
makeplusplus contains a generation program "`makexx`" which is very tiny program to generate visual studio solution/makefile.

to build "`makexx`", run `./makexx -gh` under `makexx` folder and then run `./makexx -gv ../makexx.make.cpp` to generate visual studio solution.

Yes, this program itself is descripted with makeplusplus.

And if you want to have more details about how to use makeplusplus in your own application development, please check `makexx.make.cpp` for more usages which is the project descriptor.

## What's more
Please send me messages or pull requrests to make this project better.