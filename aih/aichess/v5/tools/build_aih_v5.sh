#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT_DIR/tools/script_binary_launcher.cpp"
OUT_DIR="$ROOT_DIR/bin"
CXX="${CXX:-c++}"
CXXFLAGS_DEFAULT="-O2 -std=c++17 -Wall -Wextra"

mkdir -p "$OUT_DIR"
chmod +x "$ROOT_DIR/aih_v5.sh"

"$CXX" ${CXXFLAGS:-$CXXFLAGS_DEFAULT} \
  "-DSCRIPT_REL_PATH=\"aih_v5.sh\"" \
  "$SRC" \
  -o "$OUT_DIR/aih_v5"

chmod +x "$OUT_DIR/aih_v5"
printf '%s -> %s\n' "$OUT_DIR/aih_v5" "aih_v5.sh"

"$CXX" ${CXXFLAGS:-$CXXFLAGS_DEFAULT} \
  "$ROOT_DIR/tools/generate_aih_v5_html_report.cpp" \
  -o "$OUT_DIR/aih_v5_html_report"

chmod +x "$OUT_DIR/aih_v5_html_report"
printf '%s -> %s\n' "$OUT_DIR/aih_v5_html_report" "tools/generate_aih_v5_html_report.cpp"

"$CXX" ${CXXFLAGS:-$CXXFLAGS_DEFAULT} \
  "$ROOT_DIR/tools/generate_aih_v5_repeat_html.cpp" \
  -o "$OUT_DIR/aih_v5_repeat_html"

chmod +x "$OUT_DIR/aih_v5_repeat_html"
printf '%s -> %s\n' "$OUT_DIR/aih_v5_repeat_html" "tools/generate_aih_v5_repeat_html.cpp"

"$CXX" ${CXXFLAGS:-$CXXFLAGS_DEFAULT} \
  "$ROOT_DIR/tools/run_aih_v5_single_game.cpp" \
  -o "$OUT_DIR/aih_v5_single_game"

chmod +x "$OUT_DIR/aih_v5_single_game"
printf '%s -> %s\n' "$OUT_DIR/aih_v5_single_game" "tools/run_aih_v5_single_game.cpp"
