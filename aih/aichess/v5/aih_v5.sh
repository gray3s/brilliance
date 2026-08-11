#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
ENGINE="$ROOT_DIR/qwen_ollama_chess_qt/qwen_ollama_chess_qt"
case "$ROOT_DIR" in
  */aih/aichess/v5) ;;
  *)
    echo "aih_v5: refusing to run outside the AIH v5 tree: $ROOT_DIR" >&2
    exit 127
    ;;
esac
case "$ENGINE" in
  "$ROOT_DIR"/*) ;;
  *)
    echo "aih_v5: refusing engine outside the AIH v5 tree: $ENGINE" >&2
    exit 127
    ;;
esac
AIH_V5_START_SECONDS=$SECONDS
AIH_V5_START_EPOCH="$(date '+%s')"
AIH_V5_START_TIMESTAMP="$(date '+%Y-%m-%d %H:%M:%S %Z')"
AIH_V5_RUN_STAMP="$(date '+%Y%m%d_%H%M%S')"
AIH_V5_TERMINAL_LOG_ENABLED="${AIH_V5_TERMINAL_LOG:-1}"
for _aih_v5_arg in "$@"; do
  case "$_aih_v5_arg" in
    --no-terminal-log|--no-output-log)
      AIH_V5_TERMINAL_LOG_ENABLED=0
      ;;
    --terminal-log|--output-log)
      AIH_V5_TERMINAL_LOG_ENABLED=1
      ;;
  esac
done
if [[ "$AIH_V5_TERMINAL_LOG_ENABLED" == "1" ||
      "$AIH_V5_TERMINAL_LOG_ENABLED" == "yes" ||
      "$AIH_V5_TERMINAL_LOG_ENABLED" == "true" ]]; then
  AIH_V5_TERMINAL_LOG_DIR="${AIH_V5_TERMINAL_LOG_DIR:-$ROOT_DIR/logs}"
  mkdir -p "$AIH_V5_TERMINAL_LOG_DIR"
  AIH_V5_TERMINAL_LOG_PATH="${AIH_V5_TERMINAL_LOG_PATH:-$AIH_V5_TERMINAL_LOG_DIR/aih_v5_terminal_${AIH_V5_RUN_STAMP}.log}"
  exec > >(tee -a "$AIH_V5_TERMINAL_LOG_PATH") 2>&1
  echo "aih_v5: terminal log: $AIH_V5_TERMINAL_LOG_PATH"
fi
echo "start timestamp: $AIH_V5_START_TIMESTAMP" >&2
echo "aih_v5: starting..." >&2
if [[ -n "${AIH_V5_REPEAT_RUN_INDEX:-}" ]]; then
  if [[ -n "${AIH_V5_REPEAT_RUN_COUNT:-}" ]]; then
    echo "aih_v5: repeat run ${AIH_V5_REPEAT_RUN_INDEX}/${AIH_V5_REPEAT_RUN_COUNT}" >&2
  elif [[ -n "${AIH_V5_REPEAT_MINREGS:-}" ]]; then
    echo "aih_v5: repeat run ${AIH_V5_REPEAT_RUN_INDEX} for minregs=${AIH_V5_REPEAT_MINREGS}" >&2
  else
    echo "aih_v5: repeat run ${AIH_V5_REPEAT_RUN_INDEX}" >&2
  fi
fi
if [[ -z "${AIH_V5_ORIGINAL_LAUNCH_STRING:-}" ]]; then
  printf -v AIH_V5_ORIGINAL_LAUNCH_STRING '%q' "$0"
  for arg in "$@"; do
    printf -v _aih_v5_arg '%q' "$arg"
    AIH_V5_ORIGINAL_LAUNCH_STRING+=" $_aih_v5_arg"
  done
fi
aih_v5_print_exit_footer() {
  local status=$?
  if [[ "${AIH_V5_SUPPRESS_EXIT_FOOTER:-0}" == "1" ]]; then
    return "$status"
  fi
  local elapsed=$((SECONDS - AIH_V5_START_SECONDS))
  echo "end timestamp: $(date '+%Y-%m-%d %H:%M:%S %Z')" >&2
  printf 'elapsed time notes: runtime=%02d:%02d elapsed_seconds=%d\n' $((elapsed / 60)) $((elapsed % 60)) "$elapsed" >&2
  echo "aih_v5: launch: $AIH_V5_ORIGINAL_LAUNCH_STRING" >&2
  return "$status"
}
trap aih_v5_print_exit_footer EXIT
LOCAL_AGENT_REGISTRY="${AIH_V5_LOCAL_AGENT_REGISTRY:-$ROOT_DIR/qualification_cache/local_qualification_20260729032018.csv}"
LOCAL_AGENT_REGISTRY="${AIH_V5_LOCAL_AGENT_REGISTRY:-$LOCAL_AGENT_REGISTRY}"
DEFAULT_LOCAL_AGENTS="${AIH_V5_DEFAULT_LOCAL_AGENTS:-gemma3:270m,qwen2.5:0.5b,smollm2:135m,llama3.2:1b,gemma3:1b,phi3:mini,mistral:latest,gemma3:4b}"
REGISTRATION_STATUS_CSV="${AIH_V5_REGISTRATION_STATUS_CSV:-$ROOT_DIR/AIH_V5_REGISTRATION_STATUS.csv}"
REGISTRATION_DIAGNOSTIC_LOG="${AIH_V5_REGISTRATION_DIAGNOSTIC_LOG:-$ROOT_DIR/AIH_V5_REGISTRATION_DIAGNOSTICS.log}"
REGISTRATION_SMOKE_ENABLED="${AIH_V5_REGISTRATION_SMOKE_ENABLED:-1}"
REGISTRATION_TIMEOUT_SECONDS="${AIH_V5_REGISTRATION_TIMEOUT_SECONDS:-5}"
REGISTRATION_DYNAMIC_TIMEOUT_ENABLED="${AIH_V5_REGISTRATION_DYNAMIC_TIMEOUT:-1}"
REGISTRATION_DYNAMIC_TIMEOUT_MULTIPLIER="${AIH_V5_REGISTRATION_DYNAMIC_TIMEOUT_MULTIPLIER:-2}"
REGISTRATION_DYNAMIC_TIMEOUT_MIN_SECONDS="${AIH_V5_REGISTRATION_DYNAMIC_TIMEOUT_MIN_SECONDS:-5}"
REGISTRATION_MODE="${AIH_V5_REGISTRATION_MODE:-liveness}"
REGISTRATION_CANDIDATE_COUNT="${AIH_V5_REGISTRATION_CANDIDATE_COUNT:-all}"
REGISTRATION_ORDER="${AIH_V5_REGISTRATION_ORDER:-random}"
REGISTRATION_PROMPT="${AIH_V5_REGISTRATION_PROMPT:-OK}"
REGISTRATION_NUM_PREDICT="${AIH_V5_REGISTRATION_NUM_PREDICT:-4}"
REGISTRATION_GAME_NUM_PREDICT="${AIH_V5_REGISTRATION_GAME_NUM_PREDICT:-128}"
REGISTRATION_GAME_SMOKE_PLIES="${AIH_V5_REGISTRATION_GAME_SMOKE_PLIES:-1}"
REGISTRATION_FINAL_RETRY="${AIH_V5_REGISTRATION_FINAL_RETRY:-1}"
REGISTRATION_KEEP_ALIVE="${AIH_V5_REGISTRATION_KEEP_ALIVE:-0s}"
REGISTRATION_SYSTEMIC_TIMEOUT_THRESHOLD="${AIH_V5_REGISTRATION_SYSTEMIC_TIMEOUT_THRESHOLD:-2}"
REGISTRATION_SYSTEMIC_PASS_RATE="${AIH_V5_REGISTRATION_SYSTEMIC_PASS_RATE:-75}"
REGISTRATION_STOP_AFTER_PASSES="${AIH_V5_REGISTRATION_STOP_AFTER_PASSES:-0}"
REGISTRATION_BATCH_SIZE="${AIH_V5_REGISTRATION_BATCH_SIZE:-5}"
REGISTRATION_STACK_RESET_SETTLE_SECONDS="${AIH_V5_REGISTRATION_STACK_RESET_SETTLE_SECONDS:-5}"
REGISTRATION_BATCH_SETTLE_SECONDS="${AIH_V5_REGISTRATION_BATCH_SETTLE_SECONDS:-$REGISTRATION_STACK_RESET_SETTLE_SECONDS}"
REGISTRATION_MIN_PASSES="${AIH_V5_REGISTRATION_MIN_PASSES:-4}"
REGISTRATION_UNLOAD_TIMEOUT_SECONDS="${AIH_V5_REGISTRATION_UNLOAD_TIMEOUT_SECONDS:-10}"
OPEN_HTML_REPORT="${AIH_V5_OPEN_HTML_REPORT:-1}"
KILL_STALE_V5_RUNS="${AIH_V5_KILL_STALE_RUNS:-1}"
OLLAMA_NUM_THREAD="${AIH_V5_OLLAMA_NUM_THREAD:-1}"
STARTING_TOKENS_PER_INPUT="${AIH_V5_STARTING_TOKENS_PER_INPUT:-${AIH_V5_DEFAULT_STARTING_TOKENSPERINPUT:-1024}}"
TOKEN_INCREASE_RATIO="${AIH_V5_TOKEN_INCREASE_RATIO:-${AIH_V5_DEFAULT_TOKEN_INCREASE_RATIO:-2.0}}"
TOKEN_DECREASE_STEP="${AIH_V5_TOKEN_DECREASE_STEP:-5}"
TOKEN_INCREASE_STEP="${AIH_V5_TOKEN_INCREASE_STEP:-10}"
DYNAMIC_NICE_ENABLED="${AIH_V5_DYNAMIC_NICE:-1}"
NICE_INITIAL="${AIH_V5_NICE_INITIAL:-5}"
NICE_STEP="${AIH_V5_NICE_STEP:-5}"
NICE_MAX="${AIH_V5_NICE_MAX:-15}"
NICE_STEP_SECONDS="${AIH_V5_NICE_STEP_SECONDS:-30}"
RENICE_OLLAMA_ENABLED="${AIH_V5_RENICE_OLLAMA:-1}"
LATEST_HTML_REPORT="$ROOT_DIR/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html"
REGISTRATION_ONLY=0
LOCAL_SMOKE=1
CLOUD_SMOKE_PROVIDER=""
CLOUD_REPRESENTATIVE_PROVIDER=""
SMOKE_STAGE="${AIH_V5_SMOKE_STAGE:-local-retry}"
PUBLISH_LATEST_ONLY=0
REPEAT_RUNS="${AIH_V5_NRUNS:-1}"
MIN_REGISTRATIONS="${AIH_V5_MINREGS:-0}"
PASSTHRU_ARGS=()
ENABLE_BOARD_AWARENESS="${AIH_V5_BOARD_AWARENESS_PROBE:-0}"
ENABLE_REASONING_MATRIX="${AIH_V5_ENABLE_REASONING_MATRIX:-0}"
REASONING_RANGE="${AIH_V5_ALLOWED_REASONINGS:-${AIH_V5_REASONING_RANGE:-}}"
VERBOSITY_RANGE="${AIH_V5_ALLOWED_VERBOSITY:-${AIH_V5_TEXT_VERBOSITY_RANGE:-}}"
EXPLICIT_VERBOSITY_RANGE=0
if [[ -n "${AIH_V5_ALLOWED_VERBOSITY:-}" || -n "${AIH_V5_TEXT_VERBOSITY_RANGE:-}" ]]; then
  EXPLICIT_VERBOSITY_RANGE=1
fi

cleanup_stale_v5_processes() {
  local engine_path="$1"
  local stale_engines child_pids pid child
  if [[ "$KILL_STALE_V5_RUNS" != "1" && "$KILL_STALE_V5_RUNS" != "yes" && "$KILL_STALE_V5_RUNS" != "true" ]]; then
    return 0
  fi
  stale_engines="$(
    ps -eo pid,ppid,comm,args |
      awk -v self="$$" -v engine="$engine_path" '
        $1 != self && index($0, engine) && $3 ~ /^qwen_ollama/ { print $1 }
      '
  )"
  if [[ -z "$stale_engines" ]]; then
    return 0
  fi
  echo "aih_v5: stale v5 engine process(es) detected before startup: $stale_engines" >&2
  for pid in $stale_engines; do
    child_pids="$(
      ps -eo pid,ppid,comm,args |
        awk -v ppid="$pid" '$2 == ppid && $3 == "curl" && index($0, "127.0.0.1:11434/api/generate") { print $1 }'
    )"
    for child in $child_pids; do
      echo "aih_v5: killing stale v5 curl child pid=$child parent=$pid" >&2
      kill -9 "$child" 2>/dev/null || true
    done
    echo "aih_v5: killing stale v5 engine pid=$pid" >&2
    kill -9 "$pid" 2>/dev/null || true
  done
  sleep 1
}

cleanup_stale_v5_processes "$ENGINE"

log_startup_ollama_state() {
  if ! command -v ollama >/dev/null 2>&1; then
    echo "aih_v5: startup ollama ps: ollama command not found" >&2
    return 0
  fi
  echo "aih_v5: startup ollama ps:" >&2
  if command -v timeout >/dev/null 2>&1; then
    timeout 5 ollama ps >&2 || echo "aih_v5: startup ollama ps failed or timed out" >&2
  else
    ollama ps >&2 || echo "aih_v5: startup ollama ps failed" >&2
  fi
}

log_startup_ollama_state

AIH_V5_LOCAL_CLOUD_MAXPLY_RATIO="${AIH_V5_LOCAL_CLOUD_MAXPLY_RATIO:-4}"
AIH_V5_LOCAL_MAXPLY_CAP="${AIH_V5_LOCAL_MAXPLY_CAP:-16}"
AIH_V5_CLOUD_MAXPLY_CAP="${AIH_V5_CLOUD_MAXPLY_CAP:-10}"
AIH_V5_BOARD_CONCURRENCY="${AIH_V5_BOARD_CONCURRENCY:-1}"
TOURNAMENT_FORMAT="${AIH_V5_TOURNAMENT_FORMAT:-top4-ladder-rungs}"
RANKING_MODE="${AIH_V5_RANKING_MODE:-weighted}"
RANKING_AIH_WEIGHT="${AIH_V5_RANKING_AIH_WEIGHT:-0.5}"
RANKING_TURN_TIME_WEIGHT="${AIH_V5_RANKING_TURN_TIME_WEIGHT:-0.5}"
EXPLICIT_TURN_TIME_WEIGHT=0
LADDER_RUNGS="${AIH_V5_LADDER_RUNGS:-3}"
ROUND_ROBIN_ROUNDS="${AIH_V5_ROUND_ROBIN_ROUNDS:-1}"

has_env() {
  local name="$1"
  [[ -n "${!name:-}" ]]
}

prepare_provider_key() {
  local provider="$1"
  case "$provider" in
    openai)
      if ! has_env OPENAI_API_KEY; then
        echo "aih_v5: OPENAI_API_KEY is not exported; openai cloud smoke cannot test authorization." >&2
        exit 2
      fi
      ;;
    google|gemini)
      if ! has_env GEMINI_API_KEY && has_env GOOGLE_API_KEY; then
        export GEMINI_API_KEY="$GOOGLE_API_KEY"
      fi
      if ! has_env GEMINI_API_KEY && has_env GOOGLE_GENAI_API_KEY; then
        export GEMINI_API_KEY="$GOOGLE_GENAI_API_KEY"
      fi
      if ! has_env GEMINI_API_KEY; then
        echo "aih_v5: GEMINI_API_KEY is not exported; google/gemini cloud smoke cannot test authorization." >&2
        echo "aih_v5: GOOGLE_API_KEY or GOOGLE_GENAI_API_KEY may be used as a fallback source for GEMINI_API_KEY." >&2
        exit 2
      fi
      ;;
    anthropic)
      if ! has_env ANTHROPIC_API_KEY; then
        echo "aih_v5: ANTHROPIC_API_KEY is not exported; anthropic cloud smoke cannot test authorization." >&2
        exit 2
      fi
      ;;
  esac
}

spec_has_provider() {
  local provider="$1"
  local spec="$2"
  case "$provider" in
    openai)
      [[ "$spec" == *openai:* || "$spec" == *codex:* ||
         "$spec" =~ (^|[[:space:],:=])(gpt-|chatgpt-|codex-cli|codex)([[:space:],:]|$) ]]
      ;;
    google|gemini)
      [[ "$spec" == *gemini:* || "$spec" == *google:* ||
         "$spec" =~ (^|[[:space:],:=])(gemini-cli|gemini-)([[:space:],:]|$) ]]
      ;;
    anthropic)
      [[ "$spec" == *anthropic:* ||
         "$spec" =~ (^|[[:space:],:=])(claude-)([[:space:],:]|$) ]]
      ;;
    *)
      return 1
      ;;
  esac
}

prepare_keys_for_specs() {
  local spec="$1"
  if spec_has_provider openai "$spec"; then
    prepare_provider_key openai
  fi
  if spec_has_provider google "$spec"; then
    prepare_provider_key google
  fi
  if spec_has_provider anthropic "$spec"; then
    prepare_provider_key anthropic
  fi
}

for arg in "$@"; do
  case "$arg" in
    --tournament-format=*)
      TOURNAMENT_FORMAT="${arg#*=}"
      ;;
    --ladder)
      TOURNAMENT_FORMAT="ladder"
      ;;
    --round-robin)
      TOURNAMENT_FORMAT="round-robin"
      ;;
    --round-robin-ladder|--hybrid)
      TOURNAMENT_FORMAT="round-robin-ladder"
      ;;
    --top4-ladder-rungs|--top4-ladder-round-robin|--ladder-top4-rungs)
      TOURNAMENT_FORMAT="top4-ladder-rungs"
      ;;
    --ranking-mode=*)
      RANKING_MODE="${arg#*=}"
      ;;
    --ranking-aih-weight=*)
      RANKING_AIH_WEIGHT="${arg#*=}"
      ;;
    --ranking-turn-time-weight=*)
      RANKING_TURN_TIME_WEIGHT="${arg#*=}"
      EXPLICIT_TURN_TIME_WEIGHT=1
      ;;
    --weighted-aih=*|--ranking-weighted-aih=*|--AIH_weight=*|--aih-weight=*)
      RANKING_MODE="weighted"
      RANKING_AIH_WEIGHT="${arg#*=}"
      EXPLICIT_TURN_TIME_WEIGHT=0
      ;;
    --ladder_rungs=*|--ladder-rungs=*)
      LADDER_RUNGS="${arg#*=}"
      ;;
    --round_robin_rounds=*|--round-robin-rounds=*)
      ROUND_ROBIN_ROUNDS="${arg#*=}"
      ;;
    --registration-order=*)
      REGISTRATION_ORDER="${arg#*=}"
      ;;
    --registration-reverse|--reverse-registration)
      REGISTRATION_ORDER="reverse-alpha"
      ;;
    --registration-random|--random-registration|--shuffle-registration)
      REGISTRATION_ORDER="random"
      ;;
    --registration-forward|--forward-registration|--no-registration-random)
      REGISTRATION_ORDER="forward"
      ;;
    --default-starting-tokensperinput=*|--starting-tokens-per-input=*|--starting-output-tokens=*)
      STARTING_TOKENS_PER_INPUT="${arg#*=}"
      ;;
    --default-token-increase-ratio=*|--token-increase-ratio=*|--tokens-increase-ratio=*)
      TOKEN_INCREASE_RATIO="${arg#*=}"
      ;;
    --token-decrease-step=*|--tokens-decrease-step=*)
      TOKEN_DECREASE_STEP="${arg#*=}"
      ;;
    --token-increase-step=*|--tokens-increase-step=*)
      TOKEN_INCREASE_STEP="${arg#*=}"
      ;;
    --publish-latest-only|--publish-only)
      PUBLISH_LATEST_ONLY=1
      ;;
    --nruns=*|--n-runs=*|--runs=*)
      REPEAT_RUNS="${arg#*=}"
      ;;
    --minregs=*|--min-regs=*|--minimum-registrations=*)
      MIN_REGISTRATIONS="${arg#*=}"
      ;;
    --terminal-log|--output-log|--no-terminal-log|--no-output-log)
      ;;
    --registration-only|--register-only)
      REGISTRATION_ONLY=1
      ;;
    --local-smoke|--no-cloud)
      LOCAL_SMOKE=1
      CLOUD_SMOKE_PROVIDER=""
      ;;
    --local-prog-smoke|--local-progress-smoke)
      LOCAL_SMOKE=1
      CLOUD_SMOKE_PROVIDER=""
      SMOKE_STAGE="local-prog"
      ;;
    --local-retry-smoke)
      LOCAL_SMOKE=1
      CLOUD_SMOKE_PROVIDER=""
      SMOKE_STAGE="local-retry"
      ;;
    --local-expand-smoke)
      LOCAL_SMOKE=1
      CLOUD_SMOKE_PROVIDER=""
      SMOKE_STAGE="local-expand"
      ;;
    --full-agent-set|--allow-cloud)
      LOCAL_SMOKE=0
      CLOUD_SMOKE_PROVIDER=""
      SMOKE_STAGE="full-agent-set"
      ;;
    --cloud-smoke-openai)
      LOCAL_SMOKE=0
      CLOUD_SMOKE_PROVIDER="openai"
      SMOKE_STAGE="cloud-provider-key"
      ;;
    --cloud-smoke-google|--cloud-smoke-gemini)
      LOCAL_SMOKE=0
      CLOUD_SMOKE_PROVIDER="google"
      SMOKE_STAGE="cloud-provider-key"
      ;;
    --cloud-rep-gemini|--cloud-representative-gemini|--gemini-representative)
      LOCAL_SMOKE=0
      CLOUD_SMOKE_PROVIDER=""
      CLOUD_REPRESENTATIVE_PROVIDER="google"
      SMOKE_STAGE="cloud-rep"
      ;;
    --cloud-smoke-anthropic)
      LOCAL_SMOKE=0
      CLOUD_SMOKE_PROVIDER="anthropic"
      SMOKE_STAGE="cloud-provider-key"
      ;;
    --cloud-smoke-provider=*)
      LOCAL_SMOKE=0
      CLOUD_SMOKE_PROVIDER="${arg#*=}"
      SMOKE_STAGE="cloud-provider-key"
      ;;
    --smoke-stage=*)
      SMOKE_STAGE="${arg#*=}"
      ;;
    --reasoning-matrix)
      ENABLE_REASONING_MATRIX=1
      EXPLICIT_VERBOSITY_RANGE=1
      ;;
    --allowed-reasonings=*|--reasoning-range=*)
      ENABLE_REASONING_MATRIX=1
      REASONING_RANGE="${arg#*=}"
      ;;
    --allowed-verbosity=*|--text-verbosity-range=*)
      ENABLE_REASONING_MATRIX=1
      EXPLICIT_VERBOSITY_RANGE=1
      VERBOSITY_RANGE="${arg#*=}"
      ;;
    --local-maxplys=*)
      AIH_V5_LOCAL_MAXPLYS="${arg#*=}"
      ;;
    --local-cloud-maxply-ratio=*)
      AIH_V5_LOCAL_CLOUD_MAXPLY_RATIO="${arg#*=}"
      ;;
    --nice-initial=*)
      NICE_INITIAL="${arg#*=}"
      ;;
    --nice-step=*)
      NICE_STEP="${arg#*=}"
      ;;
    --nice-max=*)
      NICE_MAX="${arg#*=}"
      ;;
    --nice-step-seconds=*)
      NICE_STEP_SECONDS="${arg#*=}"
      ;;
    --no-dynamic-nice)
      DYNAMIC_NICE_ENABLED=0
      ;;
    --dynamic-nice)
      DYNAMIC_NICE_ENABLED=1
      ;;
    --no-renice-ollama)
      RENICE_OLLAMA_ENABLED=0
      ;;
    --no-html-open|--no-open-html|--no-html-report-open)
      OPEN_HTML_REPORT=0
      ;;
    --html-open|--open-html|--open-html-report)
      OPEN_HTML_REPORT=1
      ;;
    *)
      PASSTHRU_ARGS+=("$arg")
      ;;
  esac
done

case "$TOURNAMENT_FORMAT" in
  ladder|round-robin|round-robin-ladder|top4-ladder-rungs)
    ;;
  *)
    echo "aih_v5: invalid tournament format: $TOURNAMENT_FORMAT" >&2
    echo "aih_v5: expected ladder, round-robin, round-robin-ladder, or top4-ladder-rungs" >&2
    exit 2
    ;;
esac

case "$RANKING_MODE" in
  aih|turn-time|weighted)
    ;;
  *)
    echo "aih_v5: invalid ranking mode: $RANKING_MODE" >&2
    echo "aih_v5: expected aih, turn-time, or weighted" >&2
    exit 2
    ;;
esac

if [[ ! "$LADDER_RUNGS" =~ ^[0-9]+$ ]]; then
  echo "aih_v5: ladder_rungs must be a non-negative integer: $LADDER_RUNGS" >&2
  exit 2
fi
if [[ ! "$ROUND_ROBIN_ROUNDS" =~ ^[1-9][0-9]*$ ]]; then
  echo "aih_v5: round_robin_rounds must be a positive integer: $ROUND_ROBIN_ROUNDS" >&2
  exit 2
fi
if [[ ! "$REPEAT_RUNS" =~ ^[1-9][0-9]*$ ]]; then
  echo "aih_v5: nruns must be a positive integer: $REPEAT_RUNS" >&2
  exit 2
fi
if [[ ! "$MIN_REGISTRATIONS" =~ ^[0-9]+$ ]]; then
  echo "aih_v5: minregs must be a non-negative integer: $MIN_REGISTRATIONS" >&2
  exit 2
fi
if ((MIN_REGISTRATIONS > 0)); then
  if [[ ! -x "$ROOT_DIR/bin/aih_v5_repeat" ]]; then
    echo "aih_v5: repeat binary is not executable: $ROOT_DIR/bin/aih_v5_repeat" >&2
    exit 127
  fi
  exec "$ROOT_DIR/bin/aih_v5_repeat" "--minregs=$MIN_REGISTRATIONS" "$@"
fi
if ((REPEAT_RUNS > 1)); then
  if [[ ! -x "$ROOT_DIR/bin/aih_v5_repeat" ]]; then
    echo "aih_v5: repeat binary is not executable: $ROOT_DIR/bin/aih_v5_repeat" >&2
    exit 127
  fi
  exec "$ROOT_DIR/bin/aih_v5_repeat" "--nruns=$REPEAT_RUNS" "$@"
fi

if [[ "$RANKING_MODE" == "weighted" && "$EXPLICIT_TURN_TIME_WEIGHT" == "0" ]]; then
  RANKING_TURN_TIME_WEIGHT="$(
    awk -v w="$RANKING_AIH_WEIGHT" 'BEGIN { printf "%.6f\n", 1.0 - w }'
  )"
fi

if [[ ! "$OLLAMA_NUM_THREAD" =~ ^[1-9][0-9]*$ ]]; then
  echo "aih_v5: AIH_V5_OLLAMA_NUM_THREAD must be a positive integer: $OLLAMA_NUM_THREAD" >&2
  exit 2
fi
if [[ ! "$STARTING_TOKENS_PER_INPUT" =~ ^[1-9][0-9]*$ ]]; then
  echo "aih_v5: starting tokens per input must be a positive integer: $STARTING_TOKENS_PER_INPUT" >&2
  exit 2
fi
if ! awk -v ratio="$TOKEN_INCREASE_RATIO" 'BEGIN { exit !(ratio + 0 > 1.0) }'; then
  echo "aih_v5: token increase ratio must be greater than 1.0: $TOKEN_INCREASE_RATIO" >&2
  exit 2
fi
if [[ ! "$TOKEN_DECREASE_STEP" =~ ^[1-9][0-9]*$ ]]; then
  echo "aih_v5: token decrease step must be a positive integer: $TOKEN_DECREASE_STEP" >&2
  exit 2
fi
if [[ ! "$TOKEN_INCREASE_STEP" =~ ^[1-9][0-9]*$ ]]; then
  echo "aih_v5: token increase step must be a positive integer: $TOKEN_INCREASE_STEP" >&2
  exit 2
fi
for _aih_v5_nice_value in "$NICE_INITIAL" "$NICE_STEP" "$NICE_MAX" "$NICE_STEP_SECONDS"; do
  if [[ ! "$_aih_v5_nice_value" =~ ^[0-9]+$ ]]; then
    echo "aih_v5: nice controls must be non-negative integers: initial=$NICE_INITIAL step=$NICE_STEP max=$NICE_MAX step_seconds=$NICE_STEP_SECONDS" >&2
    exit 2
  fi
done
if ((NICE_INITIAL > 19 || NICE_MAX > 19)); then
  echo "aih_v5: nice initial/max must be <= 19: initial=$NICE_INITIAL max=$NICE_MAX" >&2
  exit 2
fi
if ((NICE_INITIAL > NICE_MAX)); then
  echo "aih_v5: nice initial must be <= nice max: initial=$NICE_INITIAL max=$NICE_MAX" >&2
  exit 2
fi

latest_summary_path() {
  if ((PUBLISH_LATEST_ONLY == 1)); then
    find "$ROOT_DIR/runs/aih_v5_pairwise_prototype_20260729" "$ROOT_DIR/data" -maxdepth 1 -type f -name '*_summary.md' \
      -printf '%T@ %p\n' 2>/dev/null |
      sort -nr |
      awk 'NR == 1 { $1 = ""; sub(/^ /, ""); print }'
  else
    find "$ROOT_DIR/runs/aih_v5_pairwise_prototype_20260729" -maxdepth 1 -type f -name '*_summary.md' \
      -printf '%T@ %p\n' 2>/dev/null |
      awk -v start="$AIH_V5_START_EPOCH" '$1 >= start { print }' |
      sort -nr |
      awk 'NR == 1 { $1 = ""; sub(/^ /, ""); print }'
  fi
}

current_run_jsonl_paths() {
  find "$ROOT_DIR/runs/aih_v5_pairwise_prototype_20260729" -maxdepth 1 -type f -name '*.jsonl' \
    -printf '%T@ %p\n' 2>/dev/null |
    awk -v start="$AIH_V5_START_EPOCH" '$1 >= start { print }' |
    sort -n |
    awk '{ $1 = ""; sub(/^ /, ""); print }'
}

aggregate_current_run_jsonl() {
  local published_dir="$ROOT_DIR/data"
  local aggregate="$published_dir/AIH_V5_CURRENT_RUN_AGGREGATE.jsonl"
  local count=0
  local path
  mkdir -p "$published_dir"
  : > "$aggregate"
  while IFS= read -r path; do
    [[ -r "$path" ]] || continue
    cat "$path" >> "$aggregate"
    count=$((count + 1))
  done < <(current_run_jsonl_paths)
  if ((count == 0)); then
    rm -f "$aggregate"
    return 1
  fi
  printf '%s\n' "$aggregate"
}

emit_aih_ranking_rows() {
  local jsonl="$1"
  jq -r '
    .events[]? |
    .model as $model |
    if (.transport_failure == true or .error == "move_request_transport_failure" or .response.status == "request_failed") then
      [$model, "oh"]
    elif (.legal_by_rules == true or .legal == true) then
      [$model, "legal"]
    elif ((.error // "") | test("hallucination|illegal_move|unparseable_move|invalid_move|no_candidate")) then
      [$model, "agntoh"]
    elif (.irrelevant_agent_return == true or .error == "irrelevant_agent_return") then
      [$model, "agntoh"]
    else
      [$model, "agntoh"]
    end |
    @tsv
  ' "$jsonl" |
  awk -F '\t' '
    function pct(n, d) { return d > 0 ? (100.0 * n / d) : -1 }
    function agent_label(model) {
      if (model ~ /^openai:/) {
        sub(/^openai:/, "", model)
        return "c openai " model
      }
      if (model ~ /^anthropic:/) {
        sub(/^anthropic:/, "", model)
        return "c anthropic " model
      }
      if (model ~ /^gemini:/) {
        sub(/^gemini:/, "", model)
        return "c google " model
      }
      if (model ~ /^google:/) {
        sub(/^google:/, "", model)
        return "c google " model
      }
      if (model ~ /^codex:/) {
        sub(/^codex:/, "", model)
        return "l openai " model
      }
      return "l ollama " model
    }
    function fmt(x) { return sprintf("%06.3f", x + 0) }
    function cls(p) { return p >= 60 ? "aih-high" : p >= 20 ? "aih-mid" : "aih-low" }
    function esc(s) { gsub(/&/, "\\&amp;", s); gsub(/</, "\\&lt;", s); gsub(/>/, "\\&gt;", s); return s }
    {
      agent=agent_label($1)
      kind=$2
      seen[agent]=1
      if (kind == "aih") aih[agent]++
      else if (kind == "legal") legal[agent]++
      else if (kind == "oh") oh[agent]++
      else agntoh[agent]++
    }
    END {
      for (agent in seen) {
        scored = aih[agent] + legal[agent] + agntoh[agent]
        total = scored + oh[agent]
        if (total > 0) {
          aih_pct = pct(aih[agent] + agntoh[agent], total)
          legal_pct = pct(legal[agent], total)
          agntoh_pct = pct(agntoh[agent], total)
          commfail_pct = pct(oh[agent], total)
          invalid = 0
          printf "%d\t%.8f\t%.8f\t%.8f\t%s\n",
            invalid,
            aih_pct,
            legal_pct,
            agntoh_pct,
            agent
        } else {
          printf "2\t999999.00000000\t0.00000000\t0.00000000\t%s\n", agent
        }
      }
    }
  ' |
  sort -t $'\t' -k1,1n -k2,2n -k4,4n -k3,3nr -k5,5 |
  awk -F '\t' '
    function fmt(x) { return sprintf("%06.3f", x + 0) }
    function cls(p) { return p >= 60 ? "aih-high" : p >= 20 ? "aih-mid" : "aih-low" }
    function esc(s) { gsub(/&/, "\\&amp;", s); gsub(/</, "\\&lt;", s); gsub(/>/, "\\&gt;", s); return s }
    {
      rank = NR
      group = $1
      aih = $2 + 0
      legal = $3 + 0
      agntoh = $4 + 0
      agent = $5
      local_flag = ""
      title = agent
      if (title ~ /^[lc] /) {
        lc = substr(title, 1, 1)
        local_flag = lc == "l" ? "1" : "0"
        title = substr(title, 3)
      }
      if (group == 2) {
        aih_s = "n/a"
        legal_s = "n/a"
        agntoh_s = "n/a"
        class = "aih-low"
      } else {
        aih_s = fmt(aih)
        legal_s = fmt(legal)
        agntoh_s = fmt(agntoh)
        class = cls(aih)
      }
      rank_s = group == 0 ? rank : "n/a"
      printf "        <tr><td class=\"num\">%s</td><td class=\"num %s\">%s</td><td class=\"num\">%s</td><td class=\"num\">%s</td><td>%s</td><td>0/1 (000.00%%)</td></tr>\n",
        rank_s, class, aih_s, legal_s, esc(local_flag), esc(title)
    }
  '
}

emit_registration_failed_main_rows() {
  local csv="$1"
  [[ -r "$csv" ]] || return 0
  awk -F, '
    function esc(s) { gsub(/&/, "\\&amp;", s); gsub(/</, "\\&lt;", s); gsub(/>/, "\\&gt;", s); return s }
    NR > 1 && $2 != "pass" {
      failed[$1]++
      reason[$1] = $3
      elapsed[$1] += $4 + 0
      timeout[$1] = $5
      if ($3 ~ /timed out/) timeouts[$1]++
    }
    END {
      for (agent in failed) {
        if (timeouts[agent] > 0) {
          pct = failed[agent] > 0 ? (100.0 * timeouts[agent] / failed[agent]) : 0.0
          note = timeouts[agent] "/" failed[agent] " (" sprintf("%06.2f", pct) "%)"
        } else {
          note = "0/" failed[agent] " (000.00%)"
        }
        print agent "\t" note
      }
    }
  ' "$csv" |
    sort -f -k1,1 |
    awk -F'\t' '
      function esc(s) { gsub(/&/, "\\&amp;", s); gsub(/</, "\\&lt;", s); gsub(/>/, "\\&gt;", s); return s }
      {
        printf "        <tr><td class=\"num\">n/a</td><td class=\"num\">n/a</td><td class=\"num\">n/a</td><td class=\"num\">1</td><td>%s</td><td>%s</td></tr>\n", esc($1), esc($2)
      }'
}

emit_v5_ranking_csv() {
  local jsonl="$1"
  local out_csv="$2"
  jq -r '
    .events[]? |
    .model as $model |
    (.move_to_referee_elapsed_s // .response.elapsed_s // 0) as $elapsed |
    (.response.prompt_eval_count // 0) as $prompt_tokens |
    (.response.eval_count // 0) as $output_tokens |
    (.response.total_token_count // ($prompt_tokens + $output_tokens)) as $total_tokens |
    (.response.total_tokens_per_s // 0) as $total_tokens_per_s |
    (.response.output_tokens_per_s // 0) as $output_tokens_per_s |
    (.response.output_token_utilization_pct // 0) as $output_token_utilization_pct |
    (if (.response.suspected_output_token_shortage // false) then 1 else 0 end) as $token_shortage |
    if (.transport_failure == true or .error == "move_request_transport_failure" or .response.status == "request_failed") then
      [$model, "oh", $elapsed, $prompt_tokens, $output_tokens, $total_tokens, $total_tokens_per_s, $output_tokens_per_s, $output_token_utilization_pct, $token_shortage]
    elif (.legal_by_rules == true or .legal == true) then
      [$model, "legal", $elapsed, $prompt_tokens, $output_tokens, $total_tokens, $total_tokens_per_s, $output_tokens_per_s, $output_token_utilization_pct, $token_shortage]
    elif ((.error // "") | test("hallucination|illegal_move|unparseable_move|invalid_move|no_candidate")) then
      [$model, "agntoh", $elapsed, $prompt_tokens, $output_tokens, $total_tokens, $total_tokens_per_s, $output_tokens_per_s, $output_token_utilization_pct, $token_shortage]
    elif (.irrelevant_agent_return == true or .error == "irrelevant_agent_return") then
      [$model, "agntoh", $elapsed, $prompt_tokens, $output_tokens, $total_tokens, $total_tokens_per_s, $output_tokens_per_s, $output_token_utilization_pct, $token_shortage]
    else
      [$model, "agntoh", $elapsed, $prompt_tokens, $output_tokens, $total_tokens, $total_tokens_per_s, $output_tokens_per_s, $output_token_utilization_pct, $token_shortage]
    end |
    @tsv
  ' "$jsonl" |
  awk -F '\t' \
    -v tournament_format="$TOURNAMENT_FORMAT" \
    -v ranking_mode="$RANKING_MODE" \
    -v aih_weight="$RANKING_AIH_WEIGHT" \
    -v time_weight="$RANKING_TURN_TIME_WEIGHT" '
    function pct(n, d) { return d > 0 ? (100.0 * n / d) : 999999.0 }
    function esc_csv(s) { gsub(/"/, "\"\"", s); return "\"" s "\"" }
    {
      agent=$1
      kind=$2
      elapsed=$3 + 0
      prompt_tokens=$4 + 0
      output_tokens=$5 + 0
      total_tokens=$6 + 0
      total_tokens_per_s=$7 + 0
      output_tokens_per_s=$8 + 0
      output_token_utilization_pct=$9 + 0
      token_shortage=$10 + 0
      seen[agent]=1
      total[agent]++
      elapsed_total[agent]+=elapsed
      prompt_tokens_total[agent]+=prompt_tokens
      output_tokens_total[agent]+=output_tokens
      total_tokens_total[agent]+=total_tokens
      total_tokens_per_s_total[agent]+=total_tokens_per_s
      output_tokens_per_s_total[agent]+=output_tokens_per_s
      output_token_utilization_pct_total[agent]+=output_token_utilization_pct
      token_shortage_total[agent]+=token_shortage
      if (kind == "aih") aih[agent]++
      else if (kind == "legal") legal[agent]++
      else if (kind == "oh") hrnoh[agent]++
      else agntoh[agent]++
    }
    END {
      max_time=0
      for (agent in seen) {
        avg_time[agent] = total[agent] > 0 ? elapsed_total[agent] / total[agent] : 999999.0
        if (avg_time[agent] > max_time && avg_time[agent] < 999999.0) max_time = avg_time[agent]
      }
      print "agent,tournament_format,ranking_mode,aih_weight,turn_time_weight,aih_pct,net_turn_time_per_ply_s,weighted_score,legal_pct,agent_output_hallucination_pct,plies"
      for (agent in seen) {
        aih_pct = pct(aih[agent] + agntoh[agent], total[agent])
        legal_pct = pct(legal[agent], total[agent])
        agntoh_pct = pct(agntoh[agent], total[agent])
        prompt_tokens_avg = total[agent] > 0 ? prompt_tokens_total[agent] / total[agent] : 0.0
        output_tokens_avg = total[agent] > 0 ? output_tokens_total[agent] / total[agent] : 0.0
        total_tokens_avg = total[agent] > 0 ? total_tokens_total[agent] / total[agent] : 0.0
        total_tokens_per_s_avg = total[agent] > 0 ? total_tokens_per_s_total[agent] / total[agent] : 0.0
        output_tokens_per_s_avg = total[agent] > 0 ? output_tokens_per_s_total[agent] / total[agent] : 0.0
        output_token_utilization_pct_avg = total[agent] > 0 ? output_token_utilization_pct_total[agent] / total[agent] : 0.0
        time_norm = max_time > 0 ? (100.0 * avg_time[agent] / max_time) : 0.0
        if (ranking_mode == "aih") score = aih_pct
        else if (ranking_mode == "turn-time") score = avg_time[agent]
        else score = (aih_weight + 0) * aih_pct + (time_weight + 0) * time_norm
        printf "%s,%s,%s,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%d\n",
          esc_csv(agent), esc_csv(tournament_format), esc_csv(ranking_mode),
          aih_weight + 0, time_weight + 0, aih_pct, avg_time[agent], score,
          legal_pct, agntoh_pct, total[agent],
          prompt_tokens_avg, output_tokens_avg, total_tokens_avg,
          total_tokens_per_s_avg, output_tokens_per_s_avg,
          output_token_utilization_pct_avg, token_shortage_total[agent]
      }
    }
  ' |
  awk 'NR == 1 { print $0 ",prompt_tokens_per_exchange,output_tokens_per_exchange,total_tokens_per_exchange,total_tokens_per_s,output_tokens_per_s,output_token_utilization_pct,token_shortage_exchanges"; next } { print }' |
  {
    IFS= read -r header
    printf '%s\n' "$header"
    sort -t, -k8,8n
  } > "$out_csv"
}

emit_filtered_registration_rows() {
  local csv="$1"
  [[ -r "$csv" ]] || return 0
  awk -F, '
    function esc(s) { gsub(/&/, "\\&amp;", s); gsub(/</, "\\&lt;", s); gsub(/>/, "\\&gt;", s); return s }
    NR > 1 && $2 != "pass" {
      printf "        <tr><td>%s</td><td>%s</td><td>%s</td><td class=\"num\">%s</td><td class=\"num\">%s</td></tr>\n", esc($1), esc($2), esc($3), esc($4), esc($5)
    }
  ' "$csv"
}

emit_registration_timeout_rows() {
  local csv="$1"
  [[ -r "$csv" ]] || return 0
  awk -F, '
    function esc(s) { gsub(/&/, "\\&amp;", s); gsub(/</, "\\&lt;", s); gsub(/>/, "\\&gt;", s); return s }
    NR > 1 && $3 ~ /timed out/ {
      print $1 "\t" $2 "\t" $3 "\t" $4 "\t" $5
    }
  ' "$csv" |
    sort -f -k1,1 |
    awk -F'\t' '
      function esc(s) { gsub(/&/, "\\&amp;", s); gsub(/</, "\\&lt;", s); gsub(/>/, "\\&gt;", s); return s }
      {
        printf "        <tr><td>%s</td><td>%s</td><td>%s</td><td class=\"num\">%s</td><td class=\"num\">%s</td></tr>\n", esc($1), esc($2), esc($3), esc($4), esc($5)
      }'
}

emit_efficiency_measurements_html() {
  local registration_csv="$1"
  local jsonl="$2"
  local reg_elapsed tournament_elapsed boards plies total_elapsed reg_pct tournament_pct sec_per_ply
  [[ -r "$registration_csv" && -r "$jsonl" ]] || return 0
  command -v jq >/dev/null 2>&1 || return 0
  reg_elapsed="$(awk -F, 'NR > 1 { s += $4 + 0 } END { printf "%.3f", s }' "$registration_csv")"
  tournament_elapsed="$(jq -s 'map(.total_elapsed_s // 0) | add // 0' "$jsonl")"
  boards="$(jq -s 'length' "$jsonl")"
  plies="$(jq -s 'map(.plies_played // 0) | add // 0' "$jsonl")"
  total_elapsed="$(
    awk -v r="$reg_elapsed" -v t="$tournament_elapsed" 'BEGIN { printf "%.3f", r + t }'
  )"
  reg_pct="$(
    awk -v r="$reg_elapsed" -v total="$total_elapsed" 'BEGIN { if (total > 0) printf "%.1f", 100 * r / total; else printf "0.0" }'
  )"
  tournament_pct="$(
    awk -v t="$tournament_elapsed" -v total="$total_elapsed" 'BEGIN { if (total > 0) printf "%.1f", 100 * t / total; else printf "0.0" }'
  )"
  sec_per_ply="$(
    awk -v t="$tournament_elapsed" -v p="$plies" 'BEGIN { if (p > 0) printf "%.3f", t / p; else printf "0.000" }'
  )"
  echo '    <h2>Efficiency Measurements</h2>'
  echo '    <table>'
  echo '      <thead><tr><th>Metric</th><th class="num">Value</th></tr></thead>'
  echo '      <tbody>'
  printf '        <tr><td>Registration elapsed seconds</td><td class="num">%.3f</td></tr>\n' "$reg_elapsed"
  printf '        <tr><td>Tournament elapsed seconds</td><td class="num">%.3f</td></tr>\n' "$tournament_elapsed"
  printf '        <tr><td>Total measured seconds</td><td class="num">%.3f</td></tr>\n' "$total_elapsed"
  printf '        <tr><td>Registration share</td><td class="num">%s%%</td></tr>\n' "$reg_pct"
  printf '        <tr><td>Tournament share</td><td class="num">%s%%</td></tr>\n' "$tournament_pct"
  printf '        <tr><td>Tournament boards</td><td class="num">%s</td></tr>\n' "$boards"
  printf '        <tr><td>Tournament plies played</td><td class="num">%s</td></tr>\n' "$plies"
  printf '        <tr><td>Tournament seconds per played ply</td><td class="num">%s</td></tr>\n' "$sec_per_ply"
  echo '      </tbody>'
  echo '    </table>'
}

emit_registration_timing_summary_text() {
  local csv="$1"
  [[ -r "$csv" ]] || return 0
  awk -F, '
    NR > 1 {
      elapsed = $4 + 0
      timeout = $5 + 0
      total_count++
      total_elapsed += elapsed
      if ($2 == "pass") {
        pass_count++
        pass_elapsed += elapsed
      } else if ($3 ~ /timed out/) {
        timeout_count++
        timeout_elapsed += elapsed
        timeout_budget += timeout
      } else {
        other_fail_count++
        other_fail_elapsed += elapsed
      }
    }
    END {
      pass_avg = pass_count ? pass_elapsed / pass_count : 0
      timeout_avg = timeout_count ? timeout_elapsed / timeout_count : 0
      other_fail_avg = other_fail_count ? other_fail_elapsed / other_fail_count : 0
      estimated = pass_elapsed + timeout_elapsed + other_fail_elapsed
      printf "aih_v5: registration timing summary: total_candidates=%d total_elapsed_s=%.0f pass_count=%d pass_elapsed_s=%.0f pass_avg_s=%.2f timeout_count=%d timeout_elapsed_s=%.0f timeout_avg_s=%.2f timeout_budget_s=%.0f other_fail_count=%d other_fail_elapsed_s=%.0f other_fail_avg_s=%.2f formula_s=%.0f\n",
        total_count, total_elapsed,
        pass_count, pass_elapsed, pass_avg,
        timeout_count, timeout_elapsed, timeout_avg, timeout_budget,
        other_fail_count, other_fail_elapsed, other_fail_avg,
        estimated
    }
  ' "$csv"
}

emit_registration_timing_summary_html() {
  local csv="$1"
  [[ -r "$csv" ]] || return 0
  awk -F, '
    NR > 1 {
      elapsed = $4 + 0
      timeout = $5 + 0
      total_count++
      total_elapsed += elapsed
      if ($2 == "pass") {
        pass_count++
        pass_elapsed += elapsed
      } else if ($3 ~ /timed out/) {
        timeout_count++
        timeout_elapsed += elapsed
        timeout_budget += timeout
      } else {
        other_fail_count++
        other_fail_elapsed += elapsed
      }
    }
    END {
      pass_avg = pass_count ? pass_elapsed / pass_count : 0
      timeout_avg = timeout_count ? timeout_elapsed / timeout_count : 0
      other_fail_avg = other_fail_count ? other_fail_elapsed / other_fail_count : 0
      estimated = pass_elapsed + timeout_elapsed + other_fail_elapsed
      print "    <h2>Registration Timing</h2>"
      print "    <table>"
      print "      <thead><tr><th>Bucket</th><th class=\"num\">Count</th><th class=\"num\">Elapsed s</th><th class=\"num\">Avg s</th><th class=\"num\">Timeout Budget s</th></tr></thead>"
      print "      <tbody>"
      printf "        <tr><td>Pass</td><td class=\"num\">%d</td><td class=\"num\">%.0f</td><td class=\"num\">%.2f</td><td class=\"num\">n/a</td></tr>\n", pass_count, pass_elapsed, pass_avg
      printf "        <tr><td>Timeout</td><td class=\"num\">%d</td><td class=\"num\">%.0f</td><td class=\"num\">%.2f</td><td class=\"num\">%.0f</td></tr>\n", timeout_count, timeout_elapsed, timeout_avg, timeout_budget
      printf "        <tr><td>Other fail</td><td class=\"num\">%d</td><td class=\"num\">%.0f</td><td class=\"num\">%.2f</td><td class=\"num\">n/a</td></tr>\n", other_fail_count, other_fail_elapsed, other_fail_avg
      printf "        <tr><td>Total</td><td class=\"num\">%d</td><td class=\"num\">%.0f</td><td class=\"num\">n/a</td><td class=\"num\">%.0f</td></tr>\n", total_count, estimated, timeout_budget
      print "      </tbody>"
      print "    </table>"
      printf "    <p class=\"note\">Registration formula: pass_elapsed_s + timeout_elapsed_s + other_fail_elapsed_s = %.0f seconds. Timeout budget uses timeout_count * timeout_seconds from each candidate row.</p>\n", estimated
    }
  ' "$csv"
}

emit_registration_runtime_prediction_text() {
  local csv="$1"
  local maxply="${DEFAULT_MAXPLYS:-${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}}"
  local move_timeout="${DEFAULT_MOVE_TIMEOUT:-${AIH_V5_MOVE_TIMEOUT_SECONDS:-20}}"
  [[ -r "$csv" ]] || return 0
  awk -F, -v maxply="$maxply" -v move_timeout="$move_timeout" '
    NR > 1 {
      elapsed = $4 + 0
      reg_elapsed += elapsed
      if ($2 == "pass") pass_count++
    }
    END {
      tournament_ops = pass_count * maxply * 2
      tournament_worst = tournament_ops * move_timeout
      predicted_worst = reg_elapsed + tournament_worst
      printf "aih_v5: runtime prediction: registration_elapsed_s=%.0f registered_agents=%d maxply=%d tournament_ops=npass*maxply*2=%d move_timeout_s=%d tournament_worst_case_s=%d predicted_worst_case_total_s=%d\n",
        reg_elapsed, pass_count, maxply, tournament_ops, move_timeout, tournament_worst, predicted_worst
    }
  ' "$csv"
}

emit_registration_runtime_prediction_html() {
  local csv="$1"
  local maxply="${DEFAULT_MAXPLYS:-${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}}"
  [[ -r "$csv" ]] || return 0
  awk -F, -v maxply="$maxply" '
    NR > 1 {
      elapsed = $4 + 0
      reg_elapsed += elapsed
      if ($2 == "pass") pass_count++
    }
    END {
      print "    <h2>Runtime Prediction</h2>"
      print "    <table>"
      print "      <thead><tr><th>Component</th><th class=\"num\">Value</th></tr></thead>"
      print "      <tbody>"
      printf "        <tr><td>Registration elapsed seconds</td><td class=\"num\">%.0f</td></tr>\n", reg_elapsed
      printf "        <tr><td>Registered agents</td><td class=\"num\">%d</td></tr>\n", pass_count
      printf "        <tr><td>Maxply</td><td class=\"num\">%d</td></tr>\n", maxply
      print "      </tbody>"
      print "    </table>"
    }
  ' "$csv"
}

publish_v5_ranking_artifacts() {
  local jsonl="$1"
  local base out_csv latest_csv
  if [[ ! -r "$jsonl" ]]; then
    return 0
  fi
  base="${jsonl%.jsonl}"
  out_csv="${base}_v5_rankings_${TOURNAMENT_FORMAT}_${RANKING_MODE}.csv"
  latest_csv="$ROOT_DIR/AIH_V5_LATEST_RANKINGS.csv"
  emit_v5_ranking_csv "$jsonl" "$out_csv"
  cp "$out_csv" "$latest_csv"
  echo "aih_v5: ranking csv: $out_csv" >&2
  echo "aih_v5: latest ranking csv: $latest_csv" >&2
}

publish_latest_summary() {
  local latest_summary latest_jsonl
  local published_dir published_summary published_jsonl published_html latest_html
  local aggregate_jsonl ranking_jsonl
  latest_summary="$(latest_summary_path)"
  echo "aih_v5: gen cur rpt..." >&2
  if [[ -z "$latest_summary" || ! -r "$latest_summary" ]]; then
    echo "aih_v5: no sum file." >&2
    return 2
  fi

  latest_jsonl="${latest_summary%_summary.md}.jsonl"
  published_dir="$ROOT_DIR/data"
  mkdir -p "$published_dir"
  published_summary="$published_dir/$(basename "$latest_summary")"
  published_jsonl="$published_dir/$(basename "$latest_jsonl")"
  published_html="$published_dir/$(basename "${latest_summary%_summary.md}.html")"
  latest_html="$LATEST_HTML_REPORT"
  cp "$latest_summary" "$published_summary"
  if [[ -r "$latest_jsonl" ]]; then
    cp "$latest_jsonl" "$published_jsonl"
  fi
  aggregate_jsonl="$(aggregate_current_run_jsonl || true)"
  ranking_jsonl="$published_jsonl"
  if [[ -n "$aggregate_jsonl" && -r "$aggregate_jsonl" ]]; then
    ranking_jsonl="$aggregate_jsonl"
  fi

  {
    echo '<!doctype html>'
    echo '<html lang="en">'
    echo '<head>'
    echo '  <meta charset="utf-8">'
    echo '  <title>AIH v5 latest run</title>'
    echo '  <style>body{font-family:Arial,sans-serif;margin:2rem;line-height:1.45;color:#1f2933}table{border-collapse:collapse;width:100%;margin-top:1rem}th,td{border:1px solid #cad2dc;padding:.55rem;text-align:left;vertical-align:top}th{background:#eef2f6}.note{color:#52606d}code{background:#eef2f6;padding:.1rem .25rem;border-radius:3px}td.num{text-align:right;font-variant-numeric:tabular-nums}.aih-high{color:#b42318;font-weight:700}.aih-mid{color:#8a5a00;font-weight:700}.aih-low{color:#146c43;font-weight:700}</style>'
    echo '</head>'
    echo '<body>'
    echo '  <main>'
    echo '    <h1>AIH v5 latest run</h1>'
    echo "    <p class=\"note\">Summary: <code>data/$(basename "$published_summary")</code></p>"
    if [[ -r "$published_jsonl" ]]; then
      echo "    <p class=\"note\">JSONL: <code>data/$(basename "$published_jsonl")</code></p>"
    fi
    if [[ -r "$aggregate_jsonl" ]]; then
      echo "    <p class=\"note\">Aggregate JSONL: <code>data/$(basename "$aggregate_jsonl")</code></p>"
    fi
    awk '/^GameMode:/ { print "    <p class=\"note\"><strong>GameMode:</strong> <code>" $2 "</code></p>" }' "$latest_summary"
    echo '    <p class="note">Sorted by lowest AIH% across the current run aggregate; invalid or suspect rows are shown with rank n/a after scored rows.</p>'
    echo '    <table>'
    echo '      <thead><tr><th>Rank</th><th class="num">AIH%</th><th class="num">Legal%</th><th class="num">Local?</th><th>Agent Title</th><th>Reg. T.O.</th></tr></thead>'
    echo '      <tbody>'
    if [[ -r "$ranking_jsonl" ]]; then
      emit_aih_ranking_rows "$ranking_jsonl"
    fi
    if [[ -r "$REGISTRATION_STATUS_CSV" ]]; then
      emit_registration_failed_main_rows "$REGISTRATION_STATUS_CSV"
    fi
    echo '      </tbody>'
    echo '    </table>'
    if [[ -r "$REGISTRATION_STATUS_CSV" && -r "$ranking_jsonl" ]]; then
      emit_efficiency_measurements_html "$REGISTRATION_STATUS_CSV" "$ranking_jsonl"
      emit_registration_runtime_prediction_html "$REGISTRATION_STATUS_CSV"
    fi
    echo '  </main>'
    echo '</body>'
    echo '</html>'
  } > "$published_html"

  cp "$published_html" "$latest_html"

  echo "aih_v5: dat -> $published_dir." >&2
  echo "aih_v5: cur html: $published_html" >&2
  echo "aih_v5: lat html: $latest_html" >&2
  echo "aih_v5: rpt ok." >&2
}

open_latest_html_report() {
  local html="$LATEST_HTML_REPORT"
  if [[ "$OPEN_HTML_REPORT" != "1" && "$OPEN_HTML_REPORT" != "yes" && "$OPEN_HTML_REPORT" != "true" ]]; then
    return 0
  fi
  if [[ ! -r "$html" ]]; then
    echo "aih_v5: html report not found for browser open: $html" >&2
    return 0
  fi
  echo "aih_v5: html file: $html" >&2
  if command -v xdg-open >/dev/null 2>&1; then
    nohup xdg-open "$html" >/dev/null 2>&1 &
  elif command -v sensible-browser >/dev/null 2>&1; then
    nohup sensible-browser "$html" >/dev/null 2>&1 &
  elif command -v firefox >/dev/null 2>&1; then
    nohup firefox "$html" >/dev/null 2>&1 &
  else
    echo "aih_v5: no browser opener found; html report: $html" >&2
    return 0
  fi
  echo "aih_v5: opened html report: $html" >&2
}

publish_status_html_report() {
  local run_status="$1"
  local note="$2"
  local html="$LATEST_HTML_REPORT"
  {
    echo '<!doctype html>'
    echo '<html lang="en">'
    echo '<head>'
    echo '  <meta charset="utf-8">'
    echo '  <title>AIH v5 run status</title>'
    echo '  <style>body{font-family:Arial,sans-serif;margin:2rem;line-height:1.45;color:#1f2933}table{border-collapse:collapse;width:100%;margin-top:1rem}th,td{border:1px solid #cad2dc;padding:.55rem;text-align:left;vertical-align:top}th{background:#eef2f6}code{background:#eef2f6;padding:.1rem .25rem;border-radius:3px}.warn{color:#8a5a00;font-weight:700}</style>'
    echo '</head>'
    echo '<body>'
    echo '  <main>'
    echo '    <h1>AIH v5 run status</h1>'
    echo "    <p class=\"warn\">$note</p>"
    echo "    <p>Run status: <code>$run_status</code></p>"
    echo "    <p>Tournament format: <code>$TOURNAMENT_FORMAT</code></p>"
    echo "    <p>Ranking mode: <code>$RANKING_MODE</code>; AIH weight: <code>$RANKING_AIH_WEIGHT</code>; turn-time weight: <code>$RANKING_TURN_TIME_WEIGHT</code></p>"
    echo "    <p>Defaults: ladder rungs <code>$LADDER_RUNGS</code>, round-robin rounds <code>$ROUND_ROBIN_ROUNDS</code>, registration mode <code>$REGISTRATION_MODE</code>, registration timeout <code>$REGISTRATION_TIMEOUT_SECONDS</code> seconds.</p>"
    echo "    <p>Registration CSV: <code>$REGISTRATION_STATUS_CSV</code></p>"
    if [[ -r "$REGISTRATION_DIAGNOSTIC_LOG" ]]; then
      echo "    <p>Registration diagnostics: <code>$REGISTRATION_DIAGNOSTIC_LOG</code></p>"
      echo '    <h2>Diagnostics</h2>'
      echo '    <pre>'
      sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' "$REGISTRATION_DIAGNOSTIC_LOG"
      echo '    </pre>'
    fi
    if [[ -r "$REGISTRATION_STATUS_CSV" ]]; then
      emit_registration_timing_summary_html "$REGISTRATION_STATUS_CSV"
      echo '    <h2>Registration</h2>'
      echo '    <table><thead><tr><th>Candidate</th><th>Status</th><th>Reason</th><th>Elapsed s</th><th>Timeout s</th></tr></thead><tbody>'
      awk -F, 'NR > 1 {
        gsub(/&/, "\\&amp;", $1); gsub(/</, "\\&lt;", $1); gsub(/>/, "\\&gt;", $1)
        gsub(/&/, "\\&amp;", $2); gsub(/</, "\\&lt;", $2); gsub(/>/, "\\&gt;", $2)
        gsub(/&/, "\\&amp;", $3); gsub(/</, "\\&lt;", $3); gsub(/>/, "\\&gt;", $3)
        gsub(/&/, "\\&amp;", $4); gsub(/</, "\\&lt;", $4); gsub(/>/, "\\&gt;", $4)
        gsub(/&/, "\\&amp;", $5); gsub(/</, "\\&lt;", $5); gsub(/>/, "\\&gt;", $5)
        printf "      <tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n", $1, $2, $3, $4, $5
      }' "$REGISTRATION_STATUS_CSV"
      echo '    </tbody></table>'
    fi
    echo '  </main>'
    echo '</body>'
    echo '</html>'
  } > "$html"
  echo "aih_v5: status html: $html" >&2
}

publish_run_html_report() {
  local run_status="$1"
  local jsonl aggregate_jsonl
  if passthru_has_arg "--dry-run"; then
    return 0
  fi
  if ((run_status != 0)); then
    publish_status_html_report "$run_status" "The AIH v5 run exited nonzero, so this failure summary was generated instead of a ranking report."
    open_latest_html_report
    return 0
  fi
  if [[ -x "$ROOT_DIR/bin/aih_v5_html_report" ]]; then
    if "$ROOT_DIR/bin/aih_v5_html_report"; then
      aggregate_jsonl="$(aggregate_current_run_jsonl || true)"
      jsonl="$aggregate_jsonl"
      if [[ -z "$jsonl" || ! -r "$jsonl" ]]; then
        jsonl="$(latest_jsonl_path)"
      fi
      if [[ -n "$jsonl" && -r "$jsonl" ]]; then
        publish_v5_ranking_artifacts "$jsonl"
      fi
      return 0
    fi
    echo "aih_v5: compiled html report binary failed; falling back to shell publisher." >&2
  fi
  if publish_latest_summary; then
    aggregate_jsonl="$(aggregate_current_run_jsonl || true)"
    jsonl="$aggregate_jsonl"
    if [[ -z "$jsonl" || ! -r "$jsonl" ]]; then
      jsonl="$(latest_jsonl_path)"
    fi
    if [[ -n "$jsonl" && -r "$jsonl" ]]; then
      publish_v5_ranking_artifacts "$jsonl"
    fi
  else
    publish_status_html_report "$run_status" "No engine summary was available, so AIH v5 generated this status report instead of a ranking report."
  fi
  open_latest_html_report
}

if ((PUBLISH_LATEST_ONLY == 1)); then
  if [[ -x "$ROOT_DIR/bin/aih_v5_html_report" ]] && "$ROOT_DIR/bin/aih_v5_html_report"; then
    exit 0
  fi
  if ! publish_latest_summary; then
    publish_status_html_report 2 "No engine summary was available for publish-latest-only."
  fi
  open_latest_html_report
  exit 0
fi

csv_count() {
  local csv="$1"
  if [[ -z "$csv" ]]; then
    printf '0\n'
    return
  fi
  awk -v s="$csv" 'BEGIN { n=split(s, a, ","); print n }'
}

csv_field() {
  local csv="$1"
  local index="$2"
  awk -v s="$csv" -v wanted="$index" 'BEGIN { n=split(s, a, ","); if (wanted >= 1 && wanted <= n) print a[wanted] }'
}

csv_range() {
  local csv="$1"
  local start="$2"
  local count="$3"
  awk -v s="$csv" -v start="$start" -v count="$count" '
    BEGIN {
      n = split(s, a, ",")
      out = ""
      for (i = 0; i < count && start + i <= n; ++i) {
        if (out != "") out = out ","
        out = out a[start + i]
      }
      print out
    }'
}

print_csv_lines() {
  local csv="$1"
  local prefix="${2:-aih_v5:   }"
  local item index=0
  if [[ -z "$csv" ]]; then
    echo "${prefix}none" >&2
    return 0
  fi
  IFS=',' read -r -a _aih_v5_print_csv_items <<< "$csv"
  for item in "${_aih_v5_print_csv_items[@]}"; do
    [[ -n "$item" ]] || continue
    index=$((index + 1))
    echo "${prefix}${index}. $item" >&2
  done
}

repeat_csv_value() {
  local value="$1"
  local count="$2"
  awk -v value="$value" -v count="$count" '
    BEGIN {
      if (count !~ /^[0-9]+$/ || count < 1) {
        exit
      }
      out = ""
      for (i = 1; i <= count; ++i) {
        if (out != "") out = out ","
        out = out value
      }
      print out
    }'
}

passthru_has_arg() {
  local target="$1"
  local arg
  for arg in "${PASSTHRU_ARGS[@]}"; do
    if [[ "$arg" == "$target" ]]; then
      return 0
    fi
  done
  return 1
}

normalize_csv() {
  local value="$1"
  awk -v s="$value" '
    BEGIN {
      gsub(/[[:space:]]+/, ",", s)
      gsub(/,+/, ",", s)
      gsub(/^,|,$/, "", s)
      print s
    }'
}

positive_int_or_die() {
  local label="$1"
  local value="$2"
  if [[ ! "$value" =~ ^[1-9][0-9]*$ ]]; then
    echo "aih_v5: $label must be a positive integer: $value" >&2
    exit 2
  fi
}

derived_cloud_maxply() {
  local local_maxply="$1"
  local ratio="$2"
  positive_int_or_die "local maxply" "$local_maxply"
  positive_int_or_die "local/cloud maxply ratio" "$ratio"
  if ((local_maxply > AIH_V5_LOCAL_MAXPLY_CAP)); then
    local_maxply="$AIH_V5_LOCAL_MAXPLY_CAP"
  fi
  if ((ratio < 2)); then
    ratio=2
  fi
  if ((ratio > 4)); then
    ratio=4
  fi
  local derived="$(((local_maxply + ratio - 1) / ratio))"
  if ((derived > AIH_V5_CLOUD_MAXPLY_CAP)); then
    derived="$AIH_V5_CLOUD_MAXPLY_CAP"
  fi
  printf '%s\n' "$derived"
}

csv_contains() {
  local csv="$1"
  local wanted="$2"
  awk -v s="$csv" -v wanted="$wanted" '
    BEGIN {
      n = split(s, a, ",")
      for (i = 1; i <= n; ++i) {
        if (a[i] == wanted) {
          found = 1
        }
      }
      exit(found ? 0 : 1)
    }'
}

validate_reasoning_range() {
  local csv="$1"
  local invalid
  invalid="$(
    awk -v s="$csv" '
      BEGIN {
        n = split(s, a, ",")
        for (i = 1; i <= n; ++i) {
          if (a[i] !~ /^(low|medium|high|xhigh)$/) {
            if (out != "") out = out ","
            out = out a[i]
          }
        }
        print out
      }'
  )"
  if [[ -n "$invalid" ]]; then
    echo "aih_v5: invalid reasoning range value(s): $invalid" >&2
    echo "aih_v5: expected low, medium, high, xhigh" >&2
    exit 2
  fi
}

validate_verbosity_range() {
  local csv="$1"
  local invalid
  invalid="$(
    awk -v s="$csv" '
      BEGIN {
        n = split(s, a, ",")
        for (i = 1; i <= n; ++i) {
          if (a[i] !~ /^(low|medium|high)$/) {
            if (out != "") out = out ","
            out = out a[i]
          }
        }
        print out
      }'
  )"
  if [[ -n "$invalid" ]]; then
    echo "aih_v5: invalid verbosity range value(s): $invalid" >&2
    echo "aih_v5: expected low, medium, high" >&2
    exit 2
  fi
}

discover_local_agents() {
  local registry="$1"
  local discovered=""
  if [[ -r "$registry" ]]; then
    discovered="$(
      awk -F, '
        NR == 1 {
          active_schema = ($0 ~ /model_mode/)
          next
        }
        NR > 1 {
          for (i = 1; i <= NF; ++i) {
            gsub(/^"|"$/, "", $i)
          }
          if (active_schema && $3 == "local" && $4 == "ollama" && $7 == "pass") {
            if (out != "") out = out ","
            out = out $6
          } else if (!active_schema && $3 == "local" && $4 == "ollama" && $6 == "pass") {
            if (out != "") out = out ","
            out = out $5
          }
        }
        END { print out }
      ' "$registry"
    )"
  fi
  if [[ -n "$discovered" ]]; then
    printf '%s\n' "$discovered"
    return
  fi
  if command -v ollama >/dev/null 2>&1; then
    discovered="$(
      ollama list 2>/dev/null | awk '
        NR > 1 {
          if (out != "") out = out ","
          out = out $1
        }
        END { print out }
      '
    )" || discovered=""
  fi
  if [[ -n "$discovered" ]]; then
    printf '%s\n' "$discovered"
    return
  fi
  printf '%s\n' "$DEFAULT_LOCAL_AGENTS"
}

rotate_left_one() {
  local csv="$1"
  local n first rest
  n="$(csv_count "$csv")"
  if ((n <= 1)); then
    printf '%s\n' "$csv"
    return
  fi
  first="$(csv_field "$csv" 1)"
  rest="$(csv_range "$csv" 2 "$((n - 1))")"
  printf '%s,%s\n' "$rest" "$first"
}

round_robin_white_models() {
  local csv="$1"
  awk -v s="$csv" '
    BEGIN {
      n = split(s, a, ",")
      out = ""
      for (i = 1; i <= n; ++i) {
        for (j = 1; j <= n; ++j) {
          if (i == j) continue
          if (out != "") out = out ","
          out = out a[i]
        }
      }
      print out
    }'
}

round_robin_black_models() {
  local csv="$1"
  awk -v s="$csv" '
    BEGIN {
      n = split(s, a, ",")
      out = ""
      for (i = 1; i <= n; ++i) {
        for (j = 1; j <= n; ++j) {
          if (i == j) continue
          if (out != "") out = out ","
          out = out a[j]
        }
      }
      print out
    }'
}

ladder_rung_white_models() {
  local csv="$1"
  awk -v s="$csv" '
    BEGIN {
      n = split(s, a, ",")
      out = ""
      for (i = 1; i < n; ++i) {
        if (out != "") out = out ","
        out = out a[i]
      }
      print out
    }'
}

ladder_rung_black_models() {
  local csv="$1"
  awk -v s="$csv" '
    BEGIN {
      n = split(s, a, ",")
      out = ""
      for (i = 2; i <= n; ++i) {
        if (out != "") out = out ","
        out = out a[i]
      }
      print out
    }'
}

csv_from_index() {
  local csv="$1"
  local start="$2"
  awk -v s="$csv" -v start="$start" '
    BEGIN {
      n = split(s, a, ",")
      out = ""
      for (i = start; i <= n; ++i) {
        if (a[i] == "") continue
        if (out != "") out = out ","
        out = out a[i]
      }
      print out
    }'
}

csv_unique() {
  local csv="$1"
  awk -v s="$csv" '
    BEGIN {
      n = split(s, a, ",")
      for (i = 1; i <= n; ++i) {
        if (a[i] == "" || seen[a[i]]) continue
        seen[a[i]] = 1
        if (out != "") out = out ","
        out = out a[i]
      }
      print out
    }'
}

csv_sort_order() {
  local csv="$1"
  local order="$2"
  case "$order" in
    forward|"")
      printf '%s\n' "$csv"
      ;;
    alphabetical|alpha)
      awk -v s="$csv" 'BEGIN { n=split(s,a,","); for (i=1;i<=n;i++) if (a[i]!="") print a[i] }' |
        sort -f |
        paste -sd, -
      ;;
    reverse-alpha|reverse-alphabetical|reverse)
      awk -v s="$csv" 'BEGIN { n=split(s,a,","); for (i=1;i<=n;i++) if (a[i]!="") print a[i] }' |
        sort -fr |
        paste -sd, -
      ;;
    random|shuffle)
      awk -v s="$csv" 'BEGIN { n=split(s,a,","); for (i=1;i<=n;i++) if (a[i]!="") print a[i] }' |
        shuf |
        paste -sd, -
      ;;
    *)
      echo "aih_v5: unknown registration order '$order'; using forward order." >&2
      printf '%s\n' "$csv"
      ;;
  esac
}

registration_record() {
  local candidate="$1"
  local status="$2"
  local reason="$3"
  local elapsed_seconds="${4:-}"
  local timeout_seconds="${5:-}"
  reason="${reason//$'\n'/ }"
  reason="${reason//$'\r'/ }"
  reason="${reason//,/;}"
  printf '%s,%s,%s,%s,%s\n' "$candidate" "$status" "$reason" "$elapsed_seconds" "$timeout_seconds" >> "$REGISTRATION_STATUS_CSV"
}

registration_diag() {
  mkdir -p "$(dirname "$REGISTRATION_DIAGNOSTIC_LOG")"
  printf '%s %s\n' "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$*" >> "$REGISTRATION_DIAGNOSTIC_LOG"
}

capture_ollama_ps_diag() {
  local label="$1"
  registration_diag "ollama ps $label:"
  if ! command -v ollama >/dev/null 2>&1; then
    registration_diag "ollama command not found"
    return 0
  fi
  if command -v timeout >/dev/null 2>&1; then
    timeout 5 ollama ps >> "$REGISTRATION_DIAGNOSTIC_LOG" 2>&1 || registration_diag "ollama ps failed or timed out"
  else
    ollama ps >> "$REGISTRATION_DIAGNOSTIC_LOG" 2>&1 || registration_diag "ollama ps failed"
  fi
}

ollama_model_loaded() {
  local model="$1"
  ollama ps 2>/dev/null | awk -v model="$model" 'NR > 1 && $1 == model { found = 1 } END { exit found ? 0 : 1 }'
}

ollama_loaded_models_csv() {
  if ! command -v ollama >/dev/null 2>&1; then
    return 1
  fi
  if command -v timeout >/dev/null 2>&1; then
    timeout 5 ollama ps 2>/dev/null
  else
    ollama ps 2>/dev/null
  fi |
    awk 'NR > 1 && $1 != "" { print $1 }' |
    paste -sd, -
}

csv_intersection() {
  local left="$1"
  local right="$2"
  awk -v left="$left" -v right="$right" '
    BEGIN {
      nl = split(left, l, ",")
      nr = split(right, r, ",")
      for (i = 1; i <= nr; ++i) {
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", r[i])
        if (r[i] != "") ok[r[i]] = 1
      }
      for (i = 1; i <= nl; ++i) {
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", l[i])
        if (l[i] != "" && ok[l[i]] && !seen[l[i]]++) {
          if (out != "") out = out ","
          out = out l[i]
        }
      }
      print out
    }'
}

wait_for_ollama_model_unload() {
  local model="$1"
  local waited=0
  if ! command -v ollama >/dev/null 2>&1; then
    return 0
  fi
  echo "aih_v5: registration unload requested for local agent: $model" >&2
  registration_diag "registration unload requested model=$model"
  if command -v timeout >/dev/null 2>&1; then
    timeout 5 ollama stop "$model" >> "$REGISTRATION_DIAGNOSTIC_LOG" 2>&1 || true
  else
    ollama stop "$model" >> "$REGISTRATION_DIAGNOSTIC_LOG" 2>&1 || true
  fi
  while ollama_model_loaded "$model"; do
    if ((waited >= REGISTRATION_UNLOAD_TIMEOUT_SECONDS)); then
      echo "aih_v5: registration unload wait timed out for local agent: $model" >&2
      registration_diag "registration unload wait timed out model=$model waited=${waited}s"
      return 1
    fi
    sleep 1
    waited=$((waited + 1))
  done
  registration_diag "registration unload complete model=$model waited=${waited}s"
  return 0
}

registration_query_return_local() {
  local model="$1"
  local output status payload response_text timeout_seconds
  timeout_seconds="${REGISTRATION_CURRENT_TIMEOUT_SECONDS:-$REGISTRATION_TIMEOUT_SECONDS}"
  if ! command -v ollama >/dev/null 2>&1; then
    printf '%s\n' "ollama command not found"
    return 127
  fi
  if command -v curl >/dev/null 2>&1 && command -v jq >/dev/null 2>&1; then
    payload="$(
      jq -nc \
        --arg model "$model" \
        --arg prompt "$REGISTRATION_PROMPT" \
        --argjson num_predict "$REGISTRATION_NUM_PREDICT" \
        --arg keep_alive "$REGISTRATION_KEEP_ALIVE" \
        --argjson num_thread "$OLLAMA_NUM_THREAD" \
        '{model:$model,prompt:$prompt,stream:false,keep_alive:$keep_alive,options:{num_predict:$num_predict,temperature:0,num_thread:$num_thread}}'
    )"
    set +e
    output="$(
      curl -fsS \
        --connect-timeout 3 \
        --max-time "$timeout_seconds" \
        -H 'Content-Type: application/json' \
        -d "$payload" \
        http://127.0.0.1:11434/api/generate 2>&1
    )"
    status=$?
    set -e
    if ((status != 0)); then
      if ((status == 28)); then
        printf 'registration smoke timed out after %s seconds via Ollama HTTP generate\n' "$timeout_seconds"
        return "$status"
      fi
      printf '%s\n' "$output"
      return "$status"
    fi
    response_text="$(printf '%s\n' "$output" | jq -r '.response // ""' 2>/dev/null || true)"
    if [[ -z "${response_text//[[:space:]]/}" ]]; then
      printf '%s\n' "empty query-return response from Ollama HTTP generate"
      return 1
    fi
    printf '%s\n' "$response_text"
    return 0
  fi
  set +e
  if command -v timeout >/dev/null 2>&1; then
    output="$(timeout "$timeout_seconds" ollama run "$model" "$REGISTRATION_PROMPT" 2>&1)"
    status=$?
  else
    output="$(ollama run "$model" "$REGISTRATION_PROMPT" 2>&1)"
    status=$?
  fi
  set -e
  if ((status != 0)); then
    if ((status == 124)); then
      printf 'registration smoke timed out after %s seconds\n' "$timeout_seconds"
      return "$status"
    fi
    printf '%s\n' "$output"
    return "$status"
  fi
  if [[ -z "${output//[[:space:]]/}" ]]; then
    printf '%s\n' "empty query-return response"
    return 1
  fi
  printf '%s\n' "$output"
  return 0
}

registration_legal_move_from_text() {
  local text="$1"
  local legal_csv="$2"
  printf '%s\n' "$text" |
    awk -v legal_csv="$legal_csv" '
      BEGIN {
        n = split(legal_csv, legal, ",")
        for (i = 1; i <= n; ++i) legal_ok[legal[i]] = 1
      }
      {
        line = $0
        while (match(line, /[a-h][1-8][a-h][1-8][qrbn]?/)) {
          move = substr(line, RSTART, RLENGTH)
          if (legal_ok[move]) {
            print move
            exit
          }
          line = substr(line, RSTART + RLENGTH)
        }
      }'
}

registration_game_prompt() {
  local ply="$1"
  local fen="$2"
  local suggested="$3"
  local after_fen="$4"
  cat <<EOF
Chess move request.
Ply: $ply
FEN: $fen
Clue: use this legal UCI move: $suggested
Clue: current board FEN: $fen
Clue: board FEN after that suggested move: $after_fen
Return exactly one legal UCI move and nothing else.
Answer format: e2e4
No prose. No explanation. No punctuation.
EOF
}

registration_ollama_generate_local() {
  local model="$1"
  local prompt="$2"
  local output status payload response_text timeout_seconds
  timeout_seconds="${REGISTRATION_CURRENT_TIMEOUT_SECONDS:-$REGISTRATION_TIMEOUT_SECONDS}"
  if ! command -v ollama >/dev/null 2>&1; then
    printf '%s\n' "ollama command not found"
    return 127
  fi
  if ! command -v curl >/dev/null 2>&1 || ! command -v jq >/dev/null 2>&1; then
    printf '%s\n' "curl and jq are required for game-style registration smoke"
    return 127
  fi
  payload="$(
    jq -nc \
      --arg model "$model" \
      --arg prompt "$prompt" \
      --argjson num_predict "$REGISTRATION_GAME_NUM_PREDICT" \
      --arg keep_alive "$REGISTRATION_KEEP_ALIVE" \
      --argjson num_thread "$OLLAMA_NUM_THREAD" \
      '{model:$model,prompt:$prompt,stream:false,keep_alive:$keep_alive,options:{num_predict:$num_predict,temperature:0,num_thread:$num_thread}}'
  )"
  set +e
  output="$(
    curl -fsS \
      --connect-timeout 3 \
      --max-time "$timeout_seconds" \
      -H 'Content-Type: application/json' \
      -d "$payload" \
      http://127.0.0.1:11434/api/generate 2>&1
  )"
  status=$?
  set -e
  if ((status != 0)); then
    if ((status == 28)); then
      printf 'registration game smoke timed out after %s seconds via Ollama HTTP generate\n' "$timeout_seconds"
      return "$status"
    fi
    printf '%s\n' "$output"
    return "$status"
  fi
  response_text="$(printf '%s\n' "$output" | jq -r '.response // ""' 2>/dev/null || true)"
  if [[ -z "${response_text//[[:space:]]/}" ]]; then
    printf '%s\n' "empty game-style registration response from Ollama HTTP generate"
    return 1
  fi
  printf '%s\n' "$response_text"
  return 0
}

registration_game_smoke_once_local() {
  local model="$1"
  local ply prompt output status move
  local fen1="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1"
  local after1="rnbqkbnr/pppppppp/8/8/8/P7/1PPPPPPP/RNBQKBNR b - - 0 1"
  local legal1="a2a3,a2a4,b2b3,b2b4,c2c3,c2c4,d2d3,d2d4,e2e3,e2e4,f2f3,f2f4,g2g3,g2g4,h2h3,h2h4,b1a3,b1c3,g1f3,g1h3"
  local fen2="$after1"
  local after2="rnbqkbnr/1ppppppp/8/p7/8/P7/1PPPPPPP/RNBQKBNR w - - 0 1"
  local legal2="a7a5,a7a6,b7b5,b7b6,c7c5,c7c6,d7d5,d7d6,e7e5,e7e6,f7f5,f7f6,g7g5,g7g6,h7h5,h7h6,b8a6,b8c6,g8f6,g8h6"
  local max_plies="$REGISTRATION_GAME_SMOKE_PLIES"
  if ((max_plies < 1)); then
    max_plies=1
  fi
  if ((max_plies > 2)); then
    max_plies=2
  fi
  for ply in $(seq 1 "$max_plies"); do
    if ((ply == 1)); then
      prompt="$(registration_game_prompt 1 "$fen1" "a2a3" "$after1")"
      set +e
      output="$(registration_ollama_generate_local "$model" "$prompt")"
      status=$?
      set -e
      if ((status != 0)); then
        printf 'ply 1 failed: %s\n' "$output"
        return "$status"
      fi
      move="$(registration_legal_move_from_text "$output" "$legal1")"
    else
      prompt="$(registration_game_prompt 2 "$fen2" "a7a5" "$after2")"
      set +e
      output="$(registration_ollama_generate_local "$model" "$prompt")"
      status=$?
      set -e
      if ((status != 0)); then
        printf 'ply 2 failed: %s\n' "$output"
        return "$status"
      fi
      move="$(registration_legal_move_from_text "$output" "$legal2")"
    fi
    if [[ -z "$move" ]]; then
      printf 'ply %s failed: no legal UCI move parsed from response: %s\n' "$ply" "$output"
      return 1
    fi
    registration_diag "registration game smoke candidate=$model ply=$ply move=$move response=$(printf '%s' "$output" | tr '\n\r,' '   ')"
  done
  printf 'game-style smoke passed %s ply\n' "$max_plies"
  return 0
}

registration_game_smoke_local() {
  local model="$1"
  local attempt max_attempts output status
  max_attempts=$((REGISTRATION_FINAL_RETRY + 1))
  if ((max_attempts < 1)); then
    max_attempts=1
  fi
  for attempt in $(seq 1 "$max_attempts"); do
    set +e
    output="$(registration_game_smoke_once_local "$model")"
    status=$?
    set -e
    if ((status == 0)); then
      printf '%s attempt=%s/%s\n' "$output" "$attempt" "$max_attempts"
      return 0
    fi
    registration_diag "registration game smoke attempt failed candidate=$model attempt=$attempt/$max_attempts reason=$(printf '%s' "$output" | tr '\n\r,' '   ')"
    if ((status == 28)) || [[ "$output" == *"timed out"* ]]; then
      registration_diag "registration game smoke timeout terminal for candidate=$model attempt=$attempt/$max_attempts"
      printf '%s\n' "$output"
      return "$status"
    fi
    if ((attempt < max_attempts)); then
      echo "aih_v5: registration game smoke retry agent: $model attempt=$((attempt + 1))/$max_attempts" >&2
      wait_for_ollama_model_unload "$model" || true
    fi
  done
  printf '%s\n' "$output"
  return "$status"
}

register_candidate_agents() {
  local candidates="$1"
  local allow_midpass_reset="${2:-0}"
  local registered="" candidate output reason status total index consecutive_timeouts=0
  local candidate_started_at candidate_elapsed current_timeout observed_pass_max=0 dynamic_timeout
  local passed_count pass_rate batch_count=0 batch_candidates=""
  mkdir -p "$(dirname "$REGISTRATION_STATUS_CSV")"
  printf 'candidate,status,reason,elapsed_seconds,timeout_seconds\n' > "$REGISTRATION_STATUS_CSV"
  : > "$REGISTRATION_DIAGNOSTIC_LOG"
  REGISTRATION_CURRENT_TIMEOUT_SECONDS="$REGISTRATION_TIMEOUT_SECONDS"
  registration_diag "registration pass started candidates=$candidates mode=$REGISTRATION_MODE timeout=${REGISTRATION_TIMEOUT_SECONDS}s dynamic_timeout=$REGISTRATION_DYNAMIC_TIMEOUT_ENABLED multiplier=$REGISTRATION_DYNAMIC_TIMEOUT_MULTIPLIER min=${REGISTRATION_DYNAMIC_TIMEOUT_MIN_SECONDS}s prompt=$REGISTRATION_PROMPT num_predict=$REGISTRATION_NUM_PREDICT keep_alive=$REGISTRATION_KEEP_ALIVE"
  capture_ollama_ps_diag "before registration pass"
  candidates="$(csv_unique "$candidates")"
  candidates="$(csv_sort_order "$candidates" "$REGISTRATION_ORDER")"
  if [[ -z "$candidates" ]]; then
    return 0
  fi
  registration_diag "registration order applied order=$REGISTRATION_ORDER candidates=$candidates"
  IFS=',' read -r -a _aih_v5_registration_candidates <<< "$candidates"
  total="${#_aih_v5_registration_candidates[@]}"
  index=0
  echo "aih_v5: registration test:" >&2
  echo "aih_v5: starting..." >&2
  echo "aih_v5: registration order: $REGISTRATION_ORDER" >&2
  echo "aih_v5: registration candidates:" >&2
  print_csv_lines "$candidates"
  for candidate in "${_aih_v5_registration_candidates[@]}"; do
    if [[ -z "$candidate" ]]; then
      continue
    fi
    index=$((index + 1))
    if [[ -n "$batch_candidates" ]]; then
      batch_candidates+=","
    fi
    batch_candidates+="$candidate"
    current_timeout="${REGISTRATION_CURRENT_TIMEOUT_SECONDS:-$REGISTRATION_TIMEOUT_SECONDS}"
    candidate_started_at="$(date +%s)"
    echo "aih_v5: registration test starting agent $index/$total: $candidate timeout=${current_timeout}s" >&2
    if passthru_has_arg "--dry-run"; then
      candidate_elapsed=$(( $(date +%s) - candidate_started_at ))
      ((candidate_elapsed < 1)) && candidate_elapsed=0
      registration_record "$candidate" "pass" "dry-run planned query-return smoke" "$candidate_elapsed" "$current_timeout"
      echo "aih_v5: registration test passed agent $index/$total: $candidate elapsed=${candidate_elapsed}s" >&2
      if [[ -n "$registered" ]]; then
        registered+=","
      fi
      registered+="$candidate"
      continue
    fi
    if [[ "$REGISTRATION_SMOKE_ENABLED" != "1" && "$REGISTRATION_SMOKE_ENABLED" != "yes" && "$REGISTRATION_SMOKE_ENABLED" != "true" ]]; then
      candidate_elapsed=$(( $(date +%s) - candidate_started_at ))
      ((candidate_elapsed < 1)) && candidate_elapsed=0
      registration_record "$candidate" "pass" "registration smoke disabled by AIH_V5_REGISTRATION_SMOKE_ENABLED" "$candidate_elapsed" "$current_timeout"
      echo "aih_v5: registration test passed agent $index/$total: $candidate elapsed=${candidate_elapsed}s" >&2
      if [[ -n "$registered" ]]; then
        registered+=","
      fi
      registered+="$candidate"
      continue
    fi
    if spec_has_provider openai "$candidate" || spec_has_provider google "$candidate" || spec_has_provider anthropic "$candidate"; then
      if spec_has_provider openai "$candidate"; then
        prepare_provider_key openai
      elif spec_has_provider google "$candidate"; then
        prepare_provider_key google
      else
        prepare_provider_key anthropic
      fi
      candidate_elapsed=$(( $(date +%s) - candidate_started_at ))
      ((candidate_elapsed < 1)) && candidate_elapsed=0
      registration_record "$candidate" "fail" "cloud query-return registration smoke not implemented in v5 wrapper" "$candidate_elapsed" "$current_timeout"
      echo "aih_v5: registration test failed agent $index/$total: $candidate elapsed=${candidate_elapsed}s reason=cloud query-return registration smoke not implemented" >&2
      maybe_reset_registration_batch "$batch_candidates" "$index" "$total" "$batch_count"
      batch_count="$AIH_V5_REGISTRATION_BATCH_COUNT"
      if [[ "$AIH_V5_REGISTRATION_BATCH_DID_RESET" == "1" ]]; then
        batch_candidates=""
      fi
      continue
    fi
    set +e
    if [[ "$REGISTRATION_MODE" == "game" || "$REGISTRATION_MODE" == "game-style" || "$REGISTRATION_MODE" == "chess" ]]; then
      output="$(registration_game_smoke_local "$candidate")"
    else
      output="$(registration_query_return_local "$candidate")"
    fi
    status=$?
    set -e
    candidate_elapsed=$(( $(date +%s) - candidate_started_at ))
    ((candidate_elapsed < 1)) && candidate_elapsed=1
    if ((status == 0)); then
      if [[ "$REGISTRATION_MODE" == "game" || "$REGISTRATION_MODE" == "game-style" || "$REGISTRATION_MODE" == "chess" ]]; then
        reason="game-style registration smoke passed; useful for basic chess test"
      else
        reason="liveness registration smoke passed; agent responded through configured comm stack"
      fi
      registration_record "$candidate" "pass" "$reason" "$candidate_elapsed" "$current_timeout"
      registration_diag "registration pass candidate=$candidate elapsed=${candidate_elapsed}s timeout=${current_timeout}s"
      echo "aih_v5: registration test passed agent $index/$total: $candidate elapsed=${candidate_elapsed}s timeout=${current_timeout}s" >&2
      if [[ -n "$registered" ]]; then
        registered+=","
      fi
      registered+="$candidate"
      if [[ "$REGISTRATION_DYNAMIC_TIMEOUT_ENABLED" == "1" || "$REGISTRATION_DYNAMIC_TIMEOUT_ENABLED" == "yes" || "$REGISTRATION_DYNAMIC_TIMEOUT_ENABLED" == "true" ]]; then
        if ((candidate_elapsed > observed_pass_max)); then
          observed_pass_max="$candidate_elapsed"
        fi
        dynamic_timeout=$((observed_pass_max * REGISTRATION_DYNAMIC_TIMEOUT_MULTIPLIER))
        if ((dynamic_timeout < REGISTRATION_DYNAMIC_TIMEOUT_MIN_SECONDS)); then
          dynamic_timeout="$REGISTRATION_DYNAMIC_TIMEOUT_MIN_SECONDS"
        fi
        if ((dynamic_timeout > REGISTRATION_TIMEOUT_SECONDS)); then
          dynamic_timeout="$REGISTRATION_TIMEOUT_SECONDS"
        fi
        if ((dynamic_timeout < 1)); then
          dynamic_timeout=1
        fi
        if [[ "$dynamic_timeout" != "$REGISTRATION_CURRENT_TIMEOUT_SECONDS" ]]; then
          registration_diag "registration dynamic timeout update old=${REGISTRATION_CURRENT_TIMEOUT_SECONDS}s new=${dynamic_timeout}s observed_pass_max=${observed_pass_max}s"
          echo "aih_v5: registration dynamic timeout update: old=${REGISTRATION_CURRENT_TIMEOUT_SECONDS}s new=${dynamic_timeout}s observed_pass_max=${observed_pass_max}s" >&2
          REGISTRATION_CURRENT_TIMEOUT_SECONDS="$dynamic_timeout"
        fi
      fi
      wait_for_ollama_model_unload "$candidate" || true
      if [[ "$REGISTRATION_STOP_AFTER_PASSES" =~ ^[1-9][0-9]*$ &&
            "$(csv_count "$registered")" -ge "$REGISTRATION_STOP_AFTER_PASSES" ]]; then
        echo "aih_v5: registration stop-after-passes reached: $REGISTRATION_STOP_AFTER_PASSES" >&2
        registration_diag "registration stop-after-passes reached registered=$registered"
        break
      fi
    else
      if [[ "$REGISTRATION_MODE" == "game" || "$REGISTRATION_MODE" == "game-style" || "$REGISTRATION_MODE" == "chess" ]]; then
        reason="${output:-game-style registration smoke failed; good enough to work with Ollama but not good enough to be useful for a basic chess test}"
      else
        reason="${output:-liveness registration smoke failed; agent did not respond through configured comm stack}"
      fi
      registration_record "$candidate" "fail" "$reason" "$candidate_elapsed" "$current_timeout"
      registration_diag "registration fail candidate=$candidate elapsed=${candidate_elapsed}s timeout=${current_timeout}s reason=$reason"
      echo "aih_v5: registration test failed agent $index/$total: $candidate elapsed=${candidate_elapsed}s timeout=${current_timeout}s reason=$reason" >&2
      wait_for_ollama_model_unload "$candidate" || true
      if [[ "$reason" == *"timed out"* ]]; then
        consecutive_timeouts=$((consecutive_timeouts + 1))
      else
        consecutive_timeouts=0
      fi
      passed_count="$(csv_count "$registered")"
      pass_rate="$(
        awk -v passed="$passed_count" -v tested="$index" 'BEGIN {
          if (tested > 0) printf "%.1f", 100.0 * passed / tested
          else printf "0.0"
        }'
      )"
      if [[ "$reason" == *"timed out"* ]] &&
         awk -v rate="$pass_rate" -v threshold="$REGISTRATION_SYSTEMIC_PASS_RATE" 'BEGIN { exit !(rate >= threshold + 0) }'; then
        registration_diag "registration timeout treated as agent-specific candidate=$candidate pass_rate=${pass_rate}% threshold=${REGISTRATION_SYSTEMIC_PASS_RATE}%"
        echo "aih_v5: registration timeout filtered as agent-specific: $candidate pass_rate=${pass_rate}% threshold=${REGISTRATION_SYSTEMIC_PASS_RATE}%" >&2
        consecutive_timeouts=0
      fi
      if [[ "$allow_midpass_reset" == "1" &&
            -z "$registered" &&
            "$REGISTRATION_SYSTEMIC_TIMEOUT_THRESHOLD" =~ ^[1-9][0-9]*$ &&
            "$consecutive_timeouts" -ge "$REGISTRATION_SYSTEMIC_TIMEOUT_THRESHOLD" ]]; then
        registration_diag "systemic registration timeout detected after $consecutive_timeouts consecutive local timeouts with zero passes"
        echo "aih_v5: systemic registration timeout detected after $consecutive_timeouts consecutive local timeouts with zero passes." >&2
        echo "aih_v5: resetting Ollama stack and restarting registration test before trying more candidates." >&2
        printf '__AIH_V5_REGISTRATION_RESET_REQUIRED__\n'
        return 75
      fi
    fi
    maybe_reset_registration_batch "$batch_candidates" "$index" "$total" "$batch_count"
    batch_count="$AIH_V5_REGISTRATION_BATCH_COUNT"
    if [[ "$AIH_V5_REGISTRATION_BATCH_DID_RESET" == "1" ]]; then
      batch_candidates=""
    fi
  done
  echo "aih_v5: registration test complete." >&2
  emit_registration_timing_summary_text "$REGISTRATION_STATUS_CSV" >&2
  emit_registration_runtime_prediction_text "$REGISTRATION_STATUS_CSV" >&2
  echo "aih_v5: agents passed registration test:" >&2
  print_csv_lines "$registered"
  printf '%s\n' "$registered"
}

reset_local_ollama_stack_for_registration() {
  local candidates="$1"
  local reason="${2:-registration reset}"
  local before_label="${3:-before registration reset}"
  local after_label="${4:-after registration reset}"
  local candidate local_count=0 loaded_candidates stop_candidates
  IFS=',' read -r -a _aih_v5_reset_candidates <<< "$candidates"
  for candidate in "${_aih_v5_reset_candidates[@]}"; do
    if [[ -z "$candidate" ]]; then
      continue
    fi
    if spec_has_provider openai "$candidate" || spec_has_provider google "$candidate" || spec_has_provider anthropic "$candidate"; then
      continue
    fi
    local_count=$((local_count + 1))
  done
  if ((local_count == 0)); then
    echo "aih_v5: $reason, but no local Ollama candidates were present; Ollama reset skipped." >&2
    return 0
  fi
  echo "aih_v5: $reason; resetting local Ollama stack." >&2
  registration_diag "$reason; bounded Ollama reset local_count=$local_count"
  if ! command -v ollama >/dev/null 2>&1; then
    echo "aih_v5: ollama command not found; stack reset skipped." >&2
    return 0
  fi
  echo "aih_v5: ollama ps before reset:" >&2
  capture_ollama_ps_diag "$before_label"
  loaded_candidates="$(ollama_loaded_models_csv || true)"
  stop_candidates="$(csv_intersection "$candidates" "$loaded_candidates")"
  if [[ -z "$stop_candidates" ]]; then
    echo "aih_v5: no loaded local Ollama candidates matched reset scope." >&2
    registration_diag "$reason; no loaded local candidates matched reset scope loaded=$loaded_candidates candidates=$candidates"
    echo "aih_v5: ollama ps after reset:" >&2
    capture_ollama_ps_diag "$after_label"
    return 0
  fi
  IFS=',' read -r -a _aih_v5_reset_stop_candidates <<< "$stop_candidates"
  for candidate in "${_aih_v5_reset_stop_candidates[@]}"; do
    if [[ -z "$candidate" ]]; then
      continue
    fi
    if spec_has_provider openai "$candidate" || spec_has_provider google "$candidate" || spec_has_provider anthropic "$candidate"; then
      continue
    fi
    echo "aih_v5: stopping Ollama candidate for registration reset: $candidate" >&2
    if command -v timeout >/dev/null 2>&1; then
      timeout 5 ollama stop "$candidate" >&2 || true
    else
      ollama stop "$candidate" >&2 || true
    fi
  done
  echo "aih_v5: ollama ps after reset:" >&2
  capture_ollama_ps_diag "$after_label"
}

reset_loaded_local_ollama_stack() {
  local reason="${1:-global local Ollama stack reset}"
  local before_label="${2:-before global reset}"
  local after_label="${3:-after global reset}"
  local loaded_candidates candidate
  if ! command -v ollama >/dev/null 2>&1; then
    echo "aih_v5: ollama command not found; stack reset skipped." >&2
    return 0
  fi
  echo "aih_v5: $reason; resetting loaded local Ollama stack." >&2
  echo "aih_v5: ollama ps before reset:" >&2
  capture_ollama_ps_diag "$before_label"
  loaded_candidates="$(ollama_loaded_models_csv || true)"
  if [[ -z "$loaded_candidates" ]]; then
    echo "aih_v5: no loaded local Ollama models found for stack reset." >&2
    registration_diag "$reason; no loaded local Ollama models found"
    echo "aih_v5: ollama ps after reset:" >&2
    capture_ollama_ps_diag "$after_label"
    return 0
  fi
  IFS=',' read -r -a _aih_v5_loaded_reset_candidates <<< "$loaded_candidates"
  for candidate in "${_aih_v5_loaded_reset_candidates[@]}"; do
    [[ -n "$candidate" ]] || continue
    echo "aih_v5: stopping loaded Ollama model for stack reset: $candidate" >&2
    if command -v timeout >/dev/null 2>&1; then
      timeout 5 ollama stop "$candidate" >&2 || true
    else
      ollama stop "$candidate" >&2 || true
    fi
  done
  echo "aih_v5: ollama ps after reset:" >&2
  capture_ollama_ps_diag "$after_label"
}

maybe_reset_registration_batch() {
  local batch_candidates="$1"
  local index="$2"
  local total="$3"
  local batch_count="$4"
  AIH_V5_REGISTRATION_BATCH_COUNT="$batch_count"
  AIH_V5_REGISTRATION_BATCH_DID_RESET="0"
  if ! [[ "$REGISTRATION_BATCH_SIZE" =~ ^[1-9][0-9]*$ ]]; then
    return 0
  fi
  if ((index >= total)); then
    return 0
  fi
  if ((index % REGISTRATION_BATCH_SIZE != 0)); then
    return 0
  fi
  AIH_V5_REGISTRATION_BATCH_COUNT=$((batch_count + 1))
  echo "aih_v5: registration batch ${AIH_V5_REGISTRATION_BATCH_COUNT} complete after $index/$total candidates; resetting local stack before next batch." >&2
  registration_diag "registration batch ${AIH_V5_REGISTRATION_BATCH_COUNT} complete index=$index total=$total batch_size=$REGISTRATION_BATCH_SIZE"
  reset_local_ollama_stack_for_registration \
    "$batch_candidates" \
    "registration batch ${AIH_V5_REGISTRATION_BATCH_COUNT} complete after $index/$total candidates" \
    "before registration batch reset" \
    "after registration batch reset"
  AIH_V5_REGISTRATION_BATCH_DID_RESET="1"
  if [[ "$REGISTRATION_BATCH_SETTLE_SECONDS" =~ ^[0-9]+$ && "$REGISTRATION_BATCH_SETTLE_SECONDS" -gt 0 ]]; then
    echo "aih_v5: waiting ${REGISTRATION_BATCH_SETTLE_SECONDS} seconds for local comm stack to settle before next registration batch..." >&2
    sleep "$REGISTRATION_BATCH_SETTLE_SECONDS"
  fi
}

wait_after_registration_stack_reset() {
  local reason="$1"
  if [[ "$REGISTRATION_STACK_RESET_SETTLE_SECONDS" =~ ^[0-9]+$ && "$REGISTRATION_STACK_RESET_SETTLE_SECONDS" -gt 0 ]]; then
    echo "aih_v5: waiting ${REGISTRATION_STACK_RESET_SETTLE_SECONDS} seconds for local comm stack to settle after $reason..." >&2
    sleep "$REGISTRATION_STACK_RESET_SETTLE_SECONDS"
  fi
}

reset_local_ollama_stack_after_registration_failure() {
  local candidates="$1"
  reset_local_ollama_stack_for_registration \
    "$candidates" \
    "registration failed every candidate" \
    "before reset" \
    "after reset"
  wait_after_registration_stack_reset "registration failure reset"
  cat "$REGISTRATION_DIAGNOSTIC_LOG" >&2
  tail -n 20 "$REGISTRATION_DIAGNOSTIC_LOG" >&2 || true
}

register_candidate_agents_with_recovery() {
  local candidates="$1"
  local registered registered_count status
  reset_loaded_local_ollama_stack \
    "pre-registration startup reset" \
    "before pre-registration reset" \
    "after pre-registration reset"
  wait_after_registration_stack_reset "pre-registration reset"
  set +e
  registered="$(register_candidate_agents "$candidates" 1)"
  status=$?
  set -e
  if ((status == 75)); then
    reset_local_ollama_stack_after_registration_failure "$candidates"
    echo "aih_v5: waiting ${REGISTRATION_STACK_RESET_SETTLE_SECONDS} seconds for local comm stack to settle after reset..." >&2
    sleep "$REGISTRATION_STACK_RESET_SETTLE_SECONDS"
    echo "aih_v5: restarting registration test after systemic local timeout reset..." >&2
    registered="$(register_candidate_agents "$candidates" 0)"
  elif ((status != 0)); then
    echo "aih_v5: registration failed unexpectedly with status=$status" >&2
    exit "$status"
  fi
  registered_count="$(csv_count "$registered")"
  if [[ -z "$candidates" ]]; then
    printf '%s\n' "$registered"
    return 0
  fi
  if ((registered_count >= REGISTRATION_MIN_PASSES)); then
    echo "aih_v5: registration viable sample reached: passed=$registered_count min=$REGISTRATION_MIN_PASSES; timeout/nonviable agents filtered without reset." >&2
    registration_diag "registration viable sample reached passed=$registered_count min=$REGISTRATION_MIN_PASSES; no reset"
    printf '%s\n' "$registered"
    return 0
  fi
  if ((registered_count > 0)); then
    echo "aih_v5: registration produced a partial viable sample: passed=$registered_count min=$REGISTRATION_MIN_PASSES; using filtered sample without reset." >&2
    registration_diag "registration partial viable sample passed=$registered_count min=$REGISTRATION_MIN_PASSES; no reset"
    printf '%s\n' "$registered"
    return 0
  fi

  reset_local_ollama_stack_after_registration_failure "$candidates"
  echo "aih_v5: waiting ${REGISTRATION_STACK_RESET_SETTLE_SECONDS} seconds for local comm stack to settle after reset..." >&2
  sleep "$REGISTRATION_STACK_RESET_SETTLE_SECONDS"
  echo "aih_v5: restarting registration test after local stack reset..." >&2
  registered="$(register_candidate_agents "$candidates")"
  registered_count="$(csv_count "$registered")"
  if ((registered_count > 0)); then
    echo "aih_v5: registration recovered after stack reset; registered_agents=$registered" >&2
    printf '%s\n' "$registered"
    return 0
  fi

  LOCAL_AGENTS="$registered"
  echo "aih_v5: registration failed every candidate after stack reset; refusing to start tournament phases." >&2
  echo "aih_v5: registration status csv: $REGISTRATION_STATUS_CSV" >&2
  publish_status_html_report 2 "Registration failed every candidate after a local Ollama stack reset and retry, so AIH v5 stopped before round-robin or ladder play."
  open_latest_html_report
  exit 2
}

emit_registration_summary_line() {
  local registered_csv="$1"
  local tested_csv="$2"
  local passed tested pct
  passed="$(csv_count "$registered_csv")"
  tested="$(csv_count "$tested_csv")"
  if ((tested == 0)); then
    pct="0.0"
  else
    pct="$(awk -v p="$passed" -v t="$tested" 'BEGIN { printf "%.1f", (100.0 * p) / t }')"
  fi
  echo "aih_v5: registration summary: passed=$passed tested=$tested pct_passed=${pct}%" >&2
}

csv_filter_registered() {
  local csv="$1"
  local registered="$2"
  awk -v s="$csv" -v registered="$registered" '
    BEGIN {
      n = split(registered, r, ",")
      for (i = 1; i <= n; ++i) {
        if (r[i] != "") pass[r[i]] = 1
      }
      n = split(s, a, ",")
      for (i = 1; i <= n; ++i) {
        if (a[i] == "" || !pass[a[i]]) continue
        if (out != "") out = out ","
        out = out a[i]
      }
      print out
    }'
}

ranking_csv_lower_rungs_after_top4() {
  local csv="$1"
  if [[ ! -r "$csv" ]]; then
    return 1
  fi
  awk -F, '
    NR > 5 {
      agent = $1
      gsub(/^"|"$/, "", agent)
      if (agent == "") next
      if (out != "") out = out ","
      out = out agent
    }
    END { print out }
  ' "$csv"
}

ranking_csv_top_n() {
  local csv="$1"
  local count="$2"
  if [[ ! -r "$csv" ]]; then
    return 1
  fi
  awk -F, -v count="$count" '
    NR > 1 && (NR - 1) <= count {
      agent = $1
      gsub(/^"|"$/, "", agent)
      if (agent == "") next
      if (out != "") out = out ","
      out = out agent
    }
    END { print out }
  ' "$csv"
}

ranking_csv_best_of_pair() {
  local csv="$1"
  local first="$2"
  local second="$3"
  awk -F, -v first="$first" -v second="$second" '
    NR > 1 {
      agent = $1
      gsub(/^"|"$/, "", agent)
      if (agent == first || agent == second) {
        print agent
        exit
      }
    }
  ' "$csv"
}

ranking_csv_worst_of_pair() {
  local csv="$1"
  local first="$2"
  local second="$3"
  awk -F, -v first="$first" -v second="$second" '
    NR > 1 {
      agent = $1
      gsub(/^"|"$/, "", agent)
      if (agent == first || agent == second) {
        found = agent
      }
    }
    END {
      if (found != "") print found
    }
  ' "$csv"
}

latest_jsonl_path() {
  local latest_summary
  latest_summary="$(latest_summary_path)"
  if [[ -n "$latest_summary" ]]; then
    printf '%s\n' "${latest_summary%_summary.md}.jsonl"
  fi
}

reject_cloud_agent_spec() {
  local label="$1"
  local spec="$2"
  if ((LOCAL_SMOKE == 0)); then
    return
  fi
  if [[ "$spec" =~ (^|[[:space:],:=])(openai|anthropic|gemini|google): ||
        "$spec" =~ (^|[[:space:],:=])(gpt-|claude-|gemini-) ]]; then
    echo "aih_v5: cloud agent rejected for local smoke test: $label=$spec" >&2
    echo "aih_v5: smoke tests default to --local-smoke. Use --full-agent-set when cloud agents are intentionally part of the run." >&2
    exit 2
  fi
}

reject_default_self_play() {
  local white_csv="$1"
  local black_csv="$2"
  local board_count="$3"
  local allow_self="${AIH_V5_ALLOW_SELF_PLAY:-0}"
  if [[ "$allow_self" == "1" || "$allow_self" == "yes" || "$allow_self" == "true" ]]; then
    return
  fi
  if ! awk -v white="$white_csv" -v black="$black_csv" -v boards="$board_count" '
    BEGIN {
      nw = split(white, w, ",")
      nb = split(black, b, ",")
      if (boards !~ /^[0-9]+$/ || boards < 1) {
        boards = nw < nb ? nw : nb
      }
      for (i = 1; i <= boards; ++i) {
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", w[i])
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", b[i])
        if (w[i] != "" && b[i] != "" && w[i] == b[i]) {
          exit 1
        }
      }
      exit 0
    }'; then
    echo "aih_v5: refusing same-agent same-mode self-play by default." >&2
    echo "aih_v5: white-models=$white_csv" >&2
    echo "aih_v5: black-models=$black_csv" >&2
    echo "aih_v5: set AIH_V5_ALLOW_SELF_PLAY=1 only when self-play is intentional." >&2
    exit 2
  fi
}

if [[ -n "$CLOUD_SMOKE_PROVIDER" ]]; then
  prepare_provider_key "$CLOUD_SMOKE_PROVIDER"
  LOCAL_AGENTS="$(discover_local_agents "$LOCAL_AGENT_REGISTRY")"
  REGISTRATION_INPUT_AGENTS="$LOCAL_AGENTS"
  LOCAL_AGENTS="$(register_candidate_agents_with_recovery "$REGISTRATION_INPUT_AGENTS")"
  LOCAL_AGENT_COUNT="$(csv_count "$LOCAL_AGENTS")"
  emit_registration_summary_line "$LOCAL_AGENTS" "$REGISTRATION_INPUT_AGENTS"
  if [[ "$REGISTRATION_ONLY" == "1" ]]; then
    echo "aih_v5: registration status csv: $REGISTRATION_STATUS_CSV" >&2
    echo "aih_v5: registered agents: ${LOCAL_AGENTS:-none}" >&2
    exit 0
  fi
  if [[ -z "$LOCAL_AGENTS" || "$LOCAL_AGENT_COUNT" == "0" ]]; then
    echo "aih_v5: no local agents discovered for cloud smoke opponent." >&2
    echo "aih_v5: checked registry: $LOCAL_AGENT_REGISTRY" >&2
    exit 2
  fi
  case "$CLOUD_SMOKE_PROVIDER" in
    openai)
      DEFAULT_WHITE_MODELS="${AIH_V5_WHITE_MODELS:-openai:gpt-4.1-mini}"
      ;;
    google|gemini)
      DEFAULT_WHITE_MODELS="${AIH_V5_WHITE_MODELS:-gemini:gemini-3.5-flash-lite}"
      ;;
    anthropic)
      DEFAULT_WHITE_MODELS="${AIH_V5_WHITE_MODELS:-anthropic:claude-3-5-haiku}"
      ;;
    *)
      echo "aih_v5: unknown cloud smoke provider: $CLOUD_SMOKE_PROVIDER" >&2
      echo "aih_v5: expected openai, google/gemini, or anthropic" >&2
      exit 2
      ;;
  esac
  DEFAULT_BLACK_MODELS="${AIH_V5_BLACK_MODELS:-${AIH_V5_BLACK_MODELS:-$(csv_field "$LOCAL_AGENTS" "${AIH_V5_CLOUD_REPRESENTATIVE_LOCAL_INDEX:-1}")}}"
  DEFAULT_BOARDS="${AIH_V5_BOARDS:-1}"
  export AICHESS_REASONING_PERFORMANCE_MODE="${AICHESS_REASONING_PERFORMANCE_MODE:-medium}"
  export AICHESS_OPENAI_REASONING_EFFORT="${AICHESS_OPENAI_REASONING_EFFORT:-medium}"
  export AICHESS_OPENAI_TEXT_VERBOSITY="${AICHESS_OPENAI_TEXT_VERBOSITY:-medium}"
  AIH_V5_REFERENCE_CONFIG="${AIH_V5_REFERENCE_CONFIG:-aih_v5_cloud_provider_key_smoke_${CLOUD_SMOKE_PROVIDER}_medium_20260729}"
elif [[ -n "$CLOUD_REPRESENTATIVE_PROVIDER" ]]; then
  prepare_provider_key "$CLOUD_REPRESENTATIVE_PROVIDER"
  LOCAL_AGENTS="$(discover_local_agents "$LOCAL_AGENT_REGISTRY")"
  EXPLICIT_AGENT_SPECS="$(normalize_csv "${AIH_V5_WHITE_MODELS:-},${AIH_V5_BLACK_MODELS:-}")"
  if [[ -n "$EXPLICIT_AGENT_SPECS" ]]; then
    LOCAL_AGENTS="$(csv_unique "$EXPLICIT_AGENT_SPECS")"
  else
    LOCAL_AGENTS="$(csv_unique "$DEFAULT_LOCAL_AGENTS,$LOCAL_AGENTS")"
  fi
  REGISTRATION_INPUT_AGENTS="$LOCAL_AGENTS"
  LOCAL_AGENTS="$(register_candidate_agents_with_recovery "$REGISTRATION_INPUT_AGENTS")"
  LOCAL_AGENT_COUNT="$(csv_count "$LOCAL_AGENTS")"
  emit_registration_summary_line "$LOCAL_AGENTS" "$REGISTRATION_INPUT_AGENTS"
  if [[ "$REGISTRATION_ONLY" == "1" ]]; then
    echo "aih_v5: registration status csv: $REGISTRATION_STATUS_CSV" >&2
    echo "aih_v5: registered agents: ${LOCAL_AGENTS:-none}" >&2
    exit 0
  fi
  CLOUD_REPRESENTATIVE_START="${AIH_V5_CLOUD_REPRESENTATIVE_LOCAL_START:-${AIH_V5_CLOUD_REPRESENTATIVE_LOCAL_INDEX:-1}}"
  CLOUD_REPRESENTATIVE_COUNT="${AIH_V5_CLOUD_REPRESENTATIVE_LOCAL_COUNT:-$LOCAL_AGENT_COUNT}"
  if ((CLOUD_REPRESENTATIVE_START < 1)); then
    CLOUD_REPRESENTATIVE_START=1
  fi
  if ((CLOUD_REPRESENTATIVE_COUNT < 1)); then
    CLOUD_REPRESENTATIVE_COUNT=1
  fi
  if ((LOCAL_AGENT_COUNT > 0 && CLOUD_REPRESENTATIVE_START > LOCAL_AGENT_COUNT)); then
    CLOUD_REPRESENTATIVE_START="$LOCAL_AGENT_COUNT"
  fi
  if [[ -z "$LOCAL_AGENTS" || "$LOCAL_AGENT_COUNT" == "0" ]]; then
      echo "aih_v5: no loc agt for cld rep run." >&2
    echo "aih_v5: checked registry: $LOCAL_AGENT_REGISTRY" >&2
    exit 2
  fi
  case "$CLOUD_REPRESENTATIVE_PROVIDER" in
    google|gemini)
      DEFAULT_WHITE_MODELS="${AIH_V5_WHITE_MODELS:-gemini:gemini-3.5-flash-lite}"
      ;;
    *)
      echo "aih_v5: bad cld rep prv: $CLOUD_REPRESENTATIVE_PROVIDER" >&2
      echo "aih_v5: expected google/gemini" >&2
      exit 2
      ;;
  esac
  DEFAULT_BLACK_MODELS="${AIH_V5_BLACK_MODELS:-$(csv_range "$LOCAL_AGENTS" "$CLOUD_REPRESENTATIVE_START" "$CLOUD_REPRESENTATIVE_COUNT")}"
  DEFAULT_BOARDS="${AIH_V5_BOARDS:-$(csv_count "$DEFAULT_BLACK_MODELS")}"
  DEFAULT_WHITE_MODELS="${AIH_V5_WHITE_MODELS:-$(repeat_csv_value "$DEFAULT_WHITE_MODELS" "$DEFAULT_BOARDS")}"
  export AICHESS_REASONING_PERFORMANCE_MODE="${AICHESS_REASONING_PERFORMANCE_MODE:-medium}"
  export AICHESS_OPENAI_REASONING_EFFORT="${AICHESS_OPENAI_REASONING_EFFORT:-medium}"
  export AICHESS_OPENAI_TEXT_VERBOSITY="${AICHESS_OPENAI_TEXT_VERBOSITY:-medium}"
  AIH_V5_REFERENCE_CONFIG="${AIH_V5_REFERENCE_CONFIG:-aih_v5_cld_rep_${CLOUD_REPRESENTATIVE_PROVIDER}_vs_loc_20260803}"
else
  if [[ "$SMOKE_STAGE" == "full-agent-set" ]]; then
    if [[ -z "${AIH_V5_WHITE_MODELS:-}" || -z "${AIH_V5_BLACK_MODELS:-}" ]]; then
      echo "aih_v5: full-agent-set requires explicit AIH_V5_WHITE_MODELS and AIH_V5_BLACK_MODELS." >&2
      echo "aih_v5: this prevents accidentally treating the local Ollama cache as the full v5 roster." >&2
      exit 2
    fi
    prepare_keys_for_specs "${AIH_V5_WHITE_MODELS},${AIH_V5_BLACK_MODELS},${AIH_V5_REFEREE_MODELS:-harness},${PASSTHRU_ARGS[*]}"
  fi
  LOCAL_AGENTS="$(discover_local_agents "$LOCAL_AGENT_REGISTRY")"
  EXPLICIT_AGENT_SPECS="$(normalize_csv "${AIH_V5_WHITE_MODELS:-},${AIH_V5_BLACK_MODELS:-}")"
  if [[ -n "$EXPLICIT_AGENT_SPECS" ]]; then
    LOCAL_AGENTS="$(csv_unique "$EXPLICIT_AGENT_SPECS")"
  else
    LOCAL_AGENTS="$(csv_unique "$DEFAULT_LOCAL_AGENTS,$LOCAL_AGENTS")"
  fi
  PAIR_START="${AIH_V5_LOCAL_PAIR_START:-1}"
  if [[ "$TOURNAMENT_FORMAT" == "round-robin" || "$TOURNAMENT_FORMAT" == "round-robin-ladder" ]]; then
    PAIR_COUNT="${AIH_V5_LOCAL_PAIR_COUNT:-3}"
  elif [[ "$TOURNAMENT_FORMAT" == "top4-ladder-rungs" ]]; then
    DEFAULT_TOP4_PAIR_COUNT="$((LADDER_RUNGS + 1))"
    PAIR_COUNT="${AIH_V5_LOCAL_PAIR_COUNT:-$DEFAULT_TOP4_PAIR_COUNT}"
  else
    PAIR_COUNT="${AIH_V5_LOCAL_PAIR_COUNT:-1}"
  fi
  if ((PAIR_START < 1)); then
    PAIR_START=1
  fi
  if ((PAIR_COUNT < 1)); then
    PAIR_COUNT=1
  fi
  LOCAL_AGENT_COUNT="$(csv_count "$LOCAL_AGENTS")"
  if ((LOCAL_AGENT_COUNT > 0 && PAIR_START > LOCAL_AGENT_COUNT)); then
    PAIR_START="$LOCAL_AGENT_COUNT"
  fi
  if [[ "$REGISTRATION_CANDIDATE_COUNT" == "all" ]]; then
    SELECTED_REGISTRATION_AGENTS="$(csv_from_index "$LOCAL_AGENTS" "$PAIR_START")"
  elif [[ "$REGISTRATION_CANDIDATE_COUNT" =~ ^[1-9][0-9]*$ ]]; then
    SELECTED_REGISTRATION_AGENTS="$(csv_range "$LOCAL_AGENTS" "$PAIR_START" "$REGISTRATION_CANDIDATE_COUNT")"
  else
    echo "aih_v5: registration_candidate_count must be 'all' or a positive integer: $REGISTRATION_CANDIDATE_COUNT" >&2
    exit 2
  fi
  echo "aih_v5: selected registration candidates:" >&2
  print_csv_lines "$SELECTED_REGISTRATION_AGENTS"
  LOCAL_AGENTS="$(register_candidate_agents_with_recovery "$SELECTED_REGISTRATION_AGENTS")"
  LOCAL_AGENT_COUNT="$(csv_count "$LOCAL_AGENTS")"
  emit_registration_summary_line "$LOCAL_AGENTS" "$SELECTED_REGISTRATION_AGENTS"
  if [[ "$REGISTRATION_ONLY" == "1" ]]; then
    echo "aih_v5: registration status csv: $REGISTRATION_STATUS_CSV" >&2
    echo "aih_v5: registered agents: ${LOCAL_AGENTS:-none}" >&2
    exit 0
  fi

  SELECTED_LOCAL_AGENTS="$(csv_range "$LOCAL_AGENTS" 1 "$PAIR_COUNT")"
  SELECTED_LOCAL_AGENT_COUNT="$(csv_count "$SELECTED_LOCAL_AGENTS")"
  if [[ "$TOURNAMENT_FORMAT" == "top4-ladder-rungs" && "$LADDER_RUNGS" -gt "$SELECTED_LOCAL_AGENT_COUNT" ]]; then
    echo "aih_v5: top4-ladder-rungs requires ladder_rungs <= registered selected contestants." >&2
    echo "aih_v5: ladder_rungs=$LADDER_RUNGS registered_selected=$SELECTED_LOCAL_AGENT_COUNT" >&2
    echo "aih_v5: registration status csv: $REGISTRATION_STATUS_CSV" >&2
    publish_status_html_report 2 "Registration produced too few usable agents for the selected top-4 ladder-rungs configuration. Filtered candidates are listed below."
    open_latest_html_report
    exit 2
  fi
  if [[ "$TOURNAMENT_FORMAT" == "top4-ladder-rungs" && "$SELECTED_LOCAL_AGENT_COUNT" -lt "$((LADDER_RUNGS + 1))" ]]; then
    echo "aih_v5: top4-ladder-rungs requires ladder_rungs+1 registered contestants for the placement ladder stage." >&2
    echo "aih_v5: ladder_rungs=$LADDER_RUNGS registered_selected=$SELECTED_LOCAL_AGENT_COUNT required=$((LADDER_RUNGS + 1))" >&2
    echo "aih_v5: registration status csv: $REGISTRATION_STATUS_CSV" >&2
    publish_status_html_report 2 "Registration produced fewer usable agents than the selected top-4 ladder-rungs tournament requires. Filtered candidates are listed below."
    open_latest_html_report
    exit 2
  fi
  if [[ -n "${AIH_V5_WHITE_MODELS:-}" ]]; then
    DEFAULT_WHITE_MODELS="$(csv_filter_registered "${AIH_V5_WHITE_MODELS:-}" "$LOCAL_AGENTS")"
  else
    DEFAULT_WHITE_MODELS="$SELECTED_LOCAL_AGENTS"
  fi
  if [[ -n "${AIH_V5_BLACK_MODELS:-}" ]]; then
    DEFAULT_BLACK_MODELS="$(csv_filter_registered "${AIH_V5_BLACK_MODELS:-}" "$LOCAL_AGENTS")"
  else
    DEFAULT_BLACK_MODELS="$(csv_range "$(rotate_left_one "$LOCAL_AGENTS")" "$PAIR_START" "$PAIR_COUNT")"
  fi
  if [[ ( "$TOURNAMENT_FORMAT" == "round-robin" || "$TOURNAMENT_FORMAT" == "round-robin-ladder" ) &&
        -z "${AIH_V5_WHITE_MODELS:-}" &&
        -z "${AIH_V5_BLACK_MODELS:-}" ]]; then
    DEFAULT_WHITE_MODELS="$(round_robin_white_models "$SELECTED_LOCAL_AGENTS")"
    DEFAULT_BLACK_MODELS="$(round_robin_black_models "$SELECTED_LOCAL_AGENTS")"
  fi
  DEFAULT_BOARDS="${AIH_V5_BOARDS:-$(csv_count "$DEFAULT_WHITE_MODELS")}"
  if [[ -z "$DEFAULT_WHITE_MODELS" || -z "$DEFAULT_BLACK_MODELS" || "$DEFAULT_BOARDS" == "0" ]]; then
    echo "aih_v5: no local agents discovered." >&2
    echo "aih_v5: checked registry: $LOCAL_AGENT_REGISTRY" >&2
    publish_status_html_report 2 "No usable local agents were available after registration, so AIH v5 stopped before tournament play."
    open_latest_html_report
    exit 2
  fi
fi

ALL_AGENT_SPECS="$DEFAULT_WHITE_MODELS,$DEFAULT_BLACK_MODELS,${AIH_V5_REFEREE_MODELS:-harness},${PASSTHRU_ARGS[*]}"
CLOUD_MATRIX_APPLIES=0
if spec_has_provider openai "$ALL_AGENT_SPECS" ||
   spec_has_provider google "$ALL_AGENT_SPECS" ||
   spec_has_provider anthropic "$ALL_AGENT_SPECS"; then
  CLOUD_MATRIX_APPLIES=1
fi

case "$SMOKE_STAGE" in
  local-prog)
    DEFAULT_MAXPLYS="${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V5_RESPONSE_ATTEMPTS:-1}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V5_MAX_FATAL_TURN_ERRORS:-1}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V5_OUTPUT_TOKENS:-${AIH_V5_LOCAL_OUTPUT_TOKENS:-$STARTING_TOKENS_PER_INPUT}}"
    DEFAULT_LOGLVL="${AIH_V5_LOGLVL:-2}"
    DEFAULT_CLUE_MODE="${AIH_V5_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V5_REFERENCE_CONFIG:-aih_v5_local_prog_smoke_lowmaxply_harness_referee_20260729}"
    ;;
  local-retry)
    DEFAULT_MAXPLYS="${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V5_RESPONSE_ATTEMPTS:-1}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V5_MAX_FATAL_TURN_ERRORS:-1}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V5_OUTPUT_TOKENS:-${AIH_V5_LOCAL_OUTPUT_TOKENS:-$STARTING_TOKENS_PER_INPUT}}"
    DEFAULT_LOGLVL="${AIH_V5_LOGLVL:-5}"
    DEFAULT_CLUE_MODE="${AIH_V5_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V5_REFERENCE_CONFIG:-aih_v5_local_retry_concede_smoke_harness_referee_20260729}"
    ;;
  local-expand)
    DEFAULT_MAXPLYS="${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V5_RESPONSE_ATTEMPTS:-3}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V5_MAX_FATAL_TURN_ERRORS:-1}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V5_OUTPUT_TOKENS:-${AIH_V5_LOCAL_OUTPUT_TOKENS:-$STARTING_TOKENS_PER_INPUT}}"
    DEFAULT_LOGLVL="${AIH_V5_LOGLVL:-4}"
    DEFAULT_CLUE_MODE="${AIH_V5_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V5_REFERENCE_CONFIG:-aih_v5_local_expand_smoke_harness_referee_20260729}"
    if [[ -z "${AIH_V5_LOCAL_PAIR_COUNT:-}" && -z "${AIH_V5_WHITE_MODELS:-}" && -z "${AIH_V5_BLACK_MODELS:-}" ]]; then
      echo "aih_v5: local-expand selected with default pair count $DEFAULT_BOARDS." >&2
      echo "aih_v5: set AIH_V5_LOCAL_PAIR_COUNT to scale this stage deliberately." >&2
    fi
    ;;
  cloud-provider-key)
    LOCAL_BASE_MAXPLYS="${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}"
    DEFAULT_MAXPLYS="$(derived_cloud_maxply "$LOCAL_BASE_MAXPLYS" "$AIH_V5_LOCAL_CLOUD_MAXPLY_RATIO")"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V5_RESPONSE_ATTEMPTS:-3}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V5_MAX_FATAL_TURN_ERRORS:-3}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V5_OUTPUT_TOKENS:-$STARTING_TOKENS_PER_INPUT}"
    DEFAULT_LOGLVL="${AIH_V5_LOGLVL:-4}"
    DEFAULT_CLUE_MODE="${AIH_V5_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V5_REFERENCE_CONFIG:-aih_v5_cloud_provider_key_entitlement_smoke_${CLOUD_SMOKE_PROVIDER}_medium_20260729}"
    ;;
  cloud-rep)
    LOCAL_BASE_MAXPLYS="${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}"
    DEFAULT_MAXPLYS="$(derived_cloud_maxply "$LOCAL_BASE_MAXPLYS" "$AIH_V5_LOCAL_CLOUD_MAXPLY_RATIO")"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V5_RESPONSE_ATTEMPTS:-3}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V5_MAX_FATAL_TURN_ERRORS:-3}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V5_OUTPUT_TOKENS:-$STARTING_TOKENS_PER_INPUT}"
    DEFAULT_LOGLVL="${AIH_V5_LOGLVL:-4}"
    DEFAULT_CLUE_MODE="${AIH_V5_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V5_REFERENCE_CONFIG:-aih_v5_cld_rep_${CLOUD_REPRESENTATIVE_PROVIDER}_vs_loc_20260803}"
    ;;
  full-agent-set)
    if ((CLOUD_MATRIX_APPLIES == 1)); then
      LOCAL_BASE_MAXPLYS="${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}"
      DEFAULT_MAXPLYS="$(derived_cloud_maxply "$LOCAL_BASE_MAXPLYS" "$AIH_V5_LOCAL_CLOUD_MAXPLY_RATIO")"
    else
      DEFAULT_MAXPLYS="${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}"
    fi
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V5_RESPONSE_ATTEMPTS:-3}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V5_MAX_FATAL_TURN_ERRORS:-1}"
    if ((CLOUD_MATRIX_APPLIES == 1)); then
      DEFAULT_OUTPUT_TOKENS="${AIH_V5_OUTPUT_TOKENS:-2048}"
    else
      DEFAULT_OUTPUT_TOKENS="${AIH_V5_OUTPUT_TOKENS:-${AIH_V5_LOCAL_OUTPUT_TOKENS:-$STARTING_TOKENS_PER_INPUT}}"
    fi
    DEFAULT_LOGLVL="${AIH_V5_LOGLVL:-4}"
    DEFAULT_CLUE_MODE="${AIH_V5_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V5_REFERENCE_CONFIG:-aih_v5_full_agent_set_smoke_harness_referee_20260729}"
    ;;
  *)
    echo "aih_v5: unknown smoke stage: $SMOKE_STAGE" >&2
    echo "aih_v5: expected local-prog, local-retry, local-expand, cloud-provider-key, cloud-rep, or full-agent-set" >&2
    exit 2
    ;;
esac

reject_cloud_agent_spec "AIH_V5_WHITE_MODELS" "$DEFAULT_WHITE_MODELS"
reject_cloud_agent_spec "AIH_V5_BLACK_MODELS" "$DEFAULT_BLACK_MODELS"
reject_cloud_agent_spec "arguments" "${PASSTHRU_ARGS[*]}"
reject_default_self_play "$DEFAULT_WHITE_MODELS" "$DEFAULT_BLACK_MODELS" "$DEFAULT_BOARDS"

if [[ -z "$REASONING_RANGE" ]]; then
  if [[ "$ENABLE_REASONING_MATRIX" == "1" || "$ENABLE_REASONING_MATRIX" == "yes" || "$ENABLE_REASONING_MATRIX" == "true" ]]; then
    REASONING_RANGE="low,medium,high,xhigh"
  else
    REASONING_RANGE="${AICHESS_REASONING_PERFORMANCE_MODE:-medium}"
  fi
fi
if [[ -z "$VERBOSITY_RANGE" ]]; then
  if [[ "$EXPLICIT_VERBOSITY_RANGE" == "1" &&
        ( "$ENABLE_REASONING_MATRIX" == "1" || "$ENABLE_REASONING_MATRIX" == "yes" || "$ENABLE_REASONING_MATRIX" == "true" ) ]]; then
    VERBOSITY_RANGE="low,medium,high"
  else
    VERBOSITY_RANGE="${AICHESS_VERBOSITY:-${AICHESS_OPENAI_TEXT_VERBOSITY:-medium}}"
  fi
fi

REASONING_RANGE="$(normalize_csv "$REASONING_RANGE")"
VERBOSITY_RANGE="$(normalize_csv "$VERBOSITY_RANGE")"
validate_reasoning_range "$REASONING_RANGE"
validate_verbosity_range "$VERBOSITY_RANGE"

if ! spec_has_provider openai "$ALL_AGENT_SPECS"; then
  VERBOSITY_RANGE="medium"
fi

if ((CLOUD_MATRIX_APPLIES == 0)); then
  REASONING_RANGE="medium"
  VERBOSITY_RANGE="medium"
fi

if ((CLOUD_MATRIX_APPLIES == 1)); then
  CLOUD_BASE_MAXPLYS="$(derived_cloud_maxply "${AIH_V5_LOCAL_MAXPLYS:-${AIH_V5_MAXPLYS:-$AIH_V5_LOCAL_MAXPLY_CAP}}" "$AIH_V5_LOCAL_CLOUD_MAXPLY_RATIO")"
fi

if ((CLOUD_MATRIX_APPLIES == 1 && DEFAULT_MAXPLYS > CLOUD_BASE_MAXPLYS)); then
  if [[ "${AIH_V5_CLOUD_MAXPLY_AFTER_REASONING_SWEEP:-0}" != "1" &&
        "${AIH_V5_CLOUD_MAXPLY_AFTER_REASONING_SWEEP:-0}" != "yes" &&
        "${AIH_V5_CLOUD_MAXPLY_AFTER_REASONING_SWEEP:-0}" != "true" ]]; then
    echo "aih_v5: refusing cloud maxply=$DEFAULT_MAXPLYS before reasoning/thought-level sweep is explicitly confirmed." >&2
    echo "aih_v5: first broaden AIH_V5_ALLOWED_REASONINGS or --allowed-reasonings; then set AIH_V5_CLOUD_MAXPLY_AFTER_REASONING_SWEEP=1 to raise --local-maxplys or lower --local-cloud-maxply-ratio." >&2
    exit 2
  fi
fi

if ((CLOUD_MATRIX_APPLIES == 1)); then
  DEFAULT_MOVE_TIMEOUT="${AIH_V5_MOVE_TIMEOUT_SECONDS:-10}"
  DEFAULT_STACK_TIMEOUT="${AIH_V5_STACK_TIMEOUT_SECONDS:-10}"
  DEFAULT_GAME_TIMEOUT="${AIH_V5_GAME_TIMEOUT_SECONDS:-1800}"
else
  DEFAULT_MOVE_TIMEOUT="${AIH_V5_MOVE_TIMEOUT_SECONDS:-20}"
  DEFAULT_STACK_TIMEOUT="${AIH_V5_STACK_TIMEOUT_SECONDS:-20}"
  DEFAULT_GAME_TIMEOUT="${AIH_V5_GAME_TIMEOUT_SECONDS:-900}"
fi

if [[ ! -x "$ENGINE" ]]; then
  echo "aih_v5: engine is not executable: $ENGINE" >&2
  echo "aih_v5: run ./tools/build_aih_v5.sh first" >&2
  exit 127
fi

export AICHESS_TRACE_STRING_CHARS="${AICHESS_TRACE_STRING_CHARS:-1048576}"
export AICHESS_OLLAMA_NUM_THREAD="${AICHESS_OLLAMA_NUM_THREAD:-$OLLAMA_NUM_THREAD}"
export AICHESS_OUTPUT_TOKEN_INCREASE_RATIO="${AICHESS_OUTPUT_TOKEN_INCREASE_RATIO:-$TOKEN_INCREASE_RATIO}"
export AICHESS_OUTPUT_TOKEN_DECREASE_STEP="${AICHESS_OUTPUT_TOKEN_DECREASE_STEP:-$TOKEN_DECREASE_STEP}"
export AICHESS_OUTPUT_TOKEN_INCREASE_STEP="${AICHESS_OUTPUT_TOKEN_INCREASE_STEP:-$TOKEN_INCREASE_STEP}"
export AIH_V5_STACK_RESET_SETTLE_SECONDS="${AIH_V5_STACK_RESET_SETTLE_SECONDS:-$REGISTRATION_STACK_RESET_SETTLE_SECONDS}"

ENGINE_ARGS=(
  --mode aichess
  --white-models "$DEFAULT_WHITE_MODELS"
  --black-models "$DEFAULT_BLACK_MODELS"
  --boards "$DEFAULT_BOARDS"
  --board-concurrency "$AIH_V5_BOARD_CONCURRENCY"
  --loops "${AIH_V5_LOOPS:-1}"
  --referee harness
  --mxply "$DEFAULT_MAXPLYS"
  --cnrtlm "$DEFAULT_RESPONSE_ATTEMPTS"
  --max-illegal "$DEFAULT_FATAL_TURN_ERRORS"
  --move-timeout "$DEFAULT_MOVE_TIMEOUT"
  --stack-timeout "$DEFAULT_STACK_TIMEOUT"
  --gmto "$DEFAULT_GAME_TIMEOUT"
  --otkns "$DEFAULT_OUTPUT_TOKENS"
  --auto-output-tokens
  --loglvl "$DEFAULT_LOGLVL"
  --clue-mode "$DEFAULT_CLUE_MODE"
)

BASE_ENGINE_ARGS=("${ENGINE_ARGS[@]}")

if [[ "$SMOKE_STAGE" == "cloud-rep" || "$TOURNAMENT_FORMAT" == "ladder" ]]; then
  ENGINE_ARGS+=(--tournament-bracket)
fi

if [[ "$ENABLE_BOARD_AWARENESS" == "1" || "$ENABLE_BOARD_AWARENESS" == "yes" || "$ENABLE_BOARD_AWARENESS" == "true" ]]; then
  ENGINE_ARGS+=(--board-awareness-probe)
fi

REASONING_COUNT="$(csv_count "$REASONING_RANGE")"
VERBOSITY_COUNT="$(csv_count "$VERBOSITY_RANGE")"
MATRIX_COUNT="$((REASONING_COUNT * VERBOSITY_COUNT))"

renice_pid_quietly() {
  local nice_level="$1"
  local pid="$2"
  local err
  err="$(renice -n "$nice_level" -p "$pid" 2>&1 >/dev/null)" || {
    if [[ "${AIH_V5_RENICE_DIAGNOSTICS:-1}" == "1" ]]; then
      echo "aih_v5: renice failed: pid=$pid nice=$nice_level err=${err:-unknown}" >&2
    fi
    return 1
  }
  return 0
}

renice_local_ollama_processes() {
  local nice_level="$1"
  local matches pid ni comm args matched=0 changed=0
  if [[ "$RENICE_OLLAMA_ENABLED" != "1" &&
        "$RENICE_OLLAMA_ENABLED" != "yes" &&
        "$RENICE_OLLAMA_ENABLED" != "true" ]]; then
    return 0
  fi
  matches="$(
    ps -u "$(id -u)" -o pid=,ni=,comm=,args= |
      awk '
        $3 == "ollama" ||
        $3 ~ /^llama/ ||
        $3 ~ /ollama/ ||
        index($0, "/ollama") ||
        index($0, "ollama_llama") ||
        index($0, "llama-server") ||
        index($0, "llama runner") { print }
      '
  )"
  while read -r pid ni comm args; do
    [[ -n "${pid:-}" ]] || continue
    case "$comm" in
      awk|ps|grep|rg|sed|sort)
        continue
        ;;
    esac
    matched=$((matched + 1))
    if renice_pid_quietly "$nice_level" "$pid"; then
      changed=$((changed + 1))
    fi
    if [[ "${AIH_V5_RENICE_DIAGNOSTICS:-1}" == "1" ]]; then
      echo "aih_v5: renice candidate: pid=$pid old_ni=$ni target_ni=$nice_level comm=$comm args=${args:-}" >&2
    fi
  done <<< "$matches"
  if [[ "${AIH_V5_RENICE_DIAGNOSTICS:-1}" == "1" ]]; then
    if ((matched == 0)); then
      echo "aih_v5: renice ollama: no same-user ollama/llama worker matched" >&2
    else
      echo "aih_v5: renice ollama: matched=$matched changed=$changed target_ni=$nice_level" >&2
    fi
  fi
}

log_ollama_nice_state() {
  if [[ "${AIH_V5_RENICE_DIAGNOSTICS:-1}" != "1" ]]; then
    return 0
  fi
  echo "aih_v5: ollama nice state:" >&2
  ps -u "$(id -u)" -o pid=,ni=,pri=,comm=,args= |
    awk '
      $4 == "ollama" ||
      $4 ~ /^llama/ ||
      $4 ~ /ollama/ ||
      index($0, "/ollama") ||
      index($0, "ollama_llama") ||
      index($0, "llama-server") ||
      index($0, "llama runner") { print "aih_v5:   " $0 }
    ' >&2 || true
}

run_with_heartbeat() {
  local label="$1"
  shift
  local interval="${AIH_V5_HEARTBEAT_SECONDS:-5}"
  local elapsed=0
  local pid status current_nice next_nice_at
  current_nice="$NICE_INITIAL"
  next_nice_at="$NICE_STEP_SECONDS"
  if [[ "$DYNAMIC_NICE_ENABLED" == "1" ||
        "$DYNAMIC_NICE_ENABLED" == "yes" ||
        "$DYNAMIC_NICE_ENABLED" == "true" ]]; then
    nice -n "$current_nice" "$@" &
  else
    "$@" &
  fi
  pid="$!"
  if [[ "$DYNAMIC_NICE_ENABLED" == "1" ||
        "$DYNAMIC_NICE_ENABLED" == "yes" ||
        "$DYNAMIC_NICE_ENABLED" == "true" ]]; then
    renice_pid_quietly "$current_nice" "$pid"
    renice_local_ollama_processes "$current_nice"
    log_ollama_nice_state
    echo "aih_v5: dynamic nice start: nice=$current_nice max=$NICE_MAX step=$NICE_STEP step_seconds=$NICE_STEP_SECONDS renice_ollama=$RENICE_OLLAMA_ENABLED label=$label" >&2
  fi
  while kill -0 "$pid" 2>/dev/null; do
    sleep "$interval"
    if kill -0 "$pid" 2>/dev/null; then
      elapsed=$((elapsed + interval))
      if [[ "$DYNAMIC_NICE_ENABLED" == "1" ||
            "$DYNAMIC_NICE_ENABLED" == "yes" ||
            "$DYNAMIC_NICE_ENABLED" == "true" ]]; then
        if ((NICE_STEP_SECONDS > 0 && elapsed >= next_nice_at && current_nice < NICE_MAX)); then
          current_nice=$((current_nice + NICE_STEP))
          if ((current_nice > NICE_MAX)); then
            current_nice="$NICE_MAX"
          fi
          next_nice_at=$((elapsed + NICE_STEP_SECONDS))
          echo "aih_v5: dynamic nice update: nice=$current_nice elapsed=${elapsed}s label=$label" >&2
        fi
        renice_pid_quietly "$current_nice" "$pid"
        renice_local_ollama_processes "$current_nice"
        log_ollama_nice_state
      fi
      echo "aih_v5: still running after ${elapsed}s: $label" >&2
    fi
  done
  set +e
  wait "$pid"
  status="$?"
  set -e
  return "$status"
}

run_engine_for_config() {
  local reasoning="$1"
  local verbosity="$2"
  local reference_config="$3"
  export AICHESS_REASONING_PERFORMANCE_MODE="$reasoning"
  export AICHESS_OPENAI_REASONING_EFFORT="$reasoning"
  export AICHESS_VERBOSITY="$verbosity"
  export AICHESS_OPENAI_TEXT_VERBOSITY="$verbosity"
  run_with_heartbeat "eng refcfg=$reference_config" \
    "$ENGINE" \
    "${ENGINE_ARGS[@]}" \
    --reference-config "$reference_config" \
    "${PASSTHRU_ARGS[@]}"
}

run_engine_with_args_for_config() {
  local reasoning="$1"
  local verbosity="$2"
  local reference_config="$3"
  shift 3
  export AICHESS_REASONING_PERFORMANCE_MODE="$reasoning"
  export AICHESS_OPENAI_REASONING_EFFORT="$reasoning"
  export AICHESS_VERBOSITY="$verbosity"
  export AICHESS_OPENAI_TEXT_VERBOSITY="$verbosity"
  run_with_heartbeat "eng refcfg=$reference_config" \
    "$ENGINE" \
    "$@" \
    --reference-config "$reference_config" \
    "${PASSTHRU_ARGS[@]}"
}

run_engine_pairings_for_config() {
  local reasoning="$1"
  local verbosity="$2"
  local reference_config="$3"
  local white_models="$4"
  local black_models="$5"
  local boards="$6"
  local bracket="$7"
  local args=(
    --mode aichess
    --white-models "$white_models"
    --black-models "$black_models"
    --boards "$boards"
    --board-concurrency "$AIH_V5_BOARD_CONCURRENCY"
    --loops "${AIH_V5_LOOPS:-1}"
    --referee harness
    --mxply "$DEFAULT_MAXPLYS"
    --cnrtlm "$DEFAULT_RESPONSE_ATTEMPTS"
    --max-illegal "$DEFAULT_FATAL_TURN_ERRORS"
    --move-timeout "$DEFAULT_MOVE_TIMEOUT"
    --stack-timeout "$DEFAULT_STACK_TIMEOUT"
    --gmto "$DEFAULT_GAME_TIMEOUT"
    --otkns "$DEFAULT_OUTPUT_TOKENS"
    --auto-output-tokens
    --loglvl "$DEFAULT_LOGLVL"
    --clue-mode "$DEFAULT_CLUE_MODE"
  )
  if [[ "$bracket" == "1" ]]; then
    args+=(--tournament-bracket)
  fi
  run_engine_with_args_for_config "$reasoning" "$verbosity" "$reference_config" "${args[@]}"
}

run_round_robin_ladder_for_config() {
  local reasoning="$1"
  local verbosity="$2"
  local reference_config="$3"
  local rr_reference ladder_reference
  rr_reference="${reference_config}_phase-round-robin"
  ladder_reference="${reference_config}_phase-ladder"
  echo "aih_v5: hybrid phase 1/2 round-robin refcfg=$rr_reference" >&2
  run_engine_with_args_for_config "$reasoning" "$verbosity" "$rr_reference" "${BASE_ENGINE_ARGS[@]}"
  echo "aih_v5: hybrid phase 2/2 ladder refcfg=$ladder_reference" >&2
  run_engine_with_args_for_config "$reasoning" "$verbosity" "$ladder_reference" "${BASE_ENGINE_ARGS[@]}" --tournament-bracket
}

run_top4_ladder_rungs_for_config() {
  local reasoning="$1"
  local verbosity="$2"
  local reference_config="$3"
  local lower_reference ladder_reference seed_reference seed_white seed_black seed_boards lower_agents lower_count lower_white lower_black lower_boards top_agents top_count lower_round_reference rr_round ranked_top
  local semi_reference final_reference consolation_reference a1 a2 a3 a4 latest_csv winner_a winner_b loser_a loser_b stage_status
  lower_reference="${reference_config}_phase-lower-rungs-round-robin"
  ladder_reference="${reference_config}_phase-top4-ladder"
  lower_agents="$(csv_from_index "$SELECTED_LOCAL_AGENTS" "$((LADDER_RUNGS + 2))")"
  lower_count="$(csv_count "$lower_agents")"
  if [[ -z "$lower_agents" || "$lower_count" == "0" || "$lower_count" == "1" ]]; then
    echo "aih_v5: top4/rungs phase 1/2 lower round-robin skipped; fewer than two lower-rung agents." >&2
  else
    echo "aih_v5: starting round-robin mode..." >&2
    lower_white="$(round_robin_white_models "$lower_agents")"
    lower_black="$(round_robin_black_models "$lower_agents")"
    lower_boards="$(csv_count "$lower_white")"
    for rr_round in $(seq 1 "$ROUND_ROBIN_ROUNDS"); do
      lower_round_reference="${lower_reference}_round-${rr_round}"
      echo "aih_v5: round-robin round $rr_round started..." >&2
      echo "aih_v5: top4/rungs phase 1/2 lower round-robin round=$rr_round/$ROUND_ROBIN_ROUNDS refcfg=$lower_round_reference lower_agents=$lower_agents boards=$lower_boards" >&2
      run_engine_pairings_for_config "$reasoning" "$verbosity" "$lower_round_reference" "$lower_white" "$lower_black" "$lower_boards" 0
    done
    echo "aih_v5: round-robin mode complete." >&2
  fi

  top_agents="$(csv_range "$SELECTED_LOCAL_AGENTS" 1 "$((LADDER_RUNGS + 1))")"
  top_count="$(csv_count "$top_agents")"
  if [[ -z "$top_agents" || "$top_count" == "0" || "$top_count" == "1" ]]; then
    echo "aih_v5: top4/rungs top-4 ladder skipped; fewer than two top-rung agents." >&2
    return 0
  fi
  if ((top_count != 4 || LADDER_RUNGS != 3)); then
    echo "aih_v5: top ladder currently requires exactly four ladder-stage contestants and --ladder_rungs=3." >&2
    echo "aih_v5: ladder_rungs=$LADDER_RUNGS top_ladder_contestants=$top_count" >&2
    exit 2
  fi

  seed_reference="${reference_config}_phase-top4-seed-round-robin"
  seed_white="$(round_robin_white_models "$top_agents")"
  seed_black="$(round_robin_black_models "$top_agents")"
  seed_boards="$(csv_count "$seed_white")"
  echo "aih_v5: top4 seed round-robin starting..." >&2
  echo "aih_v5: top4/rungs seed round-robin refcfg=$seed_reference top_agents=$top_agents boards=$seed_boards" >&2
  run_engine_pairings_for_config "$reasoning" "$verbosity" "$seed_reference" "$seed_white" "$seed_black" "$seed_boards" 0
  stage_status=$?
  if ((stage_status != 0)); then
    return "$stage_status"
  fi
  if ! passthru_has_arg "--dry-run"; then
    publish_latest_summary
    publish_v5_ranking_artifacts "$(latest_jsonl_path)"
    latest_csv="$ROOT_DIR/AIH_V5_LATEST_RANKINGS.csv"
    ranked_top="$(ranking_csv_top_n "$latest_csv" 4)"
    ranked_top="$(csv_filter_registered "$ranked_top" "$top_agents")"
    if [[ "$(csv_count "$ranked_top")" == "4" ]]; then
      top_agents="$ranked_top"
      echo "aih_v5: top4 seed ranking order: $top_agents" >&2
    else
      echo "aih_v5: top4 seed ranking unavailable; using registration order for ladder." >&2
    fi
  fi

  a1="$(csv_field "$top_agents" 1)"
  a2="$(csv_field "$top_agents" 2)"
  a3="$(csv_field "$top_agents" 3)"
  a4="$(csv_field "$top_agents" 4)"
  semi_reference="${ladder_reference}_semifinals"
  final_reference="${ladder_reference}_final"
  consolation_reference="${ladder_reference}_consolation"

  echo "aih_v5: ladder test starting..." >&2
  echo "aih_v5: ladder test rr1 started..." >&2
  echo "aih_v5: top4/rungs ladder semifinals refcfg=$semi_reference boards=2 pairs=$a1-vs-$a2,$a3-vs-$a4" >&2
  run_engine_pairings_for_config "$reasoning" "$verbosity" "$semi_reference" "$a1,$a3" "$a2,$a4" 2 1
  stage_status=$?
  if ((stage_status != 0)); then
    return "$stage_status"
  fi

  if passthru_has_arg "--dry-run"; then
    winner_a="$a1"
    loser_a="$a2"
    winner_b="$a3"
    loser_b="$a4"
  else
    publish_latest_summary
    publish_v5_ranking_artifacts "$(latest_jsonl_path)"
    latest_csv="$ROOT_DIR/AIH_V5_LATEST_RANKINGS.csv"
    winner_a="$(ranking_csv_best_of_pair "$latest_csv" "$a1" "$a2")"
    loser_a="$(ranking_csv_worst_of_pair "$latest_csv" "$a1" "$a2")"
    winner_b="$(ranking_csv_best_of_pair "$latest_csv" "$a3" "$a4")"
    loser_b="$(ranking_csv_worst_of_pair "$latest_csv" "$a3" "$a4")"
  fi

  if [[ -z "$winner_a" || -z "$winner_b" || -z "$loser_a" || -z "$loser_b" ]]; then
    echo "aih_v5: unable to derive semifinal winners/losers for ladder placement." >&2
    echo "aih_v5: latest ranking csv: ${latest_csv:-none}" >&2
    return 1
  fi

  echo "aih_v5: ladder test winners board started..." >&2
  echo "aih_v5: top4/rungs ladder final refcfg=$final_reference board=1 pair=$winner_a-vs-$winner_b" >&2
  run_engine_pairings_for_config "$reasoning" "$verbosity" "$final_reference" "$winner_a" "$winner_b" 1 1
  stage_status=$?
  if ((stage_status != 0)); then
    return "$stage_status"
  fi

  echo "aih_v5: ladder test losers board started..." >&2
  echo "aih_v5: top4/rungs ladder consolation refcfg=$consolation_reference board=1 pair=$loser_a-vs-$loser_b" >&2
  run_engine_pairings_for_config "$reasoning" "$verbosity" "$consolation_reference" "$loser_a" "$loser_b" 1 1
  echo "aih_v5: ladder test complete." >&2
}

emit_run_configuration() {
  local reference_config="$1"
  echo "aih_v5: run config:" >&2
  echo "aih_v5:   tournament_format=$TOURNAMENT_FORMAT ladder_rungs=$LADDER_RUNGS round_robin_rounds=$ROUND_ROBIN_ROUNDS ranking_mode=$RANKING_MODE ranking_aih_weight=$RANKING_AIH_WEIGHT ranking_turn_time_weight=$RANKING_TURN_TIME_WEIGHT" >&2
  echo "aih_v5:   smoke_stage=$SMOKE_STAGE local_smoke=$LOCAL_SMOKE cloud_provider=${CLOUD_SMOKE_PROVIDER:-none} cloud_representative=${CLOUD_REPRESENTATIVE_PROVIDER:-none}" >&2
  echo "aih_v5:   white_models=$DEFAULT_WHITE_MODELS" >&2
  echo "aih_v5:   black_models=$DEFAULT_BLACK_MODELS" >&2
  echo "aih_v5:   boards=$DEFAULT_BOARDS loops=${AIH_V5_LOOPS:-1} maxply=$DEFAULT_MAXPLYS response_attempts=$DEFAULT_RESPONSE_ATTEMPTS max_illegal=$DEFAULT_FATAL_TURN_ERRORS" >&2
  echo "aih_v5:   board_concurrency=$AIH_V5_BOARD_CONCURRENCY" >&2
  echo "aih_v5:   ollama_num_thread=$OLLAMA_NUM_THREAD" >&2
  echo "aih_v5:   dynamic_nice=$DYNAMIC_NICE_ENABLED nice_initial=$NICE_INITIAL nice_step=$NICE_STEP nice_max=$NICE_MAX nice_step_seconds=$NICE_STEP_SECONDS renice_ollama=$RENICE_OLLAMA_ENABLED" >&2
  echo "aih_v5:   starting_tokens_per_input=$STARTING_TOKENS_PER_INPUT token_decrease_step=$TOKEN_DECREASE_STEP token_increase_step=$TOKEN_INCREASE_STEP token_increase_ratio=$TOKEN_INCREASE_RATIO" >&2
  echo "aih_v5:   move_timeout=$DEFAULT_MOVE_TIMEOUT stack_timeout=$DEFAULT_STACK_TIMEOUT game_timeout=$DEFAULT_GAME_TIMEOUT output_tokens=$DEFAULT_OUTPUT_TOKENS loglvl=$DEFAULT_LOGLVL clue_mode=$DEFAULT_CLUE_MODE" >&2
  echo "aih_v5:   reasoning_range=$REASONING_RANGE verbosity_range=$VERBOSITY_RANGE matrix_count=$MATRIX_COUNT reference_config=$reference_config" >&2
  echo "aih_v5:   engine=$ENGINE" >&2
  if ((${#PASSTHRU_ARGS[@]} > 0)); then
    printf 'aih_v5:   passthru=' >&2
    printf ' %q' "${PASSTHRU_ARGS[@]}" >&2
    printf '\n' >&2
  else
    echo "aih_v5:   passthru=none" >&2
  fi
}

if ((MATRIX_COUNT <= 1)); then
  reasoning="$(csv_field "$REASONING_RANGE" 1)"
  verbosity="$(csv_field "$VERBOSITY_RANGE" 1)"
  export AICHESS_REASONING_PERFORMANCE_MODE="$reasoning"
  export AICHESS_OPENAI_REASONING_EFFORT="$reasoning"
  export AICHESS_VERBOSITY="$verbosity"
  export AICHESS_OPENAI_TEXT_VERBOSITY="$verbosity"
  emit_run_configuration "$DEFAULT_REFERENCE_CONFIG"
  set +e
  if [[ "$TOURNAMENT_FORMAT" == "round-robin-ladder" ]]; then
    run_round_robin_ladder_for_config "$reasoning" "$verbosity" "$DEFAULT_REFERENCE_CONFIG"
  elif [[ "$TOURNAMENT_FORMAT" == "top4-ladder-rungs" ]]; then
    run_top4_ladder_rungs_for_config "$reasoning" "$verbosity" "$DEFAULT_REFERENCE_CONFIG"
  else
    run_with_heartbeat "eng refcfg=$DEFAULT_REFERENCE_CONFIG" \
      "$ENGINE" \
      "${ENGINE_ARGS[@]}" \
      --reference-config "$DEFAULT_REFERENCE_CONFIG" \
      "${PASSTHRU_ARGS[@]}"
  fi
  run_status=$?
  set -e
  if ((run_status == 0)) && ! passthru_has_arg "--dry-run"; then
    echo "aih_v5: eng ok; prep rpt..." >&2
  fi
  publish_run_html_report "$run_status"
  echo "aih_v5: exiting with status $run_status." >&2
  exit "$run_status"
fi

if ((CLOUD_MATRIX_APPLIES == 0)); then
  echo "aih_v5: mat req; no cloud cap agent." >&2
  echo "aih_v5: run 1 loc cfg." >&2
  emit_run_configuration "$DEFAULT_REFERENCE_CONFIG"
  set +e
  if [[ "$TOURNAMENT_FORMAT" == "round-robin-ladder" ]]; then
    run_round_robin_ladder_for_config "$(csv_field "$REASONING_RANGE" 1)" "$(csv_field "$VERBOSITY_RANGE" 1)" "$DEFAULT_REFERENCE_CONFIG"
  elif [[ "$TOURNAMENT_FORMAT" == "top4-ladder-rungs" ]]; then
    run_top4_ladder_rungs_for_config "$(csv_field "$REASONING_RANGE" 1)" "$(csv_field "$VERBOSITY_RANGE" 1)" "$DEFAULT_REFERENCE_CONFIG"
  else
    run_with_heartbeat "eng refcfg=$DEFAULT_REFERENCE_CONFIG" \
      "$ENGINE" \
      "${ENGINE_ARGS[@]}" \
      --reference-config "$DEFAULT_REFERENCE_CONFIG" \
      "${PASSTHRU_ARGS[@]}"
  fi
  run_status=$?
  set -e
  if ((run_status == 0)) && ! passthru_has_arg "--dry-run"; then
    echo "aih_v5: eng ok; prep rpt..." >&2
  fi
  publish_run_html_report "$run_status"
  echo "aih_v5: exiting with status $run_status." >&2
  exit "$run_status"
fi

echo "aih_v5: run rsn/vrb mat: rsn=$REASONING_RANGE vrb=$VERBOSITY_RANGE" >&2
emit_run_configuration "$DEFAULT_REFERENCE_CONFIG"
matrix_failures=0
for reasoning_index in $(seq 1 "$REASONING_COUNT"); do
  reasoning="$(csv_field "$REASONING_RANGE" "$reasoning_index")"
  for verbosity_index in $(seq 1 "$VERBOSITY_COUNT"); do
    verbosity="$(csv_field "$VERBOSITY_RANGE" "$verbosity_index")"
    matrix_reference="${DEFAULT_REFERENCE_CONFIG}_reasoning-${reasoning}_verbosity-${verbosity}"
    echo "aih_v5: mat cfg rsn=$reasoning vrb=$verbosity refcfg=$matrix_reference" >&2
    if [[ "$TOURNAMENT_FORMAT" == "round-robin-ladder" ]]; then
      if ! run_round_robin_ladder_for_config "$reasoning" "$verbosity" "$matrix_reference"; then
        matrix_failures=$((matrix_failures + 1))
      fi
    elif [[ "$TOURNAMENT_FORMAT" == "top4-ladder-rungs" ]]; then
      if ! run_top4_ladder_rungs_for_config "$reasoning" "$verbosity" "$matrix_reference"; then
        matrix_failures=$((matrix_failures + 1))
      fi
    elif ! run_engine_for_config "$reasoning" "$verbosity" "$matrix_reference"; then
      matrix_failures=$((matrix_failures + 1))
    fi
  done
done

if ((matrix_failures > 0)); then
  echo "aih_v5: mat fail cfgs: $matrix_failures" >&2
  exit 1
fi

echo "aih_v5: eng mat ok; prep rpt..." >&2
publish_run_html_report 0
echo "aih_v5: exiting with status 0." >&2
