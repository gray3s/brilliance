#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
TIMEOUT_FINDER="$ROOT_DIR/ollama_agentic_registration_timeout_finder.sh"

VERIFIED_CSV="$ROOT_DIR/AIH_V5_LOCAL_AGENT_REGISTRATION_VERIFIED.csv"
FAILURES_CSV="$ROOT_DIR/AIH_V5_LOCAL_AGENT_REGISTRATION_FAILURES.csv"
COMM_SETTINGS_CSV="$ROOT_DIR/AIH_V5_LOCAL_AGENT_REGISTRATION_COMM_SETTINGS.csv"
SUMMARY_CSV="$ROOT_DIR/AIH_V5_LOCAL_AGENT_REGISTRATION_SUMMARY.csv"
README_MD="$ROOT_DIR/AIH_V5_LOCAL_AGENT_REGISTRATION_README.md"

usage() {
  cat <<EOF
Usage:
  $0 --publish-run RUN_DIR
  $0 --latest
  $0 --refresh [MODEL ...]
  $0 --verify

Commands:
  --publish-run RUN_DIR  Publish an existing timeout-finder run as the stable
                         AIH v5 local-agent registration database.
  --latest               Publish the newest standalone_registration_timeouts run.
  --refresh [MODEL ...]  Run timeout finder, then publish that run.
  --verify               Validate the stable registration database files.

Stable outputs:
  $VERIFIED_CSV
  $FAILURES_CSV
  $COMM_SETTINGS_CSV
  $SUMMARY_CSV
  $README_MD
EOF
}

