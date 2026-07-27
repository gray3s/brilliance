#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY="$ROOT_DIR/bin/test_cloud_agents_binary"
LIVENESS_RUNNER="$ROOT_DIR/bin/run_adapter_liveness_probes"
SET_API_KEY="$ROOT_DIR/setapikey.sh"

ALL_AGENTS=(01 02 03 04 05 06 07 08 09 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 10)
CLOUD_AGENTS=(01 02 03 04 05 06 07 08 09)
OPENAI_CLOUD_AGENTS=(01 02 03 04)
GEMINI_CLOUD_AGENTS=(05)
ANTHROPIC_CLOUD_AGENTS=(06 07 08 09)
LOCAL_AGENTS=(11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 10)
CHESS_CANDIDATE_AGENTS=(11 12 14 15 19 1C 1D 1E 1F 10)
CAPABILITY_PROBE_AGENTS=(13 16 17 18 1A 1B)

usage() {
  cat <<'EOF'
Usage:
  ./agents-all-test.sh
  ./agents-all-test.sh --clue-mode 6
  ./agents-all-test.sh --clue-ladder
  ./agents-all-test.sh --legacy-clue-ladder
  ./agents-all-test.sh --aichess-only
  ./agents-all-test.sh --liveness-only
  ./agents-all-test.sh --selector 09
  ./agents-all-test.sh --cloud-only
  ./agents-all-test.sh --openai-only
  ./agents-all-test.sh --gemini-only
  ./agents-all-test.sh --anthropic-only
  ./agents-all-test.sh --local-only
  ./agents-all-test.sh --chess-candidates-only
  ./agents-all-test.sh --capability-probe-only
  ./agents-all-test.sh --complete-game --local-only
  ./agents-all-test.sh --dry-run
  ./agents-all-test.sh -- --max-plies 20 --game-timeout 300

Runs the two AIH v2 baseline smoke surfaces for all selected agents:
  1. AIChess one-board self-play at the easiest clue mode.
  2. Preschool adapter liveness / rational-response probes.

AIChess is treated as the practical AIH v1 game prototype.
For the broader AIH v2 Class1/Class2/Class3 database catalog, use:

  ../../v2/run_aih_v2_eval.sh --catalog-only --include-cloud-skips

Defaults:
  agents:    01 02 03 04 05 06 07 08 09 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 10
  clues:     6
  log level: 3, including child run transcript output

Agent keys:
  01 openai:gpt-4.1-mini
  02 openai:gpt-4o-mini
  03 openai:gpt-5-mini
  04 openai:gpt-5-nano
  05 gemini:gemini-3.1-flash-lite
  06 anthropic:claude-opus-4
  07 anthropic:claude-sonnet-4
  08 anthropic:claude-3-7-sonnet
  09 anthropic:claude-3-5-haiku
  11 granite4:3b
  12 qwen2.5-coder:3b
  13 qwen2.5:0.5b
  14 qwen2.5:latest
  15 qwen:4b
  16 robit/qwen3.5-9b-r7-research:q4km
  17 smollm2:135m
  18 gemma3:270m
  19 llama3.2:1b
  1A gemma3:1b
  1B tinyllama:latest
  1C phi3:mini
  1D phi4-mini:latest
  1E mistral:latest
  1F llama3.2:3b
  10 gemma3:4b

Run controls:
  --clue-mode N narrows the run to one clue mode.
  --clue-ladder runs easiest-to-hardest clue modes: 6 5 4 3 2 1 0.
  --legacy-clue-ladder runs the original four clue modes: 3 2 1 0.
  --aichess-only runs only the AIChess smoke surface.
  --liveness-only runs only the preschool adapter liveness smoke surface.
  --chess-candidates-only runs downloaded local agents that passed the
    non-Qt one-move UCI legality screen.
  --capability-probe-only runs downloaded local agents that failed the
    one-move UCI legality screen and should not be used for 1v1 chess yet.
  --complete-game raises the binary limits for a complete-game attempt:
    --max-plies 120 --game-timeout 900 --stack-timeout 90 --corrections 5
  Arguments after -- are passed through to ./bin/test_cloud_agents_binary.

Key handling:
  OpenAI cloud slots need OPENAI_API_KEY. If it is not already set, this script
  sources ./setapikey.sh.
  Gemini cloud slots need GEMINI_API_KEY.
  Anthropic cloud slots need ANTHROPIC_API_KEY.
  Dry runs do not require keys.
EOF
}

