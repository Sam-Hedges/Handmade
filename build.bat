@echo off
rem ============================================================================
rem BUILD.BAT - Simple C++ Project Build Script for MSVC on Windows
rem ============================================================================

rem === SET UP PROJECT VARIABLES ===
setlocal enabledelayedexpansion
set "EXE_NAME=Game.exe"
set "STATIC_LIBS=User32.lib Gdi32.lib"
set "PROJECT_ROOT=%~dp0"
set "BUILD_DIR=%PROJECT_ROOT%build"
set "SRC_DIR=%PROJECT_ROOT%src"
set "COMPILE_MODE=DEBUG"

rem === SDL CONFIGURATION ===
set "SDL_DIR=C:\Libraries\SDL3-devel-3.2.24-VC\SDL3-3.2.24"
set "SDL_INCLUDE=%SDL_DIR%\include"
set "SDL_LIB=%SDL_DIR%\lib\x64\SDL3.lib"
set "SDL_DLL=%SDL_LIB%\SDL3.dll"

rem === ENSURE BUILD FOLDER EXISTS (UPDATED) ===
if not exist "%BUILD_DIR%" (
    echo Creating build folder...
    mkdir "%BUILD_DIR%"
) else (
    echo Cleaning build folder but keeping SDL3.dll...
    pushd "%BUILD_DIR%"
    for %%f in (*.*) do (
        if /I not "%%f"=="SDL3.dll" (
            del /Q "%%f"
        )
    )
    popd
)

rem === COPY SDL3.dll IF MISSING (NEW) ===
if not exist "%BUILD_DIR%\SDL3.dll" (
    echo Copying SDL3.dll into build folder...
    copy /Y "%SDL_DLL%" "%BUILD_DIR%" > nul
)

rem === COMPILING SOURCES ===
rem W4     Enables high warning verbosity [0–4] messages when compiling.
rem Zi     Generate complete debugging information (PDB File).
rem FC     Shows full file paths in compiler error/warning messages instead of just filenames.
rem Od     Disable optimizations, easier to debug and avoids compiler fiddling with variables.
rem EHsc   Tells the compiler to assume exceptions only come from C++ throw.
rem MDd    Use debug multithreaded DLL runtime, fill heap memory with patterns like 0xCC.
rem RTC1   Enable basic runtime checks, helps detect stack corruption and use of uninitialized locals.
rem GS     Enable buffer security checks, helps detect stack corruption and use of uninitialized locals.
rem O2     Is shorthand for enabling a group of specific optimization flags. Look them up.

pushd "%BUILD_DIR%"
if /I "%COMPILE_MODE%"=="DEBUG" (
    echo Compiling for DEBUG...
    cl -W4 -Zi -FC -Od -EHsc -MDd -RTC1 -GS -I"%SDL_INCLUDE%" "%SRC_DIR%\*.cpp" -Fe%EXE_NAME% -link %SDL_LIB% %STATIC_LIBS%
) else if /I "%COMPILE_MODE%"=="RELEASE" (
    echo Compiling for RELEASE...
    cl -O2 -EHsc -I"%SDL_INCLUDE%" "%SRC_DIR%\*.cpp" -Fe%EXE_NAME% -link %SDL_LIB% %STATIC_LIBS%
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
        echo     "command": "cl -I!SDL_INCLUDE:\=/! -EHsc -std:c++17 -c !FILE!",
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
