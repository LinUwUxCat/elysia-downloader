#!/bin/bash

# Elysia Downloader Build Script

echo "Building Elysia Downloader..."

# Check if build directory exists, create if not
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

# Change to build directory
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build the application
echo "Building application..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build completed successfully!"
echo "You can now run the application with: ./build/ElysiaDownloader"