die() {
  echo "agents-all-test.sh: $*" >&2
  exit 2
}

contains_agent() {
  local needle="$1"
  shift
  local value
  for value in "$@"; do
    [[ "$value" == "$needle" ]] && return 0
  done
  return 1
}

CLUE_MODES=(6)
agent_mode="all"
single_selector=""
dry_run=0
extra_args=(
  "--max-plies" "2"
  "--corrections" "1"
  "--game-timeout" "120"
  "--stack-timeout" "60"
  "--output-tokens" "256"
  "--pass-mode" "configured-ply-limit"
)
log_level=3
run_aichess=1
run_liveness=1

while (($#)); do
  case "$1" in
    --help|-\?|/\?)
      usage
      exit 0
      ;;
    --clue-mode)
      (($# >= 2)) || die "--clue-mode requires 0, 1, 2, 3, 4, 5, or 6"
      [[ "$2" =~ ^[0-6]$ ]] || die "--clue-mode requires 0, 1, 2, 3, 4, 5, or 6"
      CLUE_MODES=("$2")
      shift 2
      ;;
    --clue-ladder)
      CLUE_MODES=(6 5 4 3 2 1 0)
      shift
      ;;
    --legacy-clue-ladder)
      CLUE_MODES=(3 2 1 0)
      shift
      ;;
    --aichess-only)
      run_aichess=1
      run_liveness=0
      shift
      ;;
    --liveness-only)
      run_aichess=0
      run_liveness=1
      shift
      ;;
    --selector)
      (($# >= 2)) || die "--selector requires an agent slot, for example 09"
      single_selector="$2"
      agent_mode="single"
      shift 2
      ;;
    --cloud-only)
      agent_mode="cloud"
      shift
      ;;
    --openai-only)
      agent_mode="openai"
      shift
      ;;
    --gemini-only)
      agent_mode="gemini"
      shift
      ;;
    --anthropic-only)
      agent_mode="anthropic"
      shift
      ;;
    --local-only)
      agent_mode="local"
      shift
      ;;
    --chess-candidates-only)
      agent_mode="chess-candidates"
      shift
      ;;
    --capability-probe-only)
      agent_mode="capability-probe"
      shift
      ;;
    --dry-run)
      dry_run=1
      extra_args+=("--dry-run")
      shift
      ;;
    --complete-game)
      extra_args+=("--max-plies" "120" "--game-timeout" "900" "--stack-timeout" "90" "--corrections" "5")
      shift
      ;;
    --loglvl|--log-level)
      (($# >= 2)) || die "$1 requires a number from 0 through 5"
      [[ "$2" =~ ^[0-5]$ ]] || die "$1 requires a number from 0 through 5"
      log_level="$2"
      shift 2
      ;;
    --)
      shift
      extra_args+=("$@")
      break
      ;;
    *)
      die "unknown option: $1"
      ;;
  esac
done

if ((run_aichess)); then
  [[ -x "$BINARY" ]] || die "missing executable: $BINARY"
fi
if ((run_liveness)); then
  [[ -x "$LIVENESS_RUNNER" ]] || die "missing executable: $LIVENESS_RUNNER"
fi

