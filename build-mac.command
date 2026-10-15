#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

echo '=== 3 KNOB by Leo Gallagher: compact AU build ==='
if [[ "$(uname -s)" != "Darwin" ]]; then
    echo 'This script must run on macOS.'; exit 1
fi

# Xcode's full graphical app is not needed for a CMake Makefiles build.
if ! xcrun --find clang++ >/dev/null 2>&1; then
    echo 'Apple Command Line Tools are missing. Install them with:'
    echo '  xcode-select --install'
    echo 'No full Xcode installation is needed.'
    exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
    echo 'CMake 3.22+ is needed. Install the compatible macOS version from cmake.org.'
    exit 1
fi
if ! command -v make >/dev/null 2>&1; then
    echo 'The make tool is missing. Install Apple Command Line Tools via xcode-select --install.'
    exit 1
fi

CXX="$(xcrun --find clang++)"
echo 'Running DSP smoke tests...'
mkdir -p test-build
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    Tests/test_engine.cpp -o test-build/test_engine
./test-build/test_engine

ARCH="$(uname -m)"
echo "Compiling native $ARCH AU using Apple Command Line Tools (without full Xcode)..."
# Use a separate directory so any prior Xcode-generator build cache is harmless.
cmake -S . -B build-clt -G 'Unix Makefiles' \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
cmake --build build-clt --config Release --target ThreeKnob_AU --parallel 2

COMPONENT="$(find build-clt -type d -name '3 KNOB.component' -print -quit)"
if [[ -z "$COMPONENT" ]]; then
    echo 'Compilation ended, but 3 KNOB.component was not found.'; exit 1
fi
DEST="$HOME/Library/Audio/Plug-Ins/Components/3 KNOB.component"
mkdir -p "$(dirname "$DEST")"
rm -rf "$DEST"
ditto "$COMPONENT" "$DEST"
codesign --force --deep --sign - "$DEST"
echo "Installed: $DEST"

if command -v auval >/dev/null 2>&1; then
    echo 'Validating Audio Unit with auval...'
    auval -v aufx Knob Lega
else
    echo 'auval is not installed; use Logic Pro Plug-in Manager to validate.'
fi
printf '\nDone. Restart Logic and find Audio FX > Audio Units > Leo Gallagher > 3 KNOB.\n'
