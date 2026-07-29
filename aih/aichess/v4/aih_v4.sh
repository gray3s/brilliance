#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE="$ROOT_DIR/qwen_ollama_chess_qt/qwen_ollama_chess_qt"
LOCAL_AGENT_REGISTRY="${AIH_V4_LOCAL_AGENT_REGISTRY:-$ROOT_DIR/../v3/qualification_cache/local_qualification_20260729032018.csv}"
LOCAL_SMOKE=1
CLOUD_SMOKE_PROVIDER=""
SMOKE_STAGE="${AIH_V4_SMOKE_STAGE:-local-retry}"
PASSTHRU_ARGS=()
ENABLE_BOARD_AWARENESS="${AIH_V4_BOARD_AWARENESS_PROBE:-0}"
ENABLE_REASONING_MATRIX="${AIH_V4_ENABLE_REASONING_MATRIX:-0}"
REASONING_RANGE="${AIH_V4_ALLOWED_REASONINGS:-${AIH_V4_REASONING_RANGE:-}}"
VERBOSITY_RANGE="${AIH_V4_ALLOWED_VERBOSITY:-${AIH_V4_TEXT_VERBOSITY_RANGE:-}}"
EXPLICIT_VERBOSITY_RANGE=0
if [[ -n "${AIH_V4_ALLOWED_VERBOSITY:-}" || -n "${AIH_V4_TEXT_VERBOSITY_RANGE:-}" ]]; then
  EXPLICIT_VERBOSITY_RANGE=1
fi
AIH_V4_LOCAL_CLOUD_MAXPLY_RATIO="${AIH_V4_LOCAL_CLOUD_MAXPLY_RATIO:-4}"
AIH_V4_LOCAL_MAXPLY_CAP="${AIH_V4_LOCAL_MAXPLY_CAP:-40}"
AIH_V4_CLOUD_MAXPLY_CAP="${AIH_V4_CLOUD_MAXPLY_CAP:-10}"

has_env() {
  local name="$1"
  [[ -n "${!name:-}" ]]
}

prepare_provider_key() {
  local provider="$1"
  case "$provider" in
    openai)
      if ! has_env OPENAI_API_KEY; then
        echo "aih_v4: OPENAI_API_KEY is not exported; openai cloud smoke cannot test authorization." >&2
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
        echo "aih_v4: GEMINI_API_KEY is not exported; google/gemini cloud smoke cannot test authorization." >&2
        echo "aih_v4: GOOGLE_API_KEY or GOOGLE_GENAI_API_KEY may be used as a fallback source for GEMINI_API_KEY." >&2
        exit 2
      fi
      ;;
    anthropic)
      if ! has_env ANTHROPIC_API_KEY; then
        echo "aih_v4: ANTHROPIC_API_KEY is not exported; anthropic cloud smoke cannot test authorization." >&2
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
    --local-smoke|--no-cloud)
      LOCAL_SMOKE=1
      CLOUD_SMOKE_PROVIDER=""
      ;;
    --local-progress-smoke)
      LOCAL_SMOKE=1
      CLOUD_SMOKE_PROVIDER=""
      SMOKE_STAGE="local-progress"
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
      AIH_V4_LOCAL_MAXPLYS="${arg#*=}"
      ;;
    --local-cloud-maxply-ratio=*)
      AIH_V4_LOCAL_CLOUD_MAXPLY_RATIO="${arg#*=}"
      ;;
    *)
      PASSTHRU_ARGS+=("$arg")
      ;;
  esac
done

latest_summary_path() {
  find "$ROOT_DIR/runs/aih_v4_pairwise_prototype_20260729" -maxdepth 1 -type f -name '*_summary.md' \
    -printf '%T@ %p\n' 2>/dev/null | sort -nr | awk 'NR == 1 { $1 = ""; sub(/^ /, ""); print }'
}

