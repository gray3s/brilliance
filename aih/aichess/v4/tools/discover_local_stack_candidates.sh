#!/usr/bin/env bash
set -euo pipefail

cat <<'EOF'
AIH v4 local stack candidate discovery

This lists local inference runtime entry points that may be worth onboarding
after provenance review, static scanning, and sandboxed liveness tests.

Cloud-backed CLIs are reported separately. A stack is not a local AI runtime if
inference depends on a cloud token provider, even when those tokens are free of
charge.

Operational rule: if the AI cannot run without a working internet connection,
it is not local.

API keys and equivalent provider tokens are cloud licensing/access-control
mechanisms. A stack that requires them for inference is not local.

A provider key working for one agent does not prove every agent/model/mode on
that provider is authorized. Any candidate that needs a key, token, account
entitlement, or cloud licensing grant is invalidated as local AI and belongs
in cloud provider-path smoke testing.
EOF

check_cmd() {
  local label="$1"
  local cmd="$2"
  if command -v "$cmd" >/dev/null 2>&1; then
    printf 'candidate\t%s\t%s\t%s\n' "$label" "$cmd" "$(command -v "$cmd")"
  else
    printf 'missing\t%s\t%s\t-\n' "$label" "$cmd"
  fi
}

check_path() {
  local label="$1"
  local path="$2"
  if [[ -e "$path" ]]; then
    printf 'candidate_path\t%s\t-\t%s\n' "$label" "$path"
  else
    printf 'missing_path\t%s\t-\t%s\n' "$label" "$path"
  fi
}

echo
echo "status	label	command_or_path	resolved_path"
check_cmd "ollama server/client" "ollama"
check_cmd "llama.cpp cli" "llama-cli"
check_cmd "llama.cpp server" "llama-server"
check_cmd "llamafile" "llamafile"
check_cmd "koboldcpp" "koboldcpp"
check_cmd "vllm" "vllm"
check_cmd "text-generation-launcher" "text-generation-launcher"
check_cmd "lm studio cli" "lms"
check_cmd "localai" "local-ai"
check_cmd "jan" "jan"

echo
echo "Common local model/cache paths:"
check_path "ollama models" "$HOME/.ollama/models"
check_path "huggingface cache" "$HOME/.cache/huggingface"
check_path "lm studio models" "$HOME/.cache/lm-studio/models"
check_path "jan data" "$HOME/jan"

echo
echo "Cloud-backed CLI executables, not local inference runtimes:"
check_cmd "codex cli cloud-backed" "codex"
