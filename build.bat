@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set "PATH=C:\Program Files\CMake\bin;C:\Users\Lenovo\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;C:\Qt\6.8.2\msvc2022_64\bin;%PATH%"
set "CMAKE_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64;%CMAKE_PREFIX_PATH%"
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