publish_latest_summary() {
  local latest_summary latest_row model mode termination completed plies legal failed rejected elapsed
  local publish_local_maxply publish_cloud_maxply
  local published_dir published_summary published_jsonl
  latest_summary="$(latest_summary_path)"
  if [[ -z "$latest_summary" || ! -r "$latest_summary" ]]; then
    echo "aih_v4: no summary file available to publish." >&2
    return 2
  fi

  latest_row="$(awk -F'|' '/^\| / && $2 !~ /Model/ && $2 !~ /---/ { print; exit }' "$latest_summary")"
  if [[ -z "$latest_row" ]]; then
    echo "aih_v4: summary has no result row: $latest_summary" >&2
    return 2
  fi

  model="$(awk -F'|' '{ gsub(/^ +| +$/, "", $2); print $2 }' <<<"$latest_row")"
  mode="$(awk -F'|' '{ gsub(/^ +| +$/, "", $3); print $3 }' <<<"$latest_row")"
  termination="$(awk -F'|' '{ gsub(/^ +| +$/, "", $4); print $4 }' <<<"$latest_row")"
  completed="$(awk -F'|' '{ gsub(/^ +| +$/, "", $5); print $5 }' <<<"$latest_row")"
  plies="$(awk -F'|' '{ gsub(/^ +| +$/, "", $6); print $6 }' <<<"$latest_row")"
  legal="$(awk -F'|' '{ gsub(/^ +| +$/, "", $7); print $7 }' <<<"$latest_row")"
  failed="$(awk -F'|' '{ gsub(/^ +| +$/, "", $8); print $8 }' <<<"$latest_row")"
  rejected="$(awk -F'|' '{ gsub(/^ +| +$/, "", $9); print $9 }' <<<"$latest_row")"
  elapsed="$(awk -F'|' '{ gsub(/^ +| +$/, "", $10); print $10 }' <<<"$latest_row")"
  publish_local_maxply="${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-$AIH_V4_LOCAL_MAXPLY_CAP}}"
  if ((publish_local_maxply > AIH_V4_LOCAL_MAXPLY_CAP)); then
    publish_local_maxply="$AIH_V4_LOCAL_MAXPLY_CAP"
  fi
  publish_cloud_maxply="$(derived_cloud_maxply "$publish_local_maxply" "$AIH_V4_LOCAL_CLOUD_MAXPLY_RATIO")"
  published_dir="$ROOT_DIR/published_results"
  mkdir -p "$published_dir"
  published_summary="$published_dir/$(basename "$latest_summary")"
  published_jsonl="$published_dir/$(basename "${latest_summary%_summary.md}.jsonl")"
  cp "$latest_summary" "$published_summary"
  if [[ -r "${latest_summary%_summary.md}.jsonl" ]]; then
    cp "${latest_summary%_summary.md}.jsonl" "$published_jsonl"
  fi

  {
    echo "# AIH v4 preliminary results - 2026-07-29"
    echo
    echo "These are preliminary release-mode data points from the current v4 prototype."
    echo "They are not final AIH rankings. The local default maxply has been raised"
    echo "and the local/cloud maxply multiplier range is 2x to 4x."
    echo
    echo "Rendered HTML results:"
    echo "https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v4/AIH_V4_PRELIMINARY_RESULTS_20260729.html"
    echo
    echo "## Current default run controls"
    echo
    echo "- Local retry/expand/full-local default maxply: $publish_local_maxply"
    echo "- Cloud provider-key default maxply: $publish_cloud_maxply, derived from local maxply / ratio"
    echo "- Local maxply cap: $AIH_V4_LOCAL_MAXPLY_CAP"
    echo "- Cloud maxply cap: $AIH_V4_CLOUD_MAXPLY_CAP"
    echo "- Default local/cloud maxply multiplier: 4x"
    echo "- Allowed local/cloud maxply multiplier range: 2x to 4x"
    echo "- CLI controls: \`--local-maxplys=N\`, \`--local-cloud-maxply-ratio=N\`"
    echo
    echo "## Latest binary-published summary"
    echo
    echo "Source summary: \`${latest_summary#$ROOT_DIR/}\`"
    echo
    echo "| Model | Mode | Termination | Completed game | Plies | Legal moves | Failed turns | Rejected attempts | Elapsed s |"
    echo "| --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: |"
    echo "| $model | $mode | $termination | $completed | $plies | $legal | $failed | $rejected | $elapsed |"
    echo
    echo "## Preliminary interpretation"
    echo
    echo "The latest preliminary row is generated and pushed by successful"
    echo "\`bin/aih_v4\` runs from the newest v4 run summary."
  } > "$ROOT_DIR/AIH_V4_PRELIMINARY_RESULTS_20260729.md"

  {
    echo '<!doctype html>'
    echo '<html lang="en">'
    echo '<head>'
    echo '  <meta charset="utf-8">'
    echo '  <title>AIH v4 preliminary AIChess results - 2026-07-29</title>'
    echo '  <style>body{font-family:Arial,sans-serif;margin:2rem;line-height:1.45;color:#1f2933}table{border-collapse:collapse;width:100%;margin-top:1rem}th,td{border:1px solid #cad2dc;padding:.55rem;text-align:left;vertical-align:top}th{background:#eef2f6}.note{color:#52606d}code{background:#eef2f6;padding:.1rem .25rem;border-radius:3px}</style>'
    echo '</head>'
    echo '<body>'
    echo '  <main>'
    echo '    <h1>AIH v4 preliminary AIChess results - 2026-07-29</h1>'
    echo '    <p>These are preliminary release-mode data points from the current AIH v4 prototype. They are not final AIH rankings.</p>'
    echo '    <p>The local default maxply has been raised and the local/cloud maxply multiplier range is 2x to 4x.</p>'
    echo '    <h2>Current default run controls</h2>'
    echo '    <ul>'
    echo "      <li>Local retry/expand/full-local default maxply: $publish_local_maxply</li>"
    echo "      <li>Cloud provider-key default maxply: $publish_cloud_maxply, derived from local maxply / ratio</li>"
    echo "      <li>Local maxply cap: $AIH_V4_LOCAL_MAXPLY_CAP</li>"
    echo "      <li>Cloud maxply cap: $AIH_V4_CLOUD_MAXPLY_CAP</li>"
    echo '      <li>Default local/cloud maxply multiplier: 4x</li>'
    echo '      <li>Allowed local/cloud maxply multiplier range: 2x to 4x</li>'
    echo '      <li>CLI controls: <code>--local-maxplys=N</code>, <code>--local-cloud-maxply-ratio=N</code></li>'
    echo '    </ul>'
    echo '    <h2>Latest binary-published summary</h2>'
    echo "    <p class=\"note\">Source summary: <code>${latest_summary#$ROOT_DIR/}</code></p>"
    echo '    <table>'
    echo '      <thead><tr><th>Model</th><th>Mode</th><th>Termination</th><th>Completed game</th><th>Plies</th><th>Legal moves</th><th>Failed turns</th><th>Rejected attempts</th><th>Elapsed s</th></tr></thead>'
    echo "      <tbody><tr><td>$model</td><td>$mode</td><td>$termination</td><td>$completed</td><td>$plies</td><td>$legal</td><td>$failed</td><td>$rejected</td><td>$elapsed</td></tr></tbody>"
    echo '    </table>'
    echo '    <h2>Preliminary interpretation</h2>'
    echo '    <p>The latest preliminary row is generated and pushed by successful <code>bin/aih_v4</code> runs from the newest v4 run summary.</p>'
    echo '  </main>'
    echo '</body>'
    echo '</html>'
  } > "$ROOT_DIR/AIH_V4_PRELIMINARY_RESULTS_20260729.html"

  local readme="$ROOT_DIR/../README.md"
  if ! grep -qi 'AIH v4 preliminary results' "$readme"; then
    {
      echo
      echo "## AIH v4 Preliminary Results"
      echo
      echo "- Published summary folder: \`v4/published_results/\`"
      echo "- Summary: \`v4/AIH_V4_PRELIMINARY_RESULTS_20260729.md\`"
      echo "- Rendered HTML: https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v4/AIH_V4_PRELIMINARY_RESULTS_20260729.html"
      echo "- Binary publish path: successful \`v4/bin/aih_v4\` runs"
    } >> "$readme"
  fi

  echo "aih_v4: preliminary results regenerated from $latest_summary" >&2

  git -C "$ROOT_DIR" add \
    AIH_V4_PROJECT_GOALS_20260729.md \
    AIH_V4_PRELIMINARY_RESULTS_20260729.md \
    AIH_V4_PRELIMINARY_RESULTS_20260729.html \
    published_results \
    aih_v4.sh \
    ../README.md
  git -C "$ROOT_DIR" commit -m "Update AIH v4 summary data"
  git -C "$ROOT_DIR" push origin HEAD
}

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
    echo "aih_v4: $label must be a positive integer: $value" >&2
    exit 2
  fi
}