latest_run_dir() {
  ls -td "$ROOT_DIR"/standalone_registration_timeouts/* 2>/dev/null | sed -n '1p'
}

require_run_dir() {
  local run_dir="$1"
  if [[ -z "$run_dir" || ! -d "$run_dir" ]]; then
    echo "missing run directory: $run_dir" >&2
    exit 2
  fi
  if [[ ! -r "$run_dir/per_agent_timeouts.csv" ]]; then
    echo "missing run summary: $run_dir/per_agent_timeouts.csv" >&2
    exit 2
  fi
}

publish_run() {
  local run_dir="$1"
  local source_summary="$run_dir/per_agent_timeouts.csv"
  local source_comm="$run_dir/agent_communication_settings.csv"
  local generated_at
  require_run_dir "$run_dir"
  generated_at="$(date '+%Y-%m-%d %H:%M:%S %Z')"

  cp -a "$source_summary" "$SUMMARY_CSV"
  if [[ -r "$source_comm" ]]; then
    cp -a "$source_comm" "$COMM_SETTINGS_CSV"
  else
    awk -F, '
      NR == 1 {
        print "model,provider,api_surface,registration_timeout_seconds,registration_num_predict,num_thread,keep_alive,temperature,official_parse_policy,diagnostic_parse_policy,last_elapsed_seconds,last_status,last_reason,response_json"
        next
      }
      $2 == "pass" {
        printf "%s,ollama,generate,%s,%s,1,0s,0,visible_response_full_legal_uci,thinking_diagnostic_only,%s,%s,%s,%s\n",
          $1, $7, $6, $4, $2, $3, $13
      }
    ' "$source_summary" > "$COMM_SETTINGS_CSV"
  fi

  awk -F, '
    NR == 1 {
      print "model,provider,api_surface,registration_timeout_seconds,registration_num_predict,last_elapsed_seconds,last_reason,move_or_thinking_move,response_json"
      next
    }
    $2 == "pass" {
      printf "%s,ollama,generate,%s,%s,%s,%s,%s,%s\n", $1, $7, $6, $4, $3, $12, $13
    }
  ' "$source_summary" > "$VERIFIED_CSV"

  awk -F, '
    NR == 1 {
      print "model,status,reason,elapsed_seconds,global_timeout_seconds,num_predict,response_len,thinking_len,done_reason,eval_count,move_or_thinking_move,response_json,note"
      next
    }
    $2 != "pass" {
      printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", $1, $2, $3, $4, $5, $6, $8, $9, $10, $11, $12, $13, $15
    }
  ' "$source_summary" > "$FAILURES_CSV"

  local pass_count fail_count
  pass_count="$(awk -F, 'NR > 1 && $2 == "pass" { n++ } END { print n + 0 }' "$source_summary")"
  fail_count="$(awk -F, 'NR > 1 && $2 != "pass" { n++ } END { print n + 0 }' "$source_summary")"

  cat > "$README_MD" <<EOF
# AIH v5 Local Agent Registration Database

Generated: $generated_at

Source run:

\`\`\`text
$run_dir
\`\`\`

Stable files:

\`\`\`text
$VERIFIED_CSV
$FAILURES_CSV
$COMM_SETTINGS_CSV
$SUMMARY_CSV
\`\`\`

Current status:

\`\`\`text
verified_pass=$pass_count
registration_fail=$fail_count
\`\`\`

Use the communication settings in AIH v5 registration:

\`\`\`bash
AIH_V5_REGISTRATION_COMM_SETTINGS_CSV=$COMM_SETTINGS_CSV \\
AIH_V5_REGISTRATION_MODE=game \\
AIH_V5_REGISTRATION_DYNAMIC_TIMEOUT=0 \\
./aih_v5.sh --registration-only --no-open --registration-forward
\`\`\`

Policy:

- Local agent registration is a separate AIH v5 phase.
- Verified agents are listed in the verified CSV.
- Communication settings are stored separately and can be consumed by AIH v5 registration.
- Full raw response JSON paths are preserved for audit.
- response remains the official agent answer; thinking is diagnostic unless a policy explicitly says otherwise.
EOF

  echo "published_run=$run_dir"
  echo "verified_csv=$VERIFIED_CSV"
  echo "failures_csv=$FAILURES_CSV"
  echo "comm_settings_csv=$COMM_SETTINGS_CSV"
  echo "summary_csv=$SUMMARY_CSV"
  echo "readme=$README_MD"
  echo "verified_pass=$pass_count"
  echo "registration_fail=$fail_count"
}

verify_database() {
  local status=0
  for path in "$VERIFIED_CSV" "$FAILURES_CSV" "$COMM_SETTINGS_CSV" "$SUMMARY_CSV" "$README_MD"; do
    if [[ ! -s "$path" ]]; then
      echo "missing_or_empty=$path"
      status=1
    else
      echo "ok=$path"
    fi
  done
  if [[ -s "$VERIFIED_CSV" ]]; then
    echo "verified_count=$(awk -F, 'NR > 1 { n++ } END { print n + 0 }' "$VERIFIED_CSV")"
  fi
  if [[ -s "$FAILURES_CSV" ]]; then
    echo "failure_count=$(awk -F, 'NR > 1 { n++ } END { print n + 0 }' "$FAILURES_CSV")"
  fi
  return "$status"
}

refresh_registration() {
  [[ -x "$TIMEOUT_FINDER" ]] || { echo "missing executable: $TIMEOUT_FINDER" >&2; exit 2; }
  local output run_dir
  output="$("$TIMEOUT_FINDER" "$@")"
  printf '%s\n' "$output"
  run_dir="$(awk -F= '$1 == "run_dir" { print $2 }' <<< "$output" | tail -n 1)"
  publish_run "$run_dir"
}

main() {
  case "${1:-}" in
    --help|-h|"")
      usage
      ;;
    --publish-run)
      shift
      publish_run "${1:-}"
      ;;
    --latest)
      publish_run "$(latest_run_dir)"
      ;;
    --refresh)
      shift
      refresh_registration "$@"
      ;;
    --verify)
      verify_database
      ;;
    *)
      echo "unknown command: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
}

main "$@"
