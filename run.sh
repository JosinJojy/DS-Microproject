#!/usr/bin/env bash
# MazeLab Linux/macOS startup script
# Usage: ./run.sh [port]

set -e

PORT="${1:-8080}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "============================================================"
echo "  MazeLab - Interactive Maze Solver"
echo "============================================================"

# Check for compiler
if command -v gcc >/dev/null 2>&1; then
    CC=gcc
elif command -v cc >/dev/null 2>&1; then
    CC=cc
else
    echo "ERROR: No C compiler found. Install gcc or clang."
    exit 1
fi

# Check for make
if command -v make >/dev/null 2>&1; then
    MAKE=make
else
    echo "ERROR: make not found."
    exit 1
fi

# Build if needed
if [ ! -f "mazelab" ]; then
    echo "Building MazeLab..."
    $MAKE all CC="$CC"
    echo "Build complete."
else
    echo "Using existing mazelab binary."
fi

echo ""
echo "  Server will start on: http://127.0.0.1:$PORT"
echo "  Open this URL in your browser."
echo "  Press Ctrl+C to stop the server."
echo "============================================================"
echo ""

./mazelab "$PORT"
