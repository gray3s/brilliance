#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE="$ROOT_DIR/qwen_ollama_chess_qt/qwen_ollama_chess_qt"
LOCAL_AGENT_REGISTRY="${AIH_V4_LOCAL_AGENT_REGISTRY:-$ROOT_DIR/../v3/qualification_cache/local_qualification_20260729032018.csv}"
LOCAL_SMOKE=1
CLOUD_SMOKE_PROVIDER=""
CLOUD_REPRESENTATIVE_PROVIDER=""
SMOKE_STAGE="${AIH_V4_SMOKE_STAGE:-local-retry}"
PUBLISH_LATEST_ONLY=0
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
AIH_V4_LOCAL_MAXPLY_CAP="${AIH_V4_LOCAL_MAXPLY_CAP:-50}"
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
    --publish-latest-only)
      PUBLISH_LATEST_ONLY=1
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

emit_aih_ranking_rows() {
  local jsonl="$1"
  jq -r '
    .events[]? |
    .model as $model |
    if (.transport_failure == true or .error == "move_request_transport_failure" or .response.status == "request_failed") then
      [$model, "oh"]
    elif (.legal_by_rules == true or .legal == true) then
      [$model, "legal"]
    elif ((.error // "") | test("hallucination|illegal_move|unparseable_move")) then
      [$model, "aih"]
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
        if (scored > 0) {
          aih_pct = pct(aih[agent], total)
          legal_pct = pct(legal[agent], total)
          agntoh_pct = pct(agntoh[agent], total)
          hrnoh_pct = pct(oh[agent], total)
          suspect = (aih_pct == 0 || hrnoh_pct == 0 || (legal_pct == 0 && aih_pct == 0) || (aih_pct == 0 && agntoh_pct == 0) || agntoh_pct >= 99.999)
          printf "%d\t%.8f\t%.8f\t%.8f\t%.8f\t%s\n",
            suspect ? 2 : 0,
            aih_pct,
            legal_pct,
            agntoh_pct,
            hrnoh_pct,
            agent
        } else {
          printf "2\t999999.00000000\t0.00000000\t0.00000000\t100.00000000\t%s\n", agent
        }
      }
    }
  ' |
  sort -t $'\t' -k1,1n -k2,2n -k4,4nr -k3,3nr -k6,6 |
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
      oh = $5 + 0
      agent = $6
      lc = ""
      title = agent
      if (title ~ /^[lc] /) {
        lc = substr(title, 1, 1)
        lc = toupper(lc)
        title = substr(title, 3)
      }
      if (group != 0) {
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
      printf "        <tr><td class=\"num\">%s</td><td class=\"num %s\">%s</td><td class=\"num\">%s</td><td class=\"num\">%s</td><td class=\"num\">%s</td><td>%s</td><td>%s</td></tr>\n",
        rank_s, class, aih_s, legal_s, agntoh_s, fmt(oh), esc(lc), esc(title)
    }
  '
}