derived_cloud_maxply() {
  local local_maxply="$1"
  local ratio="$2"
  positive_int_or_die "local maxply" "$local_maxply"
  positive_int_or_die "local/cloud maxply ratio" "$ratio"
  if ((local_maxply > AIH_V4_LOCAL_MAXPLY_CAP)); then
    local_maxply="$AIH_V4_LOCAL_MAXPLY_CAP"
  fi
  if ((ratio < 2)); then
    ratio=2
  fi
  if ((ratio > 4)); then
    ratio=4
  fi
  local derived="$(((local_maxply + ratio - 1) / ratio))"
  if ((derived > AIH_V4_CLOUD_MAXPLY_CAP)); then
    derived="$AIH_V4_CLOUD_MAXPLY_CAP"
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
    echo "aih_v4: invalid reasoning range value(s): $invalid" >&2
    echo "aih_v4: expected low, medium, high, xhigh" >&2
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
    echo "aih_v4: invalid verbosity range value(s): $invalid" >&2
    echo "aih_v4: expected low, medium, high" >&2
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
  ollama list 2>/dev/null | awk '
    NR > 1 {
      if (out != "") out = out ","
      out = out $1
    }
    END { print out }
  '
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

reject_cloud_agent_spec() {
  local label="$1"
  local spec="$2"
  if ((LOCAL_SMOKE == 0)); then
    return
  fi
  if [[ "$spec" =~ (^|[[:space:],:=])(openai|anthropic|gemini|google): ||
        "$spec" =~ (^|[[:space:],:=])(gpt-|claude-|gemini-) ]]; then
    echo "aih_v4: cloud agent rejected for local smoke test: $label=$spec" >&2
    echo "aih_v4: smoke tests default to --local-smoke. Use --full-agent-set when cloud agents are intentionally part of the run." >&2
    exit 2
  fi
}

if [[ -n "$CLOUD_SMOKE_PROVIDER" ]]; then
  prepare_provider_key "$CLOUD_SMOKE_PROVIDER"
  case "$CLOUD_SMOKE_PROVIDER" in
    openai)
      DEFAULT_WHITE_MODELS="${AIH_V4_WHITE_MODELS:-openai:gpt-4.1-mini}"
      DEFAULT_BLACK_MODELS="${AIH_V4_BLACK_MODELS:-openai:gpt-4.1-mini}"
      ;;
    google|gemini)
      DEFAULT_WHITE_MODELS="${AIH_V4_WHITE_MODELS:-gemini:gemini-3.1-flash-lite}"
      DEFAULT_BLACK_MODELS="${AIH_V4_BLACK_MODELS:-gemini:gemini-3.1-flash-lite}"
      ;;
    anthropic)
      DEFAULT_WHITE_MODELS="${AIH_V4_WHITE_MODELS:-anthropic:claude-3-5-haiku}"
      DEFAULT_BLACK_MODELS="${AIH_V4_BLACK_MODELS:-anthropic:claude-3-5-haiku}"
      ;;
    *)
      echo "aih_v4: unknown cloud smoke provider: $CLOUD_SMOKE_PROVIDER" >&2
      echo "aih_v4: expected openai, google/gemini, or anthropic" >&2
      exit 2
      ;;
  esac
  DEFAULT_BOARDS="${AIH_V4_BOARDS:-1}"
  export AICHESS_REASONING_PERFORMANCE_MODE="${AICHESS_REASONING_PERFORMANCE_MODE:-medium}"
  export AICHESS_OPENAI_REASONING_EFFORT="${AICHESS_OPENAI_REASONING_EFFORT:-medium}"
  export AICHESS_OPENAI_TEXT_VERBOSITY="${AICHESS_OPENAI_TEXT_VERBOSITY:-medium}"
  AIH_V4_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_cloud_provider_key_smoke_${CLOUD_SMOKE_PROVIDER}_medium_20260729}"
