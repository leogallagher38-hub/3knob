#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p test-build
CXX="${CXX:-c++}"
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    Tests/test_engine.cpp -o test-build/test_engine
./test-build/test_engine

# Memory / undefined behavior instrumentation, when supported.
if "$CXX" -std=c++17 -O1 -g -fsanitize=address,undefined \
       -fno-omit-frame-pointer Tests/test_engine.cpp -o test-build/test_engine_sanitized 2>/dev/null; then
    ASAN_OPTIONS=detect_leaks=1 ./test-build/test_engine_sanitized
else
    echo 'Sanitizers unavailable on this compiler; the normal tests still ran.'
fi
