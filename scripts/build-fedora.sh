#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if ! command -v cmake >/dev/null 2>&1; then
  echo "error: cmake is required" >&2
  exit 1
fi
if ! command -v ninja >/dev/null 2>&1; then
  echo "error: ninja-build is required" >&2
  exit 1
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}" --parallel

echo
echo "Build complete: ${BUILD_DIR}/orbit-shell"
echo "Run with: ${BUILD_DIR}/orbit-shell"