else
  if [[ "$SMOKE_STAGE" == "full-agent-set" ]]; then
    if [[ -z "${AIH_V4_WHITE_MODELS:-}" || -z "${AIH_V4_BLACK_MODELS:-}" ]]; then
      echo "aih_v4: full-agent-set requires explicit AIH_V4_WHITE_MODELS and AIH_V4_BLACK_MODELS." >&2
      echo "aih_v4: this prevents accidentally treating the local Ollama cache as the full v4 roster." >&2
      exit 2
    fi
    prepare_keys_for_specs "${AIH_V4_WHITE_MODELS},${AIH_V4_BLACK_MODELS},${AIH_V4_REFEREE_MODELS:-harness},${PASSTHRU_ARGS[*]}"
  fi
  LOCAL_AGENTS="$(discover_local_agents "$LOCAL_AGENT_REGISTRY")"
  LOCAL_AGENT_COUNT="$(csv_count "$LOCAL_AGENTS")"
  PAIR_START="${AIH_V4_LOCAL_PAIR_START:-1}"
  PAIR_COUNT="${AIH_V4_LOCAL_PAIR_COUNT:-1}"
  if ((PAIR_START < 1)); then
    PAIR_START=1
  fi
  if ((PAIR_COUNT < 1)); then
    PAIR_COUNT=1
  fi
  if ((LOCAL_AGENT_COUNT > 0 && PAIR_START > LOCAL_AGENT_COUNT)); then
    PAIR_START="$LOCAL_AGENT_COUNT"
  fi

  DEFAULT_WHITE_MODELS="${AIH_V4_WHITE_MODELS:-$(csv_range "$LOCAL_AGENTS" "$PAIR_START" "$PAIR_COUNT")}"
  DEFAULT_BLACK_MODELS="${AIH_V4_BLACK_MODELS:-$(csv_range "$(rotate_left_one "$LOCAL_AGENTS")" "$PAIR_START" "$PAIR_COUNT")}"
  DEFAULT_BOARDS="${AIH_V4_BOARDS:-$(csv_count "$DEFAULT_WHITE_MODELS")}"
  if [[ -z "$DEFAULT_WHITE_MODELS" || -z "$DEFAULT_BLACK_MODELS" || "$DEFAULT_BOARDS" == "0" ]]; then
    echo "aih_v4: no local agents discovered." >&2
    echo "aih_v4: checked registry: $LOCAL_AGENT_REGISTRY" >&2
    exit 2
  fi