publish_latest_summary() {
  local latest_summary latest_jsonl
  local published_dir published_summary published_jsonl published_html latest_html
  latest_summary="$(latest_summary_path)"
  echo "aih_v4: gen cur rpt..." >&2
  if [[ -z "$latest_summary" || ! -r "$latest_summary" ]]; then
    echo "aih_v4: no sum file." >&2
    return 2
  fi

  latest_jsonl="${latest_summary%_summary.md}.jsonl"
  published_dir="$ROOT_DIR/data"
  mkdir -p "$published_dir"
  published_summary="$published_dir/$(basename "$latest_summary")"
  published_jsonl="$published_dir/$(basename "$latest_jsonl")"
  published_html="$published_dir/$(basename "${latest_summary%_summary.md}.html")"
  latest_html="$ROOT_DIR/AIH_V4_PRELIMINARY_RESULTS_20260729.html"
  cp "$latest_summary" "$published_summary"
  if [[ -r "$latest_jsonl" ]]; then
    cp "$latest_jsonl" "$published_jsonl"
  fi

  {
    echo '<!doctype html>'
    echo '<html lang="en">'
    echo '<head>'
    echo '  <meta charset="utf-8">'
    echo '  <title>AIH v4 lat run</title>'
    echo '  <style>body{font-family:Arial,sans-serif;margin:2rem;line-height:1.45;color:#1f2933}table{border-collapse:collapse;width:100%;margin-top:1rem}th,td{border:1px solid #cad2dc;padding:.55rem;text-align:left;vertical-align:top}th{background:#eef2f6}.note{color:#52606d}code{background:#eef2f6;padding:.1rem .25rem;border-radius:3px}td.num{text-align:right;font-variant-numeric:tabular-nums}.aih-high{color:#b42318;font-weight:700}.aih-mid{color:#8a5a00;font-weight:700}.aih-low{color:#146c43;font-weight:700}</style>'
    echo '</head>'
    echo '<body>'
    echo '  <main>'
    echo '    <h1>AIH v4 lat run</h1>'
    echo "    <p class=\"note\">Summary: <code>data/$(basename "$published_summary")</code></p>"
    if [[ -r "$published_jsonl" ]]; then
      echo "    <p class=\"note\">JSONL: <code>data/$(basename "$published_jsonl")</code></p>"
    fi
    awk '/^GameMode:/ { print "    <p class=\"note\"><strong>GameMode:</strong> <code>" $2 "</code></p>" }' "$latest_summary"
    echo '    <p class="note">Sorted by lowest AIH%; N/A rows remain ranked after scored rows.</p>'
    echo '    <table>'
    echo '      <thead><tr><th>Rank</th><th class="num">AIH%</th><th class="num">Legal%</th><th class="num">AgntOH%</th><th class="num">HrnOH%</th><th>L/C</th><th>Agent Title</th></tr></thead>'
    echo '      <tbody>'
    if [[ -r "$published_jsonl" ]]; then
      emit_aih_ranking_rows "$published_jsonl"
    fi
    echo '      </tbody>'
    echo '    </table>'
    echo '  </main>'
    echo '</body>'
    echo '</html>'
  } > "$published_html"

  cp "$published_html" "$latest_html"

  echo "aih_v4: dat -> $published_dir." >&2
  echo "aih_v4: cur html: $published_html" >&2
  echo "aih_v4: lat html: $latest_html" >&2
  echo "aih_v4: rpt ok." >&2
}

if ((PUBLISH_LATEST_ONLY == 1)); then
  publish_latest_summary
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

reject_default_self_play() {
  local white_csv="$1"
  local black_csv="$2"
  local board_count="$3"
  local allow_self="${AIH_V4_ALLOW_SELF_PLAY:-0}"
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
    echo "aih_v4: refusing same-agent same-mode self-play by default." >&2
    echo "aih_v4: white-models=$white_csv" >&2
    echo "aih_v4: black-models=$black_csv" >&2
    echo "aih_v4: set AIH_V4_ALLOW_SELF_PLAY=1 only when self-play is intentional." >&2
    exit 2
  fi
}

