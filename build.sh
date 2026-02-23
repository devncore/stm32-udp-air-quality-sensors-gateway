#!/bin/bash
# Incremental build script for Air_Quality_Server

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"

# Configure if needed (only runs if CMakeCache.txt doesn't exist)
if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    echo "Configuring CMake..."
    cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}"
fi

# Build
echo "Building..."
cmake --build "${BUILD_DIR}" -j$(nproc)

echo "Build complete!"