fi

ALL_AGENT_SPECS="$DEFAULT_WHITE_MODELS,$DEFAULT_BLACK_MODELS,${AIH_V4_REFEREE_MODELS:-harness},${PASSTHRU_ARGS[*]}"
CLOUD_MATRIX_APPLIES=0
if spec_has_provider openai "$ALL_AGENT_SPECS" ||
   spec_has_provider google "$ALL_AGENT_SPECS" ||
   spec_has_provider anthropic "$ALL_AGENT_SPECS"; then
  CLOUD_MATRIX_APPLIES=1
fi

case "$SMOKE_STAGE" in
  local-progress)
    DEFAULT_MAXPLYS="${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-4}}"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V4_RESPONSE_ATTEMPTS:-1}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V4_MAX_FATAL_TURN_ERRORS:-1}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V4_OUTPUT_TOKENS:-1024}"
    DEFAULT_LOGLVL="${AIH_V4_LOGLVL:-2}"
    DEFAULT_CLUE_MODE="${AIH_V4_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_local_progress_smoke_lowmaxply_harness_referee_20260729}"
    ;;
  local-retry)
    DEFAULT_MAXPLYS="${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-$AIH_V4_LOCAL_MAXPLY_CAP}}"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V4_RESPONSE_ATTEMPTS:-3}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V4_MAX_FATAL_TURN_ERRORS:-1}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V4_OUTPUT_TOKENS:-1024}"
    DEFAULT_LOGLVL="${AIH_V4_LOGLVL:-5}"
    DEFAULT_CLUE_MODE="${AIH_V4_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_local_retry_concede_smoke_harness_referee_20260729}"
    ;;
  local-expand)
    DEFAULT_MAXPLYS="${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-$AIH_V4_LOCAL_MAXPLY_CAP}}"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V4_RESPONSE_ATTEMPTS:-3}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V4_MAX_FATAL_TURN_ERRORS:-1}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V4_OUTPUT_TOKENS:-1024}"
    DEFAULT_LOGLVL="${AIH_V4_LOGLVL:-4}"
    DEFAULT_CLUE_MODE="${AIH_V4_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_local_expand_smoke_harness_referee_20260729}"
    if [[ -z "${AIH_V4_LOCAL_PAIR_COUNT:-}" && -z "${AIH_V4_WHITE_MODELS:-}" && -z "${AIH_V4_BLACK_MODELS:-}" ]]; then
      echo "aih_v4: local-expand selected with default pair count $DEFAULT_BOARDS." >&2
      echo "aih_v4: set AIH_V4_LOCAL_PAIR_COUNT to scale this stage deliberately." >&2
    fi
    ;;
  cloud-provider-key)
    LOCAL_BASE_MAXPLYS="${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-$AIH_V4_LOCAL_MAXPLY_CAP}}"
    DEFAULT_MAXPLYS="$(derived_cloud_maxply "$LOCAL_BASE_MAXPLYS" "$AIH_V4_LOCAL_CLOUD_MAXPLY_RATIO")"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V4_RESPONSE_ATTEMPTS:-1}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V4_MAX_FATAL_TURN_ERRORS:-1}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V4_OUTPUT_TOKENS:-1024}"
    DEFAULT_LOGLVL="${AIH_V4_LOGLVL:-4}"
    DEFAULT_CLUE_MODE="${AIH_V4_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_cloud_provider_key_entitlement_smoke_${CLOUD_SMOKE_PROVIDER}_medium_20260729}"
    ;;
  full-agent-set)
    if ((CLOUD_MATRIX_APPLIES == 1)); then
      LOCAL_BASE_MAXPLYS="${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-$AIH_V4_LOCAL_MAXPLY_CAP}}"
      DEFAULT_MAXPLYS="$(derived_cloud_maxply "$LOCAL_BASE_MAXPLYS" "$AIH_V4_LOCAL_CLOUD_MAXPLY_RATIO")"
    else
      DEFAULT_MAXPLYS="${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-$AIH_V4_LOCAL_MAXPLY_CAP}}"
    fi
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V4_RESPONSE_ATTEMPTS:-3}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V4_MAX_FATAL_TURN_ERRORS:-1}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V4_OUTPUT_TOKENS:-2048}"
    DEFAULT_LOGLVL="${AIH_V4_LOGLVL:-4}"
    DEFAULT_CLUE_MODE="${AIH_V4_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_full_agent_set_smoke_harness_referee_20260729}"
    ;;
  *)
    echo "aih_v4: unknown smoke stage: $SMOKE_STAGE" >&2
    echo "aih_v4: expected local-progress, local-retry, local-expand, cloud-provider-key, or full-agent-set" >&2
    exit 2
    ;;