if [[ -n "$CLOUD_SMOKE_PROVIDER" ]]; then
  prepare_provider_key "$CLOUD_SMOKE_PROVIDER"
  LOCAL_AGENTS="$(discover_local_agents "$LOCAL_AGENT_REGISTRY")"
  LOCAL_AGENT_COUNT="$(csv_count "$LOCAL_AGENTS")"
  if [[ -z "$LOCAL_AGENTS" || "$LOCAL_AGENT_COUNT" == "0" ]]; then
    echo "aih_v4: no local agents discovered for cloud smoke opponent." >&2
    echo "aih_v4: checked registry: $LOCAL_AGENT_REGISTRY" >&2
    exit 2
  fi
  case "$CLOUD_SMOKE_PROVIDER" in
    openai)
      DEFAULT_WHITE_MODELS="${AIH_V4_WHITE_MODELS:-openai:gpt-4.1-mini}"
      ;;
    google|gemini)
      DEFAULT_WHITE_MODELS="${AIH_V4_WHITE_MODELS:-gemini:gemini-3.5-flash-lite}"
      ;;
    anthropic)
      DEFAULT_WHITE_MODELS="${AIH_V4_WHITE_MODELS:-anthropic:claude-3-5-haiku}"
      ;;
    *)
      echo "aih_v4: unknown cloud smoke provider: $CLOUD_SMOKE_PROVIDER" >&2
      echo "aih_v4: expected openai, google/gemini, or anthropic" >&2
      exit 2
      ;;
  esac
  DEFAULT_BLACK_MODELS="${AIH_V4_BLACK_MODELS:-$(csv_field "$LOCAL_AGENTS" "${AIH_V4_CLOUD_REPRESENTATIVE_LOCAL_INDEX:-1}")}"
  DEFAULT_BOARDS="${AIH_V4_BOARDS:-1}"
  export AICHESS_REASONING_PERFORMANCE_MODE="${AICHESS_REASONING_PERFORMANCE_MODE:-medium}"
  export AICHESS_OPENAI_REASONING_EFFORT="${AICHESS_OPENAI_REASONING_EFFORT:-medium}"
  export AICHESS_OPENAI_TEXT_VERBOSITY="${AICHESS_OPENAI_TEXT_VERBOSITY:-medium}"
  AIH_V4_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_cloud_provider_key_smoke_${CLOUD_SMOKE_PROVIDER}_medium_20260729}"
elif [[ -n "$CLOUD_REPRESENTATIVE_PROVIDER" ]]; then
  prepare_provider_key "$CLOUD_REPRESENTATIVE_PROVIDER"
  LOCAL_AGENTS="$(discover_local_agents "$LOCAL_AGENT_REGISTRY")"
  LOCAL_AGENT_COUNT="$(csv_count "$LOCAL_AGENTS")"
  CLOUD_REPRESENTATIVE_START="${AIH_V4_CLOUD_REPRESENTATIVE_LOCAL_START:-${AIH_V4_CLOUD_REPRESENTATIVE_LOCAL_INDEX:-1}}"
  CLOUD_REPRESENTATIVE_COUNT="${AIH_V4_CLOUD_REPRESENTATIVE_LOCAL_COUNT:-$LOCAL_AGENT_COUNT}"
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
      echo "aih_v4: no loc agt for cld rep run." >&2
    echo "aih_v4: checked registry: $LOCAL_AGENT_REGISTRY" >&2
    exit 2
  fi
  case "$CLOUD_REPRESENTATIVE_PROVIDER" in
    google|gemini)
      DEFAULT_WHITE_MODELS="${AIH_V4_WHITE_MODELS:-gemini:gemini-3.5-flash-lite}"
      ;;
    *)
      echo "aih_v4: bad cld rep prv: $CLOUD_REPRESENTATIVE_PROVIDER" >&2
      echo "aih_v4: expected google/gemini" >&2
      exit 2
      ;;
  esac
  DEFAULT_BLACK_MODELS="${AIH_V4_BLACK_MODELS:-$(csv_range "$LOCAL_AGENTS" "$CLOUD_REPRESENTATIVE_START" "$CLOUD_REPRESENTATIVE_COUNT")}"
  DEFAULT_BOARDS="${AIH_V4_BOARDS:-$(csv_count "$DEFAULT_BLACK_MODELS")}"
  DEFAULT_WHITE_MODELS="${AIH_V4_WHITE_MODELS:-$(repeat_csv_value "$DEFAULT_WHITE_MODELS" "$DEFAULT_BOARDS")}"
  export AICHESS_REASONING_PERFORMANCE_MODE="${AICHESS_REASONING_PERFORMANCE_MODE:-medium}"
  export AICHESS_OPENAI_REASONING_EFFORT="${AICHESS_OPENAI_REASONING_EFFORT:-medium}"
  export AICHESS_OPENAI_TEXT_VERBOSITY="${AICHESS_OPENAI_TEXT_VERBOSITY:-medium}"
  AIH_V4_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_cld_rep_${CLOUD_REPRESENTATIVE_PROVIDER}_vs_loc_20260803}"
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
  local-prog)
    DEFAULT_MAXPLYS="${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-$AIH_V4_LOCAL_MAXPLY_CAP}}"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V4_RESPONSE_ATTEMPTS:-1}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V4_MAX_FATAL_TURN_ERRORS:-1}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V4_OUTPUT_TOKENS:-1024}"
    DEFAULT_LOGLVL="${AIH_V4_LOGLVL:-2}"
    DEFAULT_CLUE_MODE="${AIH_V4_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_local_prog_smoke_lowmaxply_harness_referee_20260729}"
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
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V4_RESPONSE_ATTEMPTS:-3}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V4_MAX_FATAL_TURN_ERRORS:-3}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V4_OUTPUT_TOKENS:-1024}"
    DEFAULT_LOGLVL="${AIH_V4_LOGLVL:-4}"
    DEFAULT_CLUE_MODE="${AIH_V4_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_cloud_provider_key_entitlement_smoke_${CLOUD_SMOKE_PROVIDER}_medium_20260729}"
    ;;
  cloud-rep)
    LOCAL_BASE_MAXPLYS="${AIH_V4_LOCAL_MAXPLYS:-${AIH_V4_MAXPLYS:-$AIH_V4_LOCAL_MAXPLY_CAP}}"
    DEFAULT_MAXPLYS="$(derived_cloud_maxply "$LOCAL_BASE_MAXPLYS" "$AIH_V4_LOCAL_CLOUD_MAXPLY_RATIO")"
    DEFAULT_RESPONSE_ATTEMPTS="${AIH_V4_RESPONSE_ATTEMPTS:-3}"
    DEFAULT_FATAL_TURN_ERRORS="${AIH_V4_MAX_FATAL_TURN_ERRORS:-3}"
    DEFAULT_OUTPUT_TOKENS="${AIH_V4_OUTPUT_TOKENS:-1024}"
    DEFAULT_LOGLVL="${AIH_V4_LOGLVL:-4}"
    DEFAULT_CLUE_MODE="${AIH_V4_CLUE_MODE:-6}"
    DEFAULT_REFERENCE_CONFIG="${AIH_V4_REFERENCE_CONFIG:-aih_v4_cld_rep_${CLOUD_REPRESENTATIVE_PROVIDER}_vs_loc_20260803}"
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
    echo "aih_v4: expected local-prog, local-retry, local-expand, cloud-provider-key, cloud-rep, or full-agent-set" >&2
    exit 2
    ;;