case "$agent_mode" in
  single) AGENTS=("$single_selector") ;;
  cloud) AGENTS=("${CLOUD_AGENTS[@]}") ;;
  openai) AGENTS=("${OPENAI_CLOUD_AGENTS[@]}") ;;
  gemini) AGENTS=("${GEMINI_CLOUD_AGENTS[@]}") ;;
  anthropic) AGENTS=("${ANTHROPIC_CLOUD_AGENTS[@]}") ;;
  local) AGENTS=("${LOCAL_AGENTS[@]}") ;;
  chess-candidates) AGENTS=("${CHESS_CANDIDATE_AGENTS[@]}") ;;
  capability-probe) AGENTS=("${CAPABILITY_PROBE_AGENTS[@]}") ;;
  all) AGENTS=("${ALL_AGENTS[@]}") ;;
  *) die "internal agent mode error: $agent_mode" ;;
esac

needs_openai=0
needs_gemini=0
needs_anthropic=0
for agent in "${AGENTS[@]}"; do
  contains_agent "$agent" "${OPENAI_CLOUD_AGENTS[@]}" && needs_openai=1
  contains_agent "$agent" "${GEMINI_CLOUD_AGENTS[@]}" && needs_gemini=1
  contains_agent "$agent" "${ANTHROPIC_CLOUD_AGENTS[@]}" && needs_anthropic=1
done

if ((dry_run == 0)); then
  if ((needs_openai)) && [[ -z "${OPENAI_API_KEY:-}" ]]; then
    [[ -f "$SET_API_KEY" ]] || die "OPENAI_API_KEY is not set and $SET_API_KEY is missing"
    # shellcheck source=/dev/null
    source "$SET_API_KEY" >/dev/null
  fi

  if ((needs_openai)) && [[ -z "${OPENAI_API_KEY:-}" ]]; then
    die "OPENAI_API_KEY is not set; run: source ./setapikey.sh"
  fi
  if ((needs_gemini)) && [[ -z "${GEMINI_API_KEY:-}" ]]; then
    die "GEMINI_API_KEY is not set; run: export GEMINI_API_KEY=\"your_key_here\""
  fi
  if ((needs_anthropic)) && [[ -z "${ANTHROPIC_API_KEY:-}" ]]; then
    die "ANTHROPIC_API_KEY is not set; run: export ANTHROPIC_API_KEY=\"your_key_here\""
  fi
fi

stamp="$(date -u +%Y%m%d%H%M%S)"
mkdir -p "$ROOT_DIR/runs"
run_dir="$(mktemp -d "$ROOT_DIR/runs/agents_all_test_${stamp}_XXXXXX")"
summary="$run_dir/summary.tsv"
printf 'agent\tsmoke_test\tclue_mode\tstatus\tartifact\n' > "$summary"

echo "AIChess v2 all-agent test run: $run_dir"
echo "Agents: ${AGENTS[*]}"
echo "Clue modes: ${CLUE_MODES[*]}"
echo "Log level: $log_level"
echo "Summary: $summary"

agent_model() {
  case "$1" in
    01) printf 'openai:gpt-4.1-mini\n' ;;
    02) printf 'openai:gpt-4o-mini\n' ;;
    03) printf 'openai:gpt-5-mini\n' ;;
    04) printf 'openai:gpt-5-nano\n' ;;
    05) printf 'gemini:gemini-3.1-flash-lite\n' ;;
    06) printf 'anthropic:claude-opus-4\n' ;;
    07) printf 'anthropic:claude-sonnet-4\n' ;;
    08) printf 'anthropic:claude-3-7-sonnet\n' ;;
    09) printf 'anthropic:claude-3-5-haiku\n' ;;
    11) printf 'granite4:3b\n' ;;
    12) printf 'qwen2.5-coder:3b\n' ;;
    13) printf 'qwen2.5:0.5b\n' ;;
    14) printf 'qwen2.5:latest\n' ;;
    15) printf 'qwen:4b\n' ;;
    16) printf 'robit/qwen3.5-9b-r7-research:q4km\n' ;;
    17) printf 'smollm2:135m\n' ;;
    18) printf 'gemma3:270m\n' ;;
    19) printf 'llama3.2:1b\n' ;;
    1A) printf 'gemma3:1b\n' ;;
    1B) printf 'tinyllama:latest\n' ;;
    1C) printf 'phi3:mini\n' ;;
    1D) printf 'phi4-mini:latest\n' ;;
    1E) printf 'mistral:latest\n' ;;
    1F) printf 'llama3.2:3b\n' ;;
    10) printf 'gemma3:4b\n' ;;
    *) return 1 ;;
  esac
}

