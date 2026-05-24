#!/bin/bash
# Build script for FFGUI++
# This script runs in MSYS2/MINGW64 environment

echo "Installing necessary PacMan packages..."
if ! pacman -Qi cmake >/dev/null 2>&1; then
    pacman -S mingw-w64-x86_64-qt6 mingw-w64-x86_64-cmake --noconfirm
fi

echo "Downloading large binaries"
curl -L -o bin/ffmpeg.exe "https://github.com/Jsoeph192/FFGUI/releases/download/Binaries/ffmpeg.exe"
curl -L -o bin/ffplay.exe "https://github.com/Jsoeph192/FFGUI/releases/download/Binaries/ffplay.exe"
curl -L -o bin/ffprobe.exe "https://github.com/Jsoeph192/FFGUI/releases/download/Binaries/ffprobe.exe"
curl -L -o bin/ffmpeg "https://github.com/Jsoeph192/FFGUI/releases/download/Binaries/ffmpeg"
curl -L -o bin/ffplay "https://github.com/Jsoeph192/FFGUI/releases/download/Binaries/ffplay"
curl -L -o bin/ffprobe "https://github.com/Jsoeph192/FFGUI/releases/download/Binaries/ffprobe"

echo "Changing to build directory..."
mkdir build
cd build

echo "Cleaning build directory..."

shopt -s extglob

rm -rf build/!(models)


echo "Running CMake configuration..."
cmake .. -G "MinGW Makefiles"

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    read -p "Press enter to continue..."
    exit 1
fi

echo "Building project..."
cmake --build . --config Release

if [ $? -ne 0 ]; then
    echo "Build failed!"
    read -p "Press enter to continue..."
    exit 1
fi

echo "Creating Setup File"
cp ../installer.iss installer.iss
cp ../LICENSE LICENSE
"/c/Program Files (x86)/Inno Setup 6/ISCC.exe" "installer.iss"

echo "Build completed successfully! Go to the build/ directory and run FFGUI++.exe"