esac

reject_cloud_agent_spec "AIH_V4_WHITE_MODELS" "$DEFAULT_WHITE_MODELS"
reject_cloud_agent_spec "AIH_V4_BLACK_MODELS" "$DEFAULT_BLACK_MODELS"
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

if ((CLOUD_MATRIX_APPLIES == 1)); then
  DEFAULT_MOVE_TIMEOUT="${AIH_V4_MOVE_TIMEOUT_SECONDS:-10}"
  DEFAULT_STACK_TIMEOUT="${AIH_V4_STACK_TIMEOUT_SECONDS:-10}"
  DEFAULT_GAME_TIMEOUT="${AIH_V4_GAME_TIMEOUT_SECONDS:-1800}"
else
  DEFAULT_MOVE_TIMEOUT="${AIH_V4_MOVE_TIMEOUT_SECONDS:-900}"
  DEFAULT_STACK_TIMEOUT="${AIH_V4_STACK_TIMEOUT_SECONDS:-900}"
  DEFAULT_GAME_TIMEOUT="${AIH_V4_GAME_TIMEOUT_SECONDS:-7200}"
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
  --move-timeout "$DEFAULT_MOVE_TIMEOUT"
  --stack-timeout "$DEFAULT_STACK_TIMEOUT"
  --gmto "$DEFAULT_GAME_TIMEOUT"
  --otkns "$DEFAULT_OUTPUT_TOKENS"
  --loglvl "$DEFAULT_LOGLVL"
  --clue-mode "$DEFAULT_CLUE_MODE"
)

if [[ "$SMOKE_STAGE" == "cloud-rep" ]]; then
  ENGINE_ARGS+=(--tournament-bracket)
fi

if [[ "$ENABLE_BOARD_AWARENESS" == "1" || "$ENABLE_BOARD_AWARENESS" == "yes" || "$ENABLE_BOARD_AWARENESS" == "true" ]]; then
  ENGINE_ARGS+=(--board-awareness-probe)
