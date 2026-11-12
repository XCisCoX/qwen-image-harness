#!/usr/bin/env bash
# Build Qwen Image Harness on Linux (or any Unix with qmake).
#
#   ./build-linux.sh              # release build into Build-Linux/
#   BUILD_CONFIG=debug ./build-linux.sh
#   QMAKE=/opt/Qt/5.15.2/gcc_64/bin/qmake ./build-linux.sh
#
# The .pro file is shared with the Windows build, so no per-platform sources
# exist: this is the same application.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT}/Build-Linux}"
BUILD_CONFIG="${BUILD_CONFIG:-release}"
QMAKE="${QMAKE:-qmake}"

if ! command -v "${QMAKE}" >/dev/null 2>&1; then
    echo "[ERROR] qmake not found. Install qtbase5-dev (or set QMAKE=/path/to/qmake)." >&2
    exit 1
fi

echo "[INFO] qmake:         ${QMAKE}"
"${QMAKE}" -v | sed -n '1,3p' || true
echo "[INFO] build dir:     ${BUILD_DIR}"
echo "[INFO] configuration: ${BUILD_CONFIG}"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

"${QMAKE}" "${ROOT}/Src/QImageHarness.pro" "CONFIG+=${BUILD_CONFIG}"

JOBS="$(nproc 2>/dev/null || echo 4)"
if command -v make >/dev/null 2>&1; then
    make -j"${JOBS}"
else
    echo "[ERROR] make not found." >&2
    exit 1
fi

echo "[INFO] Build completed: ${BUILD_DIR}/QwenImageHarness"