esac

reject_cloud_agent_spec "AIH_V4_WHITE_MODELS" "$DEFAULT_WHITE_MODELS"
reject_cloud_agent_spec "AIH_V4_BLACK_MODELS" "$DEFAULT_BLACK_MODELS"
reject_cloud_agent_spec "arguments" "${PASSTHRU_ARGS[*]}"

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
  CLOUD_BASE_MAXPLYS="$(derived_cloud_maxply "${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-$AIH_V4_LOCAL_MAXPLY_CAP}}" "$AIH_V4_LOCAL_CLOUD_MAXPLY_RATIO")"
fi

if ((CLOUD_MATRIX_APPLIES == 1 && DEFAULT_MAXPLYS > CLOUD_BASE_MAXPLYS)); then
  if [[ "${AIH_V4_CLOUD_MAXPLY_AFTER_REASONING_SWEEP:-0}" != "1" &&
        "${AIH_V4_CLOUD_MAXPLY_AFTER_REASONING_SWEEP:-0}" != "yes" &&
        "${AIH_V4_CLOUD_MAXPLY_AFTER_REASONING_SWEEP:-0}" != "true" ]]; then
    echo "aih_v4: refusing cloud maxply=$DEFAULT_MAXPLYS before reasoning/thought-level sweep is explicitly confirmed." >&2
    echo "aih_v4: first broaden AIH_V4_ALLOWED_REASONINGS or --allowed-reasonings; then set AIH_V4_CLOUD_MAXPLY_AFTER_REASONING_SWEEP=1 to raise --local-maxplys or lower --local-cloud-maxply-ratio." >&2
    exit 2
  fi
fi

if [[ ! -x "$ENGINE" ]]; then
  echo "aih_v4: engine is not executable: $ENGINE" >&2
  echo "aih_v4: run ./tools/build_aih_v4.sh first" >&2
  exit 127
fi

export AICHESS_TRACE_STRING_CHARS="${AICHESS_TRACE_STRING_CHARS:-1048576}"

