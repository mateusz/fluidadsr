#!/bin/bash
set -e

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure and build
cmake ..
make -j$(nproc)

echo ""
echo "Build complete. Run with: ./build/fluidadsr /path/to/soundfont.sf2"