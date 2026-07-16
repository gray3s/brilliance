#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
RUNNER="$ROOT_DIR/qwen_ollama_chess_qt/qwen_ollama_chess_qt"

if [[ ! -x "$RUNNER" ]]; then
  make -C "$ROOT_DIR/qwen_ollama_chess_qt"
fi

exec "$RUNNER" \
  --mode aichess \
  --boards 1 \
  --referee-count 1 \
  --max-illegal 1 \
  "$@"
