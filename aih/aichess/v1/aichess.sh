#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  ./aichess.sh qwen1
  ./aichess.sh qwen1 qwen2 qwen3
  ./aichess.sh -nb 2 -nl 3 qwen1 qwen2

Boards:
  -nb BOARDS   Number of boards to run. Defaults to 1 when omitted.
  -nl LOOPS    Number of board batches to run. Defaults to 1 when omitted.

Agent list:
  AGENTS       One to three agents.
               Defaults to qwen1 when omitted.
               1 agent: that agent fills White and Black; referee is stockfish.
               2 agents: first two agents are players; colors are randomized;
                         referee is stockfish.
               3 agents: first two agents are players; colors are randomized;
                         third agent is referee.

Role override flags:
  -w MODELS    White player model(s)
  -b MODELS    Black player model(s)
  -r MODELS    Referee model(s)

Limit flags:
  -mp PLIES    Override maximum plies
  -mt SECONDS  Override per-move timeout
  -gt SECONDS  Override per-game timeout
  -ncr TRIES   Correction retry limit after illegal/invalid moves. Defaults to 1.

Validation flags:
  --avb         Stockfish validates board/rules; referee agent validates moves.
  --avm         Stockfish validates moves; referee agent validates board.
  --ans         Agent-only validation when no Stockfish validation flag is set.

Other:
  --help, /?   Show this help
EOF
}

require_int() {
  local flag="$1"
  local value="$2"
  if [[ ! "$value" =~ ^[0-9]+$ ]]; then
    echo "$flag requires a whole number, got: $value" >&2
    usage >&2
    exit 2
  fi
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"
RUNNER="$ROOT_DIR/qwen_ollama_chess_qt/qwen_ollama_chess_qt"

boards="1"
loops="1"
args=()
agents=()
has_role_override=0
ans_requested=0
has_models_override=0
while (($#)); do
  case "$1" in
    -nb)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      boards="$2"
      shift 2
      ;;
    -nl)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      loops="$2"
      shift 2
      ;;
    -c|--class)
      echo "$1 is reserved for administrative use. Use -nb for board count." >&2
      usage >&2
      exit 2
      ;;
    --avb|--avm|--ans|-avb|-avm|-ans)
      case "$1" in
        -avb) args+=("--avb") ;;
        -avm) args+=("--avm") ;;
        -ans) args+=("--ans") ;;
        *) args+=("$1") ;;
      esac
      if [[ "$1" == "--ans" || "$1" == "-ans" ]]; then
        ans_requested=1
      fi
      shift
      ;;
    -w|--white|-b|--black|-r|--referee|--white-models|--black-models|--referee-models)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      has_role_override=1
      args+=("$1" "$2")
      shift 2
      ;;
    -mp)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("-p" "$2")
      shift 2
      ;;
    -mt)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("-t" "$2")
      shift 2
      ;;
    -gt)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("-g" "$2")
      shift 2
      ;;
    -ncr)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("--retries" "$2")
      shift 2
      ;;
    -nr)
      echo "$1 is not a user-facing flag here. Use -ncr." >&2
      usage >&2
      exit 2
      ;;
    -p|-t|-g)
      echo "$1 is not a user-facing flag here. Use -mp, -mt, or -gt." >&2
      usage >&2
      exit 2
      ;;
    --max-plies|--move-timeout|--game-timeout|--models|--mode|--boards|--referee-count|--max-illegal)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      if [[ "$1" == "--models" ]]; then
        has_models_override=1
      fi
      args+=("$1" "$2")
      shift 2
      ;;
    --dry-run|--list-models|--help-all)
      args+=("$1")
      shift
      ;;
    --help|/?)
      usage
      exit 0
      ;;
    *)
      agents+=("$1")
      shift
      ;;
  esac
done

if ((boards < 1 || boards > 999)); then
  echo "Invalid -nb BOARDS: $boards. Use 1 through 999." >&2
  usage >&2
  exit 2
fi
if ((loops < 1)); then
  echo "Invalid -nl LOOPS: $loops. Use 1 or more." >&2
  usage >&2
  exit 2
fi

if [[ ! -x "$RUNNER" ]]; then
  make -C "$ROOT_DIR/qwen_ollama_chess_qt"
fi

if ((${#agents[@]} > 0)); then
  if ((has_role_override)); then
    echo "Do not mix positional agents with -w/-b/-r role overrides." >&2
    usage >&2
    exit 2
  fi
  case "${#agents[@]}" in
    1)
      white="${agents[0]}"
      black="${agents[0]}"
      if ((ans_requested)); then
        referee="${agents[0]}"
      else
        referee="stockfish"
      fi
      ;;
    2|3)
      if ((RANDOM % 2)); then
        white="${agents[0]}"
        black="${agents[1]}"
      else
        white="${agents[1]}"
        black="${agents[0]}"
      fi
      if ((ans_requested)); then
        referee="${agents[1]}"
      else
        referee="stockfish"
      fi
      if ((${#agents[@]} == 3)); then
        referee="${agents[2]}"
      fi
      ;;
    *)
      echo "Too many positional agents. Use one, two, or three agents." >&2
      usage >&2
      exit 2
      ;;
  esac
  args=("-w" "$white" "-b" "$black" "-r" "$referee" "${args[@]}")
elif ((! has_role_override)); then
  agents=("qwen1")
  white="${agents[0]}"
  black="${agents[0]}"
  if ((ans_requested)); then
    referee="${agents[0]}"
  else
    referee="stockfish"
  fi
  args=("-w" "$white" "-b" "$black" "-r" "$referee" "${args[@]}")
fi

if [[ "$(pwd -P)" == "$ROOT_DIR" ]]; then
  output_dir="runs/"
else
  output_dir="$ROOT_DIR/runs/"
fi
echo "AIChess run starting  boards: $boards  loops: $loops  outputs: $output_dir" >&2
if ((${#agents[@]} > 0)); then
  echo "assignments: w=$white  b=$black  r=$referee" >&2
else
  echo "assignments: ${args[*]}" >&2
fi
echo >&2

if ((! has_models_override)); then
  detected_models="$(
    ollama list 2>/dev/null \
      | awk 'NR > 1 && tolower($1) ~ /qwen/ {
          size=$3
          unit=$4
          gb=size
          if (unit == "MB") gb=size/1024
          print gb "\t" $1
        }' \
      | sort -n \
      | awk '{print $2}' \
      | paste -sd, -
  )"
  if [[ -n "$detected_models" ]]; then
    args=("--models" "$detected_models" "${args[@]}")
  fi
fi

cd "$ROOT_DIR/qwen_ollama_chess_qt"
exec "$RUNNER" \
  --mode aichess \
  --boards "$boards" \
  --loops "$loops" \
  --referee-count 1 \
  --max-illegal 1 \
  "${args[@]}"
