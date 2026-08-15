#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
OUT_DIR="${AIH_V5_STANDALONE_REG_OUT_DIR:-$ROOT_DIR/standalone_registration_smoke}"
STAMP="$(date '+%Y%m%d_%H%M%S')"
RUN_DIR="$OUT_DIR/$STAMP"
SUMMARY_CSV="$RUN_DIR/summary.csv"
TIMEOUT_SECONDS="${AIH_V5_STANDALONE_REG_TIMEOUT_SECONDS:-180}"
NUM_PREDICT="${AIH_V5_STANDALONE_REG_NUM_PREDICT:-256}"
NUM_THREAD="${AIH_V5_STANDALONE_REG_NUM_THREAD:-1}"
KEEP_ALIVE="${AIH_V5_STANDALONE_REG_KEEP_ALIVE:-0s}"
API_SURFACE="${AIH_V5_STANDALONE_REG_API_SURFACE:-generate}"
THINK_SETTING="${AIH_V5_STANDALONE_REG_THINK:-}"
PROMPT_MODE="${AIH_V5_STANDALONE_REG_PROMPT_MODE:-clue}"
NORMALIZE_SAN="${AIH_V5_STANDALONE_REG_NORMALIZE_SAN:-0}"
LEGAL_CSV="a2a3,a2a4,b2b3,b2b4,c2c3,c2c4,d2d3,d2d4,e2e3,e2e4,f2f3,f2f4,g2g3,g2g4,h2h3,h2h4,b1a3,b1c3,g1f3,g1h3"

usage() {
  cat <<EOF
Usage:
  $0 [MODEL ...]

If no MODEL arguments are supplied, all installed Ollama models are tested.

Environment:
  AIH_V5_STANDALONE_REG_TIMEOUT_SECONDS   default $TIMEOUT_SECONDS
  AIH_V5_STANDALONE_REG_NUM_PREDICT       default $NUM_PREDICT
  AIH_V5_STANDALONE_REG_NUM_THREAD        default $NUM_THREAD
  AIH_V5_STANDALONE_REG_KEEP_ALIVE        default $KEEP_ALIVE
  AIH_V5_STANDALONE_REG_OUT_DIR           default $OUT_DIR
  AIH_V5_STANDALONE_REG_API_SURFACE       generate|chat, default $API_SURFACE
  AIH_V5_STANDALONE_REG_THINK             true|false, only sent when set
  AIH_V5_STANDALONE_REG_PROMPT_MODE       clue|legal-list|minimal, default $PROMPT_MODE
  AIH_V5_STANDALONE_REG_NORMALIZE_SAN     0|1, default $NORMALIZE_SAN
EOF
}

csv_escape() {
  local value="${1//$'\r'/ }"
  value="${value//$'\n'/ }"
  value="${value//\"/\"\"}"
  printf '"%s"' "$value"
}

safe_name() {
  printf '%s' "$1" | tr '/:' '__' | tr -cd 'A-Za-z0-9._-'
}

prompt_text() {
  case "$PROMPT_MODE" in
    legal-list)
      cat <<'EOF'
Chess move request.
Side to move: white.
FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1
Choose exactly one move from this legal UCI list:
a2a3 a2a4 b2b3 b2b4 c2c3 c2c4 d2d3 d2d4 e2e3 e2e4 f2f3 f2f4 g2g3 g2g4 h2h3 h2h4 b1a3 b1c3 g1f3 g1h3
Return only the selected UCI move.
No prose. No explanation. No punctuation.
EOF
      ;;
    minimal)
      cat <<'EOF'
Return one legal UCI chess move for the starting chess position with white to move.
Answer only with source square plus target square, like e2e4.
EOF
      ;;
    clue|*)
      cat <<'EOF'
Chess move request.
Ply: 1
FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1
Clue: use this legal UCI move: a2a3
Clue: current board FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1
Clue: board FEN after that suggested move: rnbqkbnr/pppppppp/8/8/8/P7/1PPPPPPP/RNBQKBNR b - - 0 1
Return exactly one legal UCI move and nothing else.
Answer format: e2e4
No prose. No explanation. No punctuation.
EOF
      ;;
  esac
}

