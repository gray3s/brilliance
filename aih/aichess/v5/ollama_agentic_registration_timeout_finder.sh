#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
SMOKE_SCRIPT="$ROOT_DIR/ollama_agentic_registration_smoke.sh"
OUT_DIR="${AIH_V5_TIMEOUT_FINDER_OUT_DIR:-$ROOT_DIR/standalone_registration_timeouts}"
STAMP="$(date '+%Y%m%d_%H%M%S')"
RUN_DIR="$OUT_DIR/$STAMP"
SUMMARY_CSV="$RUN_DIR/per_agent_timeouts.csv"
COMM_SETTINGS_CSV="$RUN_DIR/agent_communication_settings.csv"
GLOBAL_TIMEOUT_SECONDS="${AIH_V5_TIMEOUT_FINDER_GLOBAL_TIMEOUT_SECONDS:-360}"
NUM_PREDICT="${AIH_V5_TIMEOUT_FINDER_NUM_PREDICT:-256}"
BUFFER_SECONDS="${AIH_V5_TIMEOUT_FINDER_BUFFER_SECONDS:-30}"
BUFFER_PERCENT="${AIH_V5_TIMEOUT_FINDER_BUFFER_PERCENT:-25}"

usage() {
  cat <<EOF
Usage:
  $0 [MODEL ...]

Runs one standalone AIH-v5-style Ollama registration smoke per model using a
single global maximum timeout. Records the elapsed time and a conservative
per-agent timeout recommendation for models that pass.

Environment:
  AIH_V5_TIMEOUT_FINDER_GLOBAL_TIMEOUT_SECONDS  default $GLOBAL_TIMEOUT_SECONDS
  AIH_V5_TIMEOUT_FINDER_NUM_PREDICT             default $NUM_PREDICT
  AIH_V5_TIMEOUT_FINDER_BUFFER_SECONDS          default $BUFFER_SECONDS
  AIH_V5_TIMEOUT_FINDER_BUFFER_PERCENT          default $BUFFER_PERCENT
  AIH_V5_TIMEOUT_FINDER_OUT_DIR                 default $OUT_DIR
EOF
}

csv_escape() {
  local value="${1//$'\r'/ }"
  value="${value//$'\n'/ }"
  value="${value//\"/\"\"}"
  printf '"%s"' "$value"
}

models_from_ollama() {
  ollama list | awk 'NR > 1 && $1 != "" { print $1 }'
}

recommended_timeout() {
  local elapsed="$1"
  local pct_timeout fixed_timeout rec
  pct_timeout=$(((elapsed * (100 + BUFFER_PERCENT) + 99) / 100))
  fixed_timeout=$((elapsed + BUFFER_SECONDS))
  if ((pct_timeout > fixed_timeout)); then
    rec="$pct_timeout"
  else
    rec="$fixed_timeout"
  fi
  if ((rec > GLOBAL_TIMEOUT_SECONDS)); then
    rec="$GLOBAL_TIMEOUT_SECONDS"
  fi
  if ((rec < 10)); then
    rec=10
  fi
  printf '%s\n' "$rec"
}

