@echo off
setlocal enabledelayedexpansion

echo 🔍 Searching for C++ Compiler (cl.exe)...

:: 1. Try standard vswhere method
set "VSPATH="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do (
        set "VSPATH=%%i"
    )
)

:: 2. Manual fallback
if not defined VSPATH if exist "C:\Program Files\Microsoft Visual Studio\2022\Community" set "VSPATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
if not defined VSPATH if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional" set "VSPATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
if not defined VSPATH if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise" set "VSPATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"

if not defined VSPATH (
    echo ❌ ERROR: Could not find Visual Studio.
    pause
    exit /b
)

echo ✅ Found Visual Studio at: !VSPATH!

:: Find the vcvars64.bat file
set "VCVARS="
if exist "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=!VSPATH!\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "!VSPATH!\VC\BuildTools\vcvars64.bat" set "VCVARS=!VSPATH!\VC\BuildTools\vcvars64.bat"

if not defined VCVARS (
    echo ❌ ERROR: Missing C++ Build Tools ^(vcvars64.bat^).
    pause
    exit /b
)

echo ⚙️ Initializing Environment...
call "!VCVARS!"

echo 🔨 Compiling Source1.cpp...
cl /EHsc /MD /std:c++17 Source1.cpp ^
    /I "C:\vcpkg\installed\x64-windows\include" ^
    /I "C:\vcpkg\installed\x64-windows\include\imgui" ^
    "C:\vcpkg\installed\x64-windows\lib\glew32.lib" ^
    "C:\vcpkg\installed\x64-windows\lib\glfw3dll.lib" ^
    "C:\vcpkg\installed\x64-windows\lib\imgui.lib" ^
    opengl32.lib user32.lib gdi32.lib shell32.lib ^
    /Fe:RosslerCpp.exe

if %ERRORLEVEL% NEQ 0 goto :failed

echo 🚀 Success! Copying DLLs...
copy "C:\vcpkg\installed\x64-windows\bin\glfw3.dll" . >nul 2>&1
copy "C:\vcpkg\installed\x64-windows\bin\glew32.dll" . >nul 2>&1

echo 🏃 Launching...
start RosslerCpp.exe
exit /b

:failed
echo ❌ Compilation failed.
pause
exit /b
