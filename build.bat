@echo off
REM Build script for FFGUI++
REM This script launches MSYS2/MINGW64 and runs build.sh

set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

echo Script directory: %SCRIPT_DIR%

set MSYS2_PATH=C:\msys64\msys2_shell.cmd

if not exist "%MSYS2_PATH%" (
    echo Error: MSYS2 not found at %MSYS2_PATH%
    echo Please install MSYS2 or update the MSYS2_PATH variable in this script.
    pause
    exit /b 1
)

if not exist models (
    mkdir build\models
    git clone https://huggingface.co/codester2835/ffgui-models models
    cd models
    copy *.bin ..\build\models\
    cd ..
)

echo Launching MSYS2/MINGW64 and running build.sh...
echo Current directory: %SCRIPT_DIR%

"%MSYS2_PATH%" -defterm -no-start -mingw64 -here -c "cd '%SCRIPT_DIR:\=/%' && pwd && ls -la && bash build.sh"

if %ERRORLEVEL% NEQ 0 (
    echo Build process failed!
    echo Check the error messages above.
    pause
    exit /b %ERRORLEVEL%
)

echo Build completed successfully!
