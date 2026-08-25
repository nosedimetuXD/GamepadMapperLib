@echo off
setlocal enabledelayedexpansion

echo =========================================================
echo    GamepadMapperLib Universal Build Script
echo =========================================================

:: 1. AUTO-DETECT VISUAL STUDIO / MSVC
set "VS_WHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VS_WHERE!" set "VS_WHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

set "VCVARS="
if exist "!VS_WHERE!" (
    for /f "usebackq tokens=*" %%i in (`"!VS_WHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
        if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
            set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
        )
    )
)

:: Fallback standard Visual Studio search locations
if not defined VCVARS (
    for %%p in (
        "C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2026\Professional\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2026\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2026\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    ) do (
        if exist %%p if not defined VCVARS set "VCVARS=%%~p"
    )
)

if not defined VCVARS (
    echo [ERROR] No se pudo encontrar Visual Studio con soporte C++ x64.
    echo Por favor instala Visual Studio 2022/2026 con la carga de trabajo "Desarrollo para el escritorio con C++".
    pause
    exit /b 1
)

echo [OK] Usando compilador MSVC: !VCVARS!
call "!VCVARS!" >nul 2>&1

:: 2. AUTO-DETECT CMAKE
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    if defined VS_PATH if exist "!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "PATH=!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;!PATH!"
    ) else if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files\CMake\bin;!PATH!"
    )
)

:: 3. AUTO-DETECT NINJA
where ninja >nul 2>&1
if %ERRORLEVEL% neq 0 (
    if defined VS_PATH if exist "!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
        set "PATH=!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;!PATH!"
    ) else (
        for /d %%n in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\Ninja*") do (
            if exist "%%n\ninja.exe" set "PATH=%%n;!PATH!"
        )
    )
)

:: 4. AUTO-DETECT QT 6
set "FOUND_QT="
if defined QTDIR if exist "%QTDIR%\bin\Qt6Core.dll" set "FOUND_QT=%QTDIR%"
if not defined FOUND_QT if defined CMAKE_PREFIX_PATH if exist "%CMAKE_PREFIX_PATH%\bin\Qt6Core.dll" set "FOUND_QT=%CMAKE_PREFIX_PATH%"

if not defined FOUND_QT (
    for %%d in (C:\Qt D:\Qt E:\Qt) do (
        if exist "%%d" (
            for /f "delims=" %%v in ('dir /b /ad /o-n "%%d\6.*" 2^>nul') do (
                for /f "delims=" %%m in ('dir /b /ad /o-n "%%d\%%v\msvc*_64" 2^>nul') do (
                    if not defined FOUND_QT if exist "%%d\%%v\%%m\bin\Qt6Core.dll" (
                        set "FOUND_QT=%%d\%%v\%%m"
                    )
                )
            )
        )
    )
)

if defined FOUND_QT (
    echo [OK] Usando Qt6 detectado en: !FOUND_QT!
    set "PATH=!FOUND_QT!\bin;!PATH!"
    set "CMAKE_PREFIX_PATH=!FOUND_QT!;!CMAKE_PREFIX_PATH!"
    set "QT_PLUGIN_PATH=!FOUND_QT!\plugins"
) else (
    echo [AVISO] No se detecto instalacion de Qt6 en rutas estandar. CMake intentara buscarlo en el sistema.
)

:: 5. CONFIGURE & BUILD
echo [INFO] Configurando CMake...
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [AVISO] Ninja no disponible o fallo de configuracion, intentando generador por defecto de Visual Studio...
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
)

echo [INFO] Compilando libreria y ejecutables...
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] La compilacion fallo.
    exit /b %ERRORLEVEL%
)

echo [OK] Compilacion completada exitosamente.

:: 6. DEPLOY BINARIES TO ROOT IF RUNNING FROM SUBMODULE
if exist "..\gamepad_mapper.py" (
    echo [INFO] Desplegando binarios actualizados en la raiz del proyecto...
    if exist "build\GamepadMapper.dll" copy /y "build\GamepadMapper.dll" "..\GamepadMapper.dll" >nul 2>&1
    if exist "build\SingleGamepadMapperApp.exe" copy /y "build\SingleGamepadMapperApp.exe" "..\SingleGamepadMapperApp.exe" >nul 2>&1
    if exist "build\GamepadMapperApp.exe" copy /y "build\GamepadMapperApp.exe" "..\GamepadMapperApp.exe" >nul 2>&1
    if exist "build\_deps\sdl3-build\SDL3.dll" copy /y "build\_deps\sdl3-build\SDL3.dll" "..\SDL3.dll" >nul 2>&1
)

echo =========================================================
echo    Build finalizado con exito!
echo =========================================================
