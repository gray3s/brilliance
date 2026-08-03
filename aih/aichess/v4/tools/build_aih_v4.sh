#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT_DIR/tools/script_binary_launcher.cpp"
OUT_DIR="$ROOT_DIR/bin"
CXX="${CXX:-g++}"
CXXFLAGS_DEFAULT="-O2 -std=c++17 -Wall -Wextra"

mkdir -p "$OUT_DIR"
chmod +x "$ROOT_DIR/aih_v4.sh"

make -C "$ROOT_DIR/qwen_ollama_chess_qt"

"$CXX" ${CXXFLAGS:-$CXXFLAGS_DEFAULT} \
  "-DSCRIPT_REL_PATH=\"aih_v4.sh\"" \
  "$SRC" \
  -o "$OUT_DIR/aih_v4"

chmod +x "$OUT_DIR/aih_v4"
printf '%s -> %s\n' "$OUT_DIR/aih_v4" "aih_v4.sh"