ENGINE_ARGS=(
  --mode aichess
  --white-models "$DEFAULT_WHITE_MODELS"
  --black-models "$DEFAULT_BLACK_MODELS"
  --boards "$DEFAULT_BOARDS"
  --loops "${AIH_V4_LOOPS:-1}"
  --referee harness
  --mxply "$DEFAULT_MAXPLYS"
  --cnrtlm "$DEFAULT_RESPONSE_ATTEMPTS"
  --max-illegal "$DEFAULT_FATAL_TURN_ERRORS"
  --move-timeout "${AIH_V4_MOVE_TIMEOUT_SECONDS:-900}"
  --stack-timeout "${AIH_V4_STACK_TIMEOUT_SECONDS:-900}"
  --gmto "${AIH_V4_GAME_TIMEOUT_SECONDS:-7200}"
  --otkns "$DEFAULT_OUTPUT_TOKENS"
  --loglvl "$DEFAULT_LOGLVL"
  --clue-mode "$DEFAULT_CLUE_MODE"
)

if [[ "$ENABLE_BOARD_AWARENESS" == "1" || "$ENABLE_BOARD_AWARENESS" == "yes" || "$ENABLE_BOARD_AWARENESS" == "true" ]]; then
  ENGINE_ARGS+=(--board-awareness-probe)
fi

REASONING_COUNT="$(csv_count "$REASONING_RANGE")"
VERBOSITY_COUNT="$(csv_count "$VERBOSITY_RANGE")"
MATRIX_COUNT="$((REASONING_COUNT * VERBOSITY_COUNT))"

run_engine_for_config() {
  local reasoning="$1"
  local verbosity="$2"
  local reference_config="$3"
  export AICHESS_REASONING_PERFORMANCE_MODE="$reasoning"
  export AICHESS_OPENAI_REASONING_EFFORT="$reasoning"
  export AICHESS_VERBOSITY="$verbosity"
  export AICHESS_OPENAI_TEXT_VERBOSITY="$verbosity"
  "$ENGINE" \
    "${ENGINE_ARGS[@]}" \
    --reference-config "$reference_config" \
    "${PASSTHRU_ARGS[@]}"
}

if ((MATRIX_COUNT <= 1)); then
  reasoning="$(csv_field "$REASONING_RANGE" 1)"
  verbosity="$(csv_field "$VERBOSITY_RANGE" 1)"
  export AICHESS_REASONING_PERFORMANCE_MODE="$reasoning"
  export AICHESS_OPENAI_REASONING_EFFORT="$reasoning"
  export AICHESS_VERBOSITY="$verbosity"
  export AICHESS_OPENAI_TEXT_VERBOSITY="$verbosity"
  "$ENGINE" \
    "${ENGINE_ARGS[@]}" \
    --reference-config "$DEFAULT_REFERENCE_CONFIG" \
    "${PASSTHRU_ARGS[@]}"
  run_status=$?
  if ((run_status == 0)); then
    publish_latest_summary
  fi
  exit "$run_status"
fi

if ((CLOUD_MATRIX_APPLIES == 0)); then
  echo "aih_v4: reasoning/verbosity matrix requested but no cloud reasoning-capable agent specs were detected." >&2
  echo "aih_v4: running a single local configuration instead." >&2
  "$ENGINE" \
    "${ENGINE_ARGS[@]}" \
    --reference-config "$DEFAULT_REFERENCE_CONFIG" \
    "${PASSTHRU_ARGS[@]}"
  run_status=$?
  if ((run_status == 0)); then
    publish_latest_summary
  fi
  exit "$run_status"
fi

echo "aih_v4: running reasoning/verbosity matrix: allowed_reasonings=$REASONING_RANGE allowed_verbosity=$VERBOSITY_RANGE" >&2
matrix_failures=0
for reasoning_index in $(seq 1 "$REASONING_COUNT"); do
  reasoning="$(csv_field "$REASONING_RANGE" "$reasoning_index")"
  for verbosity_index in $(seq 1 "$VERBOSITY_COUNT"); do
    verbosity="$(csv_field "$VERBOSITY_RANGE" "$verbosity_index")"
    matrix_reference="${DEFAULT_REFERENCE_CONFIG}_reasoning-${reasoning}_verbosity-${verbosity}"
    echo "aih_v4: matrix config reasoning=$reasoning verbosity=$verbosity reference_config=$matrix_reference" >&2
    if ! run_engine_for_config "$reasoning" "$verbosity" "$matrix_reference"; then
      matrix_failures=$((matrix_failures + 1))
    fi
  done
done

if ((matrix_failures > 0)); then
  echo "aih_v4: matrix completed with failing configurations: $matrix_failures" >&2
  exit 1
fi

publish_latest_summary
