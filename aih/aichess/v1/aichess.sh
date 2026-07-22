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
  --boards BOARDS
               Same as -nb.
  -nl LOOPS    Number of board batches to run. Defaults to 1 when omitted.
  --loops LOOPS
               Same as -nl.

Agent list:
  AGENTS       One to three agents.
               Defaults to qwen1 when omitted.
               1 agent: that agent fills White and Black; referee is harness.
               2 agents: first two agents are players; colors are randomized;
                         referee is harness.
               3 agents: first two agents are players; colors are randomized;
                         third agent is referee.
  --models MODELS
               Comma-separated model list used to resolve qwenN/agentN aliases.
  --list-models
               List detected qwenN/agentN aliases and exit.

Role override flags:
  -w MODELS    White player model(s)
  -b MODELS    Black player model(s)
  -r MODELS    Referee model(s)
  --white-models MODELS
               White player model list, assigned round-robin by board.
  --black-models MODELS
               Black player model list, assigned round-robin by board.
  --referee-models MODELS
               Referee model list, assigned round-robin by referee role.

Limit flags:
  --mode MODE  Runner mode: aichess, hallucination-game, class-game,
               one-move, game, or both.
  -mxply PLIES Override maximum plies.
  --mxply PLIES
               Same as -mxply.
  -mt SECONDS  Override per-move response window. Defaults to 10 seconds.
  --move-timeout SECONDS
               Same as -mt.
  -sto SECONDS Override agent stack timeout. Defaults to 60 seconds.
  --stack-timeout SECONDS
               Same as -sto.
  -otkns TOKENS
               Override agent output token budget. Defaults to 256 tokens.
  --otkns TOKENS
               Same as -otkns.
  -aot         Enable automatic output-token tuning. Disabled by default.
  --auto-output-tokens
               Same as -aot.
  -lkahdlvl LEVEL
               Agent chess look-ahead level requested in the move prompt.
  --lkahdlvl LEVEL
               Same as -lkahdlvl.
  -lokahdlvl LEVEL, --lokahdlvl LEVEL
               Compatibility alias for -lkahdlvl.
  -bap         Enable pre-move and post-move board-awareness audits.
  --board-awareness-probe
               Same as -bap.
  --loglvl LEVEL
               Logging verbosity from 0 to 5. 0 is least verbose; 5 is most verbose.
               Level 1 prints compact harness I/O records.
               Level 2 prints only ho/hi harness-Ollama I/O plus mv/rf strings.
               Level 3 adds diagnostics and result artifact paths.

Harness/config flags:
  --reference-config ID
               Label this harness/stack configuration as a reference point.
  --stack-module MODULE
               Select the harness adapter module for agent I/O.
  --stack-kind KIND
               Describe the stack category interacting with the harness.
  --stack-name NAME
               Describe the concrete stack interacting with the harness.
  -gmto SECONDS
               Override per-game timeout.
  --gmto SECONDS
               Same as -gmto.
  -cnrtlm TRIES
               Correction retry limit after illegal/invalid moves. Defaults to 1.
  --cnrtlm TRIES
               Same as -cnrtlm.
  --referee-count COUNT
               Referee votes recorded per board.
  --max-illegal COUNT
               Illegal/unparseable move limit before forfeit.
  --legal-list
               Assisted comparison mode. Include the legal UCI move list in the
               player prompt. Default Class1 mode sends board state only and
               derives mv from the returned bf/af transition.

Validation flags:
  --avb         Stockfish validates board/rules; referee agent validates moves.
  -avb          Same as --avb.
  --avm         Stockfish validates moves; referee agent validates board.
  -avm          Same as --avm.
  --ans         Agent-only validation when no Stockfish validation flag is set.
  -ans          Same as --ans.

Other:
  --dry-run     Print planned run without invoking an agent.
  --help-all    Pass through Qt extended help.
  -c, --class   Reserved for administrative use; use -nb for board count.
  -p, -t, -g    Reserved internal runner flags; use -mxply, -mt, or -gmto.
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

require_loglvl() {
  local flag="$1"
  local value="$2"
  require_int "$flag" "$value"
  if ((value < 0 || value > 5)); then
    echo "$flag requires a value from 0 through 5, got: $value" >&2
    usage >&2
    exit 2
  fi
}