run_one() {
  local model="$1"
  local smoke_output smoke_status smoke_summary smoke_run_dir row status reason elapsed timeout num_predict response_len thinking_len done_reason eval_count move response_json note rec

  set +e
  smoke_output="$(
    AIH_V5_STANDALONE_REG_NUM_PREDICT="$NUM_PREDICT" \
    AIH_V5_STANDALONE_REG_TIMEOUT_SECONDS="$GLOBAL_TIMEOUT_SECONDS" \
    AIH_V5_STANDALONE_REG_OUT_DIR="$RUN_DIR/smoke_runs" \
    "$SMOKE_SCRIPT" "$model" 2>&1
  )"
  smoke_status=$?
  set -e

  smoke_summary="$(awk -F= '$1 == "summary_csv" { print $2 }' <<< "$smoke_output" | tail -n 1)"
  smoke_run_dir="$(awk -F= '$1 == "run_dir" { print $2 }' <<< "$smoke_output" | tail -n 1)"

  if [[ -z "$smoke_summary" || ! -f "$smoke_summary" ]]; then
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
      "$(csv_escape "$model")" fail smoke_script_no_summary "" "$GLOBAL_TIMEOUT_SECONDS" "$NUM_PREDICT" "" "" "" "" "" "" "" "" "" "$(csv_escape "$smoke_output")" \
      >> "$SUMMARY_CSV"
    return 1
  fi

  row="$(awk 'NR == 2 { print }' "$smoke_summary")"
  IFS=, read -r model_csv status reason elapsed timeout num_predict response_len thinking_len done_reason eval_count move response_json note <<< "$row"

  if [[ "$status" == "pass" ]]; then
    rec="$(recommended_timeout "$elapsed")"
  else
    rec=""
  fi

  printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
    "$model_csv" "$status" "$reason" "$elapsed" "$timeout" "$num_predict" "$rec" \
    "$response_len" "$thinking_len" "$done_reason" "$eval_count" "$move" "$response_json" \
    "$(csv_escape "$smoke_run_dir")" "$note" "$(csv_escape "$smoke_output")" \
    >> "$SUMMARY_CSV"

  [[ "$status" == "pass" ]]
}

main() {
  if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    usage
    exit 0
  fi
  [[ -x "$SMOKE_SCRIPT" ]] || { echo "missing executable: $SMOKE_SCRIPT" >&2; exit 2; }
  command -v ollama >/dev/null 2>&1 || { echo "ollama command not found" >&2; exit 127; }
  command -v awk >/dev/null 2>&1 || { echo "awk command not found" >&2; exit 127; }

  mkdir -p "$RUN_DIR"
  printf 'model,status,reason,elapsed_seconds,global_timeout_seconds,num_predict,recommended_timeout_seconds,response_len,thinking_len,done_reason,eval_count,move_or_thinking_move,response_json,smoke_run_dir,note,smoke_output\n' > "$SUMMARY_CSV"
  printf 'model,provider,api_surface,registration_timeout_seconds,registration_num_predict,num_thread,keep_alive,temperature,official_parse_policy,diagnostic_parse_policy,last_elapsed_seconds,last_status,last_reason,response_json\n' > "$COMM_SETTINGS_CSV"

  local models=("$@")
  if ((${#models[@]} == 0)); then
    mapfile -t models < <(models_from_ollama)
  fi
  if ((${#models[@]} == 0)); then
    echo "no Ollama models found" >&2
    exit 2
  fi

  local passed=0 failed=0 model
  for model in "${models[@]}"; do
    printf 'timeout-finder testing %s max_timeout=%ss num_predict=%s\n' "$model" "$GLOBAL_TIMEOUT_SECONDS" "$NUM_PREDICT" >&2
    if run_one "$model"; then
      passed=$((passed + 1))
    else
      failed=$((failed + 1))
    fi
  done

  awk -F, 'NR == 1 || $2 != "pass" { print }' "$SUMMARY_CSV" > "$RUN_DIR/failures.csv"
  awk -F, 'NR == 1 || $2 == "pass" { print }' "$SUMMARY_CSV" > "$RUN_DIR/passes.csv"
  awk -F, '
    NR == 1 { next }
    $2 == "pass" {
      printf "%s,ollama,generate,%s,%s,1,0s,0,visible_response_full_legal_uci,thinking_diagnostic_only,%s,%s,%s,%s\n",
        $1, $7, $6, $4, $2, $3, $13
    }
  ' "$SUMMARY_CSV" >> "$COMM_SETTINGS_CSV"

  echo "summary_csv=$SUMMARY_CSV"
  echo "comm_settings_csv=$COMM_SETTINGS_CSV"
  echo "passes_csv=$RUN_DIR/passes.csv"
  echo "failures_csv=$RUN_DIR/failures.csv"
  echo "run_dir=$RUN_DIR"
  echo "passed=$passed failed=$failed"
}

main "$@"