liveness_args_for_agent() {
  local agent="$1"
  local model
  model="$(agent_model "$agent")" || return 1
  case "$agent" in
    01|02|03|04)
      printf '%s\n' --adapter openai_responses --provider-id openai --model "${model#openai:}" --api-key-env OPENAI_API_KEY
      ;;
    05)
      printf '%s\n' --adapter gemini_generate_content --provider-id google --model "${model#gemini:}" --api-key-env GEMINI_API_KEY
      ;;
    06|07|08|09)
      printf '%s\n' --adapter anthropic_messages --provider-id anthropic --model "${model#anthropic:}" --api-key-env ANTHROPIC_API_KEY
      ;;
    *)
      printf '%s\n' --adapter ollama --provider-id ollama --model "$model"
      ;;
  esac
}

overall_status=0
for agent in "${AGENTS[@]}"; do
  if ((run_aichess)); then
    for clue_mode in "${CLUE_MODES[@]}"; do
      echo "===== aichess ${agent},${clue_mode} ====="
      if output="$("$BINARY" "${agent},${clue_mode}" --loglvl "$log_level" "${extra_args[@]}")"; then
        printf '%s\n' "$output"
        artifact="$(printf '%s\n' "$output" | awk '/^Run logs: / {print substr($0, 11)}' | tail -n 1)"
        printf '%s\taichess\t%s\tcommand_ok\t%s\n' "$agent" "$clue_mode" "$artifact" >> "$summary"
      else
        status=$?
        printf '%s\n' "$output"
        artifact="$(printf '%s\n' "$output" | awk '/^Run logs: / {print substr($0, 11)}' | tail -n 1)"
        printf '%s\taichess\t%s\tcommand_failed_%s\t%s\n' "$agent" "$clue_mode" "$status" "$artifact" >> "$summary"
        overall_status=1
      fi
    done
  fi
  if ((run_liveness)); then
    echo "===== preschool liveness ${agent} ====="
    mapfile -t liveness_args < <(liveness_args_for_agent "$agent")
    if ((dry_run)); then
      printf '%q ' "$LIVENESS_RUNNER" "${liveness_args[@]}" --timeout 60 --output-tokens 32 --loglvl 1
      printf '\n'
      printf '%s\tpreschool_liveness\t-\tcommand_dry_run\t-\n' "$agent" >> "$summary"
    elif output="$("$LIVENESS_RUNNER" "${liveness_args[@]}" --timeout 60 --output-tokens 32 --loglvl 1)"; then
      printf '%s\n' "$output"
      artifact="$(printf '%s\n' "$output" | tail -n 1)"
      fail_count="$(awk -F'"' '/"pass_fail":"fail"/ {n++} END{print n+0}' "$artifact" 2>/dev/null || printf '0')"
      if ((fail_count > 0)); then
        printf '%s\tpreschool_liveness\t-\tcommand_failed_probe\t%s\n' "$agent" "$artifact" >> "$summary"
        overall_status=1
      else
        printf '%s\tpreschool_liveness\t-\tcommand_ok\t%s\n' "$agent" "$artifact" >> "$summary"
      fi
    else
      status=$?
      printf '%s\n' "$output"
      artifact="$(printf '%s\n' "$output" | tail -n 1)"
      printf '%s\tpreschool_liveness\t-\tcommand_failed_%s\t%s\n' "$agent" "$status" "$artifact" >> "$summary"
      overall_status=1
    fi
  fi
done

echo "All-agent run complete: $summary"
exit "$overall_status"
