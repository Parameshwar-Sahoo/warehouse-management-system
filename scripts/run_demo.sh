#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR/.."

echo "=========================================================="
echo "  WMS 6-STAGE DEMONSTRATION & VERIFICATION PIPELINE       "
echo "=========================================================="

echo "[1/4] Building Linux Character Device Driver..."
cd "$ROOT_DIR/driver"
make

echo "[2/4] Configuring & Building C++ Application Suite..."
cd "$ROOT_DIR"
mkdir -p build && cd build
cmake ..
make -j$(nproc)

echo "[3/4] Running Unit & Integration Test Suites..."
./tests/wms_tests

echo "[4/4] Executing Automated End-to-End System Demonstration..."
./wms_app --demo

echo "=========================================================="
echo "  STAGE DEMONSTRATION COMPLETED SUCCESSFULLY              "
echo "=========================================================="
