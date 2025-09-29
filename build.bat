@echo off
rem ============================================================================
rem BUILD.BAT - Simple C++ Project Build Script for MSVC on Windows
rem ============================================================================

rem === SET UP PROJECT VARIABLES ===
setlocal enabledelayedexpansion
set "EXE_NAME=Game.exe"
set "PROJECT_ROOT=%~dp0"
set "BUILD_DIR=%PROJECT_ROOT%build"
set "SRC_DIR=%PROJECT_ROOT%src"
set "COMPILE_MODE=DEBUG"

rem === CLEANING BUILD FOLDER ===
if exist "%BUILD_DIR%" (
    echo Cleaning build folder...
    rmdir /s /q "%BUILD_DIR%"
)
echo Making new build folder...
mkdir "%BUILD_DIR%"

rem === COMPILING SOURCES ===
pushd %BUILD_DIR%
if /I "%COMPILE_MODE%"=="DEBUG" (
    echo Compiling for DEBUG...
    cl /Zi /FC /Od /EHsc %SRC_DIR%\*.cpp /Fe%EXE_NAME% /link User32.lib Gdi32.lib
) else if /I "%COMPILE_MODE%"=="RELEASE" (
    echo Compiling for RELEASE...
    cl /O2 /EHsc %SRC_DIR%\*.cpp /Fe%EXE_NAME% /link User32.lib Gdi32.lib
) else (
    popd
    echo ERROR: Unknown COMPILE_MODE "%COMPILE_MODE%". Must be DEBUG or RELEASE.
    exit /b 1
)
popd

rem === GENERATING compile_commands.json ===
echo Generating compile_commands.json...
(
    echo [
    set first=1
    for /r "%SRC_DIR%" %%f in (*.cpp) do (
        set "FILE=%%f"
        rem replace \ with / instead of \/ 
        set "FILE=!FILE:\=/!"

        set "PROJECT_PATH=%PROJECT_ROOT:~0,-1%"
        set "PROJECT_PATH=!PROJECT_PATH:\=/!"

        rem Output the compile command for each source file
        if "!first!"=="0" echo ,
        echo {
        echo     "directory": "!PROJECT_PATH!",
        echo     "command": "cl /EHsc /std:c++17 -c !FILE!",
        echo     "file": "!FILE!"
        echo }
        set first=0
    )
    echo ]
) > "%BUILD_DIR%\compile_commands.json"

rem === COPYING compile_commands.json TO PROJECT ROOT ===
echo Copying compile_commands.json to project root...
copy /Y "%BUILD_DIR%\compile_commands.json" "%PROJECT_ROOT%compile_commands.json" > nul

rem === BUILD COMPLETED ===
echo Build completed successfully! Run the executable from the build folder.
