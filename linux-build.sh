#!/bin/bash

echo "Starting FFGUI++ Native Linux Build Process..."

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script directory: $SCRIPT_DIR"

if [ ! -d "models" ]; then
    echo "Cloning GGML Models repository..."
    git clone https://huggingface.co/codester2835/ffgui-models models
else
    echo "Models folder already exists, skipping clone."
fi

mkdir -p build/models
if [ -d "models" ]; then
    cp models/*.bin build/models/ 2>/dev/null || echo "No .bin files to copy"
fi

echo "Installing required packages..."
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  qt6-base-dev \
  qt6-tools-dev \
  pkg-config \
  zlib1g-dev \
  libbz2-dev \
  liblzma-dev \
  zip \
  p7zip-full

if [ ! -d "src" ] || [ ! -f "src/main.cpp" ]; then
    echo "Error: Source files not found in src/ directory!"
    exit 1
fi

echo "Source files found:"
ls -la src/

echo "Cleaning up Windows files that cause build issues..."
find . -name "*:Zone.Identifier" -delete 2>/dev/null || true
find . -name "*.exe" -delete 2>/dev/null || true
find . -name "*Zone.Identifier*" -delete 2>/dev/null || true


echo "Cleaning build directory..."
cd build
shopt -s extglob
find . -mindepth 1 -maxdepth 1 ! -name 'models' ! -name '.' -exec rm -rf {} +
cd ..

echo "Configuring project with CMake (Unix Makefiles)..."
cd build

cmake "$SCRIPT_DIR" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

echo "Building project..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed! Let's check for problematic files..."
    
    echo "Checking for Windows-related files..."
    find .. -name "*:*" -o -name "*.exe" -o -name "*Zone.Identifier*"
    
    echo "Trying single-thread build..."
    make -j1
    
    if [ $? -ne 0 ]; then
        echo "Build still failing. Checking dependency files..."
        if [ -f "CMakeFiles/FFGUI++.dir/compiler_depend.make" ]; then
            echo "Problematic compiler_depend.make contents:"
            grep -n ":" CMakeFiles/FFGUI++.dir/compiler_depend.make | head -5
        fi
        exit 1
    fi
fi

find ../bin -type f ! -name "*.*" -exec cp {} . \;


echo "Build completed successfully!"
echo "Go to the 'build/' directory and run the executable"
echo "Executable name: FFGUI++"

echo "Contents of build/:"
find . -type f -executable | sort
