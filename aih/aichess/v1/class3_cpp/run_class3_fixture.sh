#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY="$SCRIPT_DIR/class3_aichess_fixture"

if [[ ! -x "$BINARY" ]]; then
  "$SCRIPT_DIR/build_class3_fixture.sh"
fi

exec "$BINARY" "$@"
