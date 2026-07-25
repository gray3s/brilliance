#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY="$ROOT_DIR/bin/test_cloud_agents_binary"
SET_API_KEY="$ROOT_DIR/setapikey.sh"

ALL_AGENTS=(01 02 03 04 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E)
CLOUD_AGENTS=(01 02 03 04)
LOCAL_AGENTS=(11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E)
CHESS_CANDIDATE_AGENTS=(11 12 14 15 19 1C 1D 1E)
CAPABILITY_PROBE_AGENTS=(13 16 17 18 1A 1B)

usage() {
  cat <<'EOF'
Usage:
  ./agents-all-test.sh
  ./agents-all-test.sh --clue-mode 3
  ./agents-all-test.sh --cloud-only
  ./agents-all-test.sh --local-only
  ./agents-all-test.sh --chess-candidates-only
  ./agents-all-test.sh --capability-probe-only
  ./agents-all-test.sh --complete-game --local-only
  ./agents-all-test.sh --dry-run
  ./agents-all-test.sh -- --max-plies 20 --game-timeout 300

Runs the AIChess one-board self-play binary test for all selected agents and
all clue modes. AIChess is treated as the practical AIH v1 game prototype.
For the broader AIH v2 Class1/Class2/Class3 database catalog, use:

  ../../v2/run_aih_v2_eval.sh --catalog-only --include-cloud-skips

Defaults:
  agents:    01 02 03 04 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E
  clues:     0 1 2 3
  log level: 3, including child run transcript output

Agent keys:
  01 openai:gpt-4.1-mini
  02 openai:gpt-4o-mini
  03 openai:gpt-5-mini
  04 openai:gpt-5-nano
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

Run controls:
  --clue-mode N narrows the run to one clue mode.
  --chess-candidates-only runs downloaded local agents that passed the
    non-Qt one-move UCI legality screen.
  --capability-probe-only runs downloaded local agents that failed the
    one-move UCI legality screen and should not be used for 1v1 chess yet.
  --complete-game raises the binary limits for a complete-game attempt:
    --max-plies 120 --game-timeout 900 --stack-timeout 90 --corrections 5
  Arguments after -- are passed through to ./bin/test_cloud_agents_binary.

Key handling:
  If cloud agents are selected and OPENAI_API_KEY is not already set, this
  script sources ./setapikey.sh. It does not print the key and does not prompt
  for the key.
EOF
}

die() {
  echo "agents-all-test.sh: $*" >&2
  exit 2
}

CLUE_MODES=(0 1 2 3)
agent_mode="all"
dry_run=0
extra_args=()
log_level=3

while (($#)); do
  case "$1" in
    --help|-\?|/\?)
      usage
      exit 0
      ;;
    --clue-mode)
      (($# >= 2)) || die "--clue-mode requires 0, 1, 2, or 3"
      [[ "$2" =~ ^[0-3]$ ]] || die "--clue-mode requires 0, 1, 2, or 3"
      CLUE_MODES=("$2")
      shift 2
      ;;
    --cloud-only)
      agent_mode="cloud"
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

[[ -x "$BINARY" ]] || die "missing executable: $BINARY"

case "$agent_mode" in
  cloud) AGENTS=("${CLOUD_AGENTS[@]}") ;;
  local) AGENTS=("${LOCAL_AGENTS[@]}") ;;
  chess-candidates) AGENTS=("${CHESS_CANDIDATE_AGENTS[@]}") ;;
  capability-probe) AGENTS=("${CAPABILITY_PROBE_AGENTS[@]}") ;;
  all) AGENTS=("${ALL_AGENTS[@]}") ;;
  *) die "internal agent mode error: $agent_mode" ;;
esac

needs_cloud=0
for agent in "${AGENTS[@]}"; do
  [[ "$agent" == 0* ]] && needs_cloud=1
done

if ((needs_cloud)) && [[ -z "${OPENAI_API_KEY:-}" ]]; then
  [[ -f "$SET_API_KEY" ]] || die "OPENAI_API_KEY is not set and $SET_API_KEY is missing"
  # shellcheck source=/dev/null
  source "$SET_API_KEY" >/dev/null
fi

if ((needs_cloud)) && [[ -z "${OPENAI_API_KEY:-}" ]]; then
  die "OPENAI_API_KEY is not set; run: source ./setapikey.sh"
fi

stamp="$(date -u +%Y%m%d%H%M%S)"
run_dir="$ROOT_DIR/runs/agents_all_test_$stamp"
mkdir -p "$run_dir"
summary="$run_dir/summary.tsv"
printf 'agent\tclue_mode\tstatus\n' > "$summary"

echo "AIChess v2 all-agent test run: $run_dir"
echo "Agents: ${AGENTS[*]}"
echo "Clue modes: ${CLUE_MODES[*]}"
echo "Log level: $log_level"
echo "Summary: $summary"

overall_status=0
for agent in "${AGENTS[@]}"; do
  for clue_mode in "${CLUE_MODES[@]}"; do
    echo "===== running ${agent},${clue_mode} ====="
    if "$BINARY" "${agent},${clue_mode}" --loglvl "$log_level" "${extra_args[@]}"; then
      printf '%s\t%s\tcommand_ok\n' "$agent" "$clue_mode" >> "$summary"
    else
      status=$?
      printf '%s\t%s\tcommand_failed_%s\n' "$agent" "$clue_mode" "$status" >> "$summary"
      overall_status=1
    fi
  done
done

echo "All-agent run complete: $summary"
exit "$overall_status"