fi

REASONING_COUNT="$(csv_count "$REASONING_RANGE")"
VERBOSITY_COUNT="$(csv_count "$VERBOSITY_RANGE")"
MATRIX_COUNT="$((REASONING_COUNT * VERBOSITY_COUNT))"

run_with_heartbeat() {
  local label="$1"
  shift
  local interval="${AIH_V4_HEARTBEAT_SECONDS:-5}"
  local elapsed=0
  local pid status
  "$@" &
  pid="$!"
  while kill -0 "$pid" 2>/dev/null; do
    sleep "$interval"
    if kill -0 "$pid" 2>/dev/null; then
      elapsed=$((elapsed + interval))
      echo "aih_v4: still running after ${elapsed}s: $label" >&2
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

if ((MATRIX_COUNT <= 1)); then
  reasoning="$(csv_field "$REASONING_RANGE" 1)"
  verbosity="$(csv_field "$VERBOSITY_RANGE" 1)"
  export AICHESS_REASONING_PERFORMANCE_MODE="$reasoning"
  export AICHESS_OPENAI_REASONING_EFFORT="$reasoning"
  export AICHESS_VERBOSITY="$verbosity"
  export AICHESS_OPENAI_TEXT_VERBOSITY="$verbosity"
  set +e
  run_with_heartbeat "eng refcfg=$DEFAULT_REFERENCE_CONFIG" \
    "$ENGINE" \
    "${ENGINE_ARGS[@]}" \
    --reference-config "$DEFAULT_REFERENCE_CONFIG" \
    "${PASSTHRU_ARGS[@]}"
  run_status=$?
  set -e
  if ((run_status == 0)) && ! passthru_has_arg "--dry-run"; then
    echo "aih_v4: eng ok; prep rpt..." >&2
    publish_latest_summary
  fi
  echo "aih_v4: exiting with status $run_status." >&2
  exit "$run_status"
fi

if ((CLOUD_MATRIX_APPLIES == 0)); then
  echo "aih_v4: mat req; no cloud cap agent." >&2
  echo "aih_v4: run 1 loc cfg." >&2
  set +e
  run_with_heartbeat "eng refcfg=$DEFAULT_REFERENCE_CONFIG" \
    "$ENGINE" \
    "${ENGINE_ARGS[@]}" \
    --reference-config "$DEFAULT_REFERENCE_CONFIG" \
    "${PASSTHRU_ARGS[@]}"
  run_status=$?
  set -e
  if ((run_status == 0)) && ! passthru_has_arg "--dry-run"; then
    echo "aih_v4: eng ok; prep rpt..." >&2
    publish_latest_summary
  fi
  echo "aih_v4: exiting with status $run_status." >&2
  exit "$run_status"
fi

echo "aih_v4: run rsn/vrb mat: rsn=$REASONING_RANGE vrb=$VERBOSITY_RANGE" >&2
matrix_failures=0
for reasoning_index in $(seq 1 "$REASONING_COUNT"); do
  reasoning="$(csv_field "$REASONING_RANGE" "$reasoning_index")"
  for verbosity_index in $(seq 1 "$VERBOSITY_COUNT"); do
    verbosity="$(csv_field "$VERBOSITY_RANGE" "$verbosity_index")"
    matrix_reference="${DEFAULT_REFERENCE_CONFIG}_reasoning-${reasoning}_verbosity-${verbosity}"
    echo "aih_v4: mat cfg rsn=$reasoning vrb=$verbosity refcfg=$matrix_reference" >&2
    if ! run_engine_for_config "$reasoning" "$verbosity" "$matrix_reference"; then
      matrix_failures=$((matrix_failures + 1))
    fi
  done
done

if ((matrix_failures > 0)); then
  echo "aih_v4: mat fail cfgs: $matrix_failures" >&2
  exit 1
fi

echo "aih_v4: eng mat ok; prep rpt..." >&2
if ! passthru_has_arg "--dry-run"; then
  publish_latest_summary
fi
echo "aih_v4: exiting with status 0." >&2
