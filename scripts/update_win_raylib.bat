@echo off

pushd ..\VENDOR\raylib\src
mingw32-make

popd

set RAYLIB_VER=raylib-6.0_win64_mingw-w64-msvcrt

copy ..\VENDOR\raylib\src\raylib.h libs\%RAYLIB_VER%\include
copy ..\VENDOR\raylib\src\raymath.h libs\%RAYLIB_VER%\include
copy ..\VENDOR\raylib\src\rlgl.h libs\%RAYLIB_VER%\include

copy ..\VENDOR\raylib\src\libraylib.a libs\%RAYLIB_VER%\lib
