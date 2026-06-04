#!/bin/bash
set -e
cd "$(dirname "$0")"
mkdir -p build
cd build
cmake ../cpp-application
make -j$(nproc)
echo "Build completed successfully"
