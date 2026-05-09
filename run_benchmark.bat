@echo off
title cmem Allocator Benchmark
setlocal enabledelayedexpansion

:: script root directory
set ROOT=%~dp0

echo =============================================
echo   cmem Custom Memory Allocator
echo   1-click Build + Test + Benchmark
echo =============================================
echo.

:: ---- tool detection (edit these paths if needed) ----------------
set MINGW=C:\Program Files (x86)\Dev-Cpp\MinGW64\bin
set CMAKE_PATH=C:\Program Files\CMake\bin

:: add to PATH if not already there
where cmake.exe >nul 2>&1 || set "PATH=%CMAKE_PATH%;%PATH%"
where g++.exe  >nul 2>&1  || set "PATH=%MINGW%;%PATH%"

:: verify tools
where cmake.exe >nul 2>&1
if errorlevel 1 (
    echo [FAIL] CMake not found. Install CMake or edit CMAKE_PATH in this script.
    goto end
)
for /f "tokens=3" %%v in ('cmake --version 2^>nul ^| findstr /i version') do echo   cmake : %%v

where g++.exe >nul 2>&1
if errorlevel 1 (
    echo [FAIL] g++ not found. Install MinGW or edit MINGW path in this script.
    goto end
)
for /f "tokens=3" %%v in ('g++ --version 2^>nul ^| findstr /i "g++"') do echo   g++   : %%v

echo.

:: ---- clean stale build cache -------------------------------------
if exist "%ROOT%build\" (
    echo [clean] removing old build cache ...
    rmdir /s /q "%ROOT%build" >nul 2>&1
    echo [clean] done.
    echo.
)

:: ---- configure --------------------------------------------------
echo [1/4] CMake configure ...
cd /d "%ROOT%"

cmake -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMEM_STRATEGY=SEGREGATED -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo.
    echo [FAIL] CMake configure failed. Check the error message above.
    echo.
    goto end
)
echo   done.
echo.

:: ---- compile ----------------------------------------------------
set BLD=%ROOT%build

echo [2/4] Compile ...
cd /d "%BLD%"
mingw32-make -j4
if errorlevel 1 (
    echo [FAIL] Build failed.
    goto end
)
echo   done.
echo.

:: ---- locate executables ------------------------------------------
set DEMO=
set BENCH=
for /f "delims=" %%f in ('dir /s /b "%BLD%\cmem_demo.exe" 2^>nul') do set DEMO=%%f
for /f "delims=" %%f in ('dir /s /b "%BLD%\cmem_bench.exe" 2^>nul') do set BENCH=%%f

if "%DEMO%"=="" (
    echo [FAIL] cmem_demo.exe not found in build tree.
    dir /s /b "%BLD%\*.exe" 2>nul
    goto end
)
if "%BENCH%"=="" (
    echo [FAIL] cmem_bench.exe not found in build tree.
    goto end
)

:: ---- test --------------------------------------------------------
echo [3/4] Functional test ...
echo.
"%DEMO%"
if errorlevel 1 (
    echo [FAIL] Functional test failed.
    goto end
)
echo.

:: ---- benchmark ---------------------------------------------------
echo [4/4] Performance benchmark ^(vs system malloc^) ...
echo.
"%BENCH%"

:end
echo.
echo =============================================
echo   Done - Press any key to exit
echo =============================================
pause >nul