extract_first_legal_uci() {
  local text="$1"
  awk -v legal_csv="$LEGAL_CSV" '
    BEGIN {
      n = split(legal_csv, legal, ",")
      for (i = 1; i <= n; ++i) legal_ok[legal[i]] = 1
    }
    {
      line = tolower($0)
      while (match(line, /[a-h][1-8][a-h][1-8][qrbn]?/)) {
        move = substr(line, RSTART, RLENGTH)
        if (legal_ok[move]) {
          print move
          exit
        }
        line = substr(line, RSTART + RLENGTH)
      }
    }
  ' <<< "$text"
}

normalize_registration_san() {
  local text="$1"
  if [[ "$NORMALIZE_SAN" != "1" && "$NORMALIZE_SAN" != "yes" && "$NORMALIZE_SAN" != "true" ]]; then
    return 0
  fi
  awk '
    {
      line = tolower($0)
      gsub(/[^a-h1-8=+#x-]+/, " ", line)
      n = split(line, parts, /[[:space:]]+/)
      for (i = 1; i <= n; ++i) {
        token = parts[i]
        gsub(/[+#]$/, "", token)
        if (token == "a3") { print "a2a3"; exit }
        if (token == "a4") { print "a2a4"; exit }
        if (token == "b3") { print "b2b3"; exit }
        if (token == "b4") { print "b2b4"; exit }
        if (token == "c3") { print "c2c3"; exit }
        if (token == "c4") { print "c2c4"; exit }
        if (token == "d3") { print "d2d3"; exit }
        if (token == "d4") { print "d2d4"; exit }
        if (token == "e3") { print "e2e3"; exit }
        if (token == "e4") { print "e2e4"; exit }
        if (token == "f3") { print "f2f3"; exit }
        if (token == "f4") { print "f2f4"; exit }
        if (token == "g3") { print "g2g3"; exit }
        if (token == "g4") { print "g2g4"; exit }
        if (token == "h3") { print "h2h3"; exit }
        if (token == "h4") { print "h2h4"; exit }
      }
    }
  ' <<< "$text"
}

models_from_ollama() {
  ollama list | awk 'NR > 1 && $1 != "" { print $1 }'
}

run_model() {
  local model="$1"
  local name request_json response_json curl_stderr status response thinking move done_reason eval_count response_len thinking_len elapsed
  name="$(safe_name "$model")"
  request_json="$RUN_DIR/${name}.request.json"
  response_json="$RUN_DIR/${name}.response.json"
  curl_stderr="$RUN_DIR/${name}.curl.stderr"

  if [[ "$API_SURFACE" == "chat" ]]; then
    if [[ -n "$THINK_SETTING" ]]; then
      jq -nc \
        --arg model "$model" \
        --arg content "$(prompt_text)" \
        --arg keep_alive "$KEEP_ALIVE" \
        --argjson num_predict "$NUM_PREDICT" \
        --argjson num_thread "$NUM_THREAD" \
        --argjson think "$THINK_SETTING" \
        '{model:$model,messages:[{role:"user",content:$content}],stream:false,think:$think,keep_alive:$keep_alive,options:{temperature:0,num_predict:$num_predict,num_thread:$num_thread}}' \
        > "$request_json"
    else
      jq -nc \
        --arg model "$model" \
        --arg content "$(prompt_text)" \
        --arg keep_alive "$KEEP_ALIVE" \
        --argjson num_predict "$NUM_PREDICT" \
        --argjson num_thread "$NUM_THREAD" \
        '{model:$model,messages:[{role:"user",content:$content}],stream:false,keep_alive:$keep_alive,options:{temperature:0,num_predict:$num_predict,num_thread:$num_thread}}' \
        > "$request_json"
    fi
  else
    jq -nc \
      --arg model "$model" \
      --arg prompt "$(prompt_text)" \
      --arg keep_alive "$KEEP_ALIVE" \
      --argjson num_predict "$NUM_PREDICT" \
      --argjson num_thread "$NUM_THREAD" \
      '{model:$model,prompt:$prompt,stream:false,keep_alive:$keep_alive,options:{temperature:0,num_predict:$num_predict,num_thread:$num_thread}}' \
      > "$request_json"
  fi

  local started ended
  started="$(date +%s)"
  set +e
  curl -sS \
    --connect-timeout 3 \
    --max-time "$TIMEOUT_SECONDS" \
    -H 'Content-Type: application/json' \
    -d @"$request_json" \
    "http://127.0.0.1:11434/api/$API_SURFACE" \
    > "$response_json" \
    2> "$curl_stderr"
  status=$?
  set -e
  ended="$(date +%s)"
  elapsed=$((ended - started))

  if ((status != 0)); then
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
      "$(csv_escape "$model")" fail curl_error "$elapsed" "$TIMEOUT_SECONDS" "$NUM_PREDICT" 0 0 "" "" "" "$response_json" "$(csv_escape "$(tr '\n\r' '  ' < "$curl_stderr")")" \
      >> "$SUMMARY_CSV"
    return 1
  fi

  if jq -e '.error?' "$response_json" >/dev/null 2>&1; then
    local err
    err="$(jq -r '.error // ""' "$response_json")"
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
      "$(csv_escape "$model")" fail ollama_error "$elapsed" "$TIMEOUT_SECONDS" "$NUM_PREDICT" 0 0 "" "" "" "$response_json" "$(csv_escape "$err")" \
      >> "$SUMMARY_CSV"
    return 1
  fi

  if [[ "$API_SURFACE" == "chat" ]]; then
    response="$(jq -r '.message.content // ""' "$response_json")"
    thinking="$(jq -r '.message.thinking // .thinking // ""' "$response_json")"
  else
    response="$(jq -r '.response // ""' "$response_json")"
    thinking="$(jq -r '.thinking // ""' "$response_json")"
  fi
  done_reason="$(jq -r '.done_reason // ""' "$response_json")"
  eval_count="$(jq -r '.eval_count // 0' "$response_json")"
  response_len="${#response}"
  thinking_len="${#thinking}"
  move="$(extract_first_legal_uci "$response")"
  if [[ -z "$move" ]]; then
    move="$(normalize_registration_san "$response")"
  fi

  if [[ -n "$move" ]]; then
    local pass_reason="visible_legal_uci"
    if ! extract_first_legal_uci "$response" >/dev/null || [[ -z "$(extract_first_legal_uci "$response")" ]]; then
      pass_reason="visible_normalized_san"
    fi
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
      "$(csv_escape "$model")" pass "$pass_reason" "$elapsed" "$TIMEOUT_SECONDS" "$NUM_PREDICT" "$response_len" "$thinking_len" "$done_reason" "$eval_count" "$move" "$response_json" "" \
      >> "$SUMMARY_CSV"
    return 0
  fi

  local thinking_move reason
  thinking_move="$(extract_first_legal_uci "$thinking")"
  if [[ -n "$thinking_move" ]]; then
    reason="thinking_only_legal_uci_not_official"
  elif [[ -z "${response//[[:space:]]/}" && "$done_reason" == "length" ]]; then
    reason="empty_visible_response_length"
  elif [[ -z "${response//[[:space:]]/}" ]]; then
    reason="empty_visible_response"
  else
    reason="no_legal_visible_uci"
  fi

  printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
    "$(csv_escape "$model")" fail "$reason" "$elapsed" "$TIMEOUT_SECONDS" "$NUM_PREDICT" "$response_len" "$thinking_len" "$done_reason" "$eval_count" "$thinking_move" "$response_json" "" \
    >> "$SUMMARY_CSV"
  return 1
}

main() {
  if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    usage
    exit 0
  fi
  command -v ollama >/dev/null 2>&1 || { echo "ollama command not found" >&2; exit 127; }
  command -v jq >/dev/null 2>&1 || { echo "jq command not found" >&2; exit 127; }
  command -v curl >/dev/null 2>&1 || { echo "curl command not found" >&2; exit 127; }

  mkdir -p "$RUN_DIR"
  printf 'model,status,reason,elapsed_seconds,timeout_seconds,num_predict,response_len,thinking_len,done_reason,eval_count,move_or_thinking_move,response_json,note\n' > "$SUMMARY_CSV"

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
    printf 'testing %s\n' "$model" >&2
    if run_model "$model"; then
      passed=$((passed + 1))
    else
      failed=$((failed + 1))
    fi
    ollama stop "$model" >/dev/null 2>&1 || true
  done

  echo "summary_csv=$SUMMARY_CSV"
  echo "run_dir=$RUN_DIR"
  echo "passed=$passed failed=$failed"
  awk -F, 'NR == 1 || $2 != "pass" { print }' "$SUMMARY_CSV" > "$RUN_DIR/failures.csv"
  echo "failures_csv=$RUN_DIR/failures.csv"
  return 0
}

main "$@"
