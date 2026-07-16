#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  "$SCRIPT_DIR/class1_aichess_fixture.cpp" \
  -o "$SCRIPT_DIR/class1_aichess_fixture"
