#!/bin/bash

# Create the build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

# Change to the build directory
cd build

# Run CMake to generate the build files
cmake ..

# Compile the code
make


