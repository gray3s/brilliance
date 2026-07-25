#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./run_aih_v2_eval.sh --summary
  ./run_aih_v2_eval.sh --tables

No Python is used by this wrapper. The current AIH v2 prototype is stored as
CSV/JSONL database tables plus SQL schema.
EOF
}

line_count() {
  local file="$1"
  local lines
  lines="$(wc -l < "$file")"
  printf '%s\t%s\n' "$lines" "$(basename "$file")"
}

case "${1:---summary}" in
  --summary)
    echo "AIH v2 table snapshot"
    line_count "$SCRIPT_DIR/agents.csv"
    line_count "$SCRIPT_DIR/tests.csv"
    line_count "$SCRIPT_DIR/results.csv"
    line_count "$SCRIPT_DIR/class3_certifications.csv"
    echo
    sed -n '1,80p' "$SCRIPT_DIR/run_summary.json"
    ;;
  --tables)
    find "$SCRIPT_DIR" -maxdepth 1 -type f \
      \( -name '*.csv' -o -name '*.jsonl' -o -name '*.json' -o -name '*.sql' -o -name 'README.md' \) \
      | sort
    ;;
  --help|-h|/\?)
    usage
    ;;
  *)
    usage >&2
    exit 2
    ;;
esac