is_openai_agent() {
  [[ "$1" == openai:* || "$1" == gpt-* || "$1" == chat-* ]]
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
stack_module=""
has_stack_kind=0
has_stack_name=0
loglvl=3
while (($#)); do
  case "$1" in
    -nb|--boards)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      boards="$2"
      shift 2
      ;;
    -nl|--loops)
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
    -aot|--auto-output-tokens)
      args+=("--auto-output-tokens")
      shift
      ;;
    -bap|--board-awareness-probe)
      args+=("--board-awareness-probe")
      shift
      ;;
    --loglvl)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_loglvl "$1" "$2"
      loglvl="$2"
      args+=("--loglvl" "$2")
      shift 2
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
    -mxply|--mxply)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("--mxply" "$2")
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
    -sto|--stack-timeout)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("--stack-timeout" "$2")
      shift 2
      ;;
    -otkns|--otkns)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("--otkns" "$2")
      shift 2
      ;;
    -gmto|--gmto)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("--gmto" "$2")
      shift 2
      ;;
    -cnrtlm|--cnrtlm)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("--cnrtlm" "$2")
      shift 2
      ;;
    -lkahdlvl|--lkahdlvl|-lokahdlvl|--lokahdlvl)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      require_int "$1" "$2"
      args+=("--lkahdlvl" "$2")
      shift 2
      ;;
    -p|-t|-g)
      echo "$1 is not a user-facing flag here. Use -mxply, -mt, or -gmto." >&2
      usage >&2
      exit 2
      ;;
    --move-timeout|--models|--mode|--referee-count|--max-illegal|--reference-config|--stack-module|--stack-kind|--stack-name)
      if (($# < 2)); then
        echo "Missing value for $1" >&2
        usage >&2
        exit 2
      fi
      if [[ "$1" == "--models" ]]; then
        has_models_override=1
      fi
      if [[ "$1" == "--stack-module" ]]; then
        stack_module="$2"
      fi
      if [[ "$1" == "--stack-kind" ]]; then
        has_stack_kind=1
      fi
      if [[ "$1" == "--stack-name" ]]; then
        has_stack_name=1
      fi
      args+=("$1" "$2")
      shift 2
      ;;
    --help-all)
      if [[ ! -x "$RUNNER" ]]; then
        make -C "$ROOT_DIR/qwen_ollama_chess_qt"
      fi
      cd "$ROOT_DIR/qwen_ollama_chess_qt"
      exec "$RUNNER" --help-all
      ;;
    --dry-run|--list-models|--legal-list)
      args+=("$1")
      shift
      ;;
    --help|/?)
      usage
      exit 0
      ;;
    -*)
      echo "Unknown flag: $1" >&2
      usage >&2
      exit 2
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

case "$stack_module" in
  openai|openai_responses|cloud_openai)
    if ((! has_stack_kind)); then
      args+=("--stack-kind" "openai_agentic_cloud")
    fi
    if ((! has_stack_name)); then
      args+=("--stack-name" "openai_responses_cloud")
    fi
    ;;
esac

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
        referee="harness"
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
        referee="harness"
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
    referee="harness"
  fi
  args=("-w" "$white" "-b" "$black" "-r" "$referee" "${args[@]}")
fi

if [[ -z "$stack_module" || "$stack_module" == "auto" || "$stack_module" == "agent_auto" || "$stack_module" == "auto_agent" ]]; then
  if is_openai_agent "${white:-}" || is_openai_agent "${black:-}" || is_openai_agent "${referee:-}"; then
    if ((! has_stack_kind)); then
      args+=("--stack-kind" "openai_agentic_cloud")
    fi
    if ((! has_stack_name)); then
      args+=("--stack-name" "openai_responses_cloud")
    fi
  fi
fi

if [[ "$(pwd -P)" == "$ROOT_DIR" ]]; then
  output_dir="runs/"
else
  output_dir="$ROOT_DIR/runs/"
fi
if ((loglvl >= 2)); then
  echo "AIChess run starting  boards: $boards  loops: $loops  outputs: $output_dir" >&2
  if ((${#agents[@]} > 0)); then
    echo "assignments: w=$white  b=$black  r=$referee" >&2
  else
    echo "assignments: ${args[*]}" >&2
  fi
  echo >&2
fi

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
