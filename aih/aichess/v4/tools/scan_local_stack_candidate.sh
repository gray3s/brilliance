#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  ./tools/scan_local_stack_candidate.sh PATH

Scans a candidate local AI stack directory for obvious hostile or high-risk
code patterns before the stack is allowed into the AIH v4 smoke roster.

This is a triage gate, not a proof of safety.
EOF
}

if (($# != 1)); then
  usage >&2
  exit 2
fi

TARGET="$1"
if [[ ! -e "$TARGET" ]]; then
  echo "scan_local_stack_candidate: target does not exist: $TARGET" >&2
  exit 2
fi

if ! command -v rg >/dev/null 2>&1; then
  echo "scan_local_stack_candidate: rg is required" >&2
  exit 127
fi

echo "AIH v4 local stack candidate scan"
echo "target=$TARGET"
echo

findings=0

scan_pattern() {
  local label="$1"
  local pattern="$2"
  echo "## $label"
  if rg -n --hidden --glob '!*.jsonl' --glob '!runs/**' --glob '!*.log' "$pattern" "$TARGET"; then
    findings=$((findings + 1))
  else
    echo "none"
  fi
  echo
}

scan_pattern "shell download/execute chains" 'curl|wget|Invoke-WebRequest|iwr|bash -c|sh -c|eval[[:space:]]|base64[[:space:]]+-d|openssl[[:space:]]+enc'
scan_pattern "privilege and persistence operations" 'sudo|su[[:space:]-]|chmod[[:space:]]+u\+s|setcap|systemctl|crontab|/etc/cron|~/.config/autostart|authorized_keys'
scan_pattern "destructive filesystem operations" 'rm[[:space:]]+-rf|shred|mkfs|dd[[:space:]].*of=|:(){:|fork bomb'
scan_pattern "credential and environment access" 'OPENAI_API_KEY|ANTHROPIC_API_KEY|GEMINI_API_KEY|GOOGLE_API_KEY|api[_-]?key|secret|token|password|printenv|process\.env|os\.environ'
scan_pattern "network exfiltration surfaces" 'https?://|scp[[:space:]]|rsync[[:space:]]|nc[[:space:]]|netcat|socat|webhook|pastebin|ngrok|tunnel'
scan_pattern "dynamic code loading" 'dlopen|LoadLibrary|importlib|exec\(|compile\(|Function\(|new Function|require\([^)]*\$|source[[:space:]]+'

if ((findings > 0)); then
  echo "scan_status=review_required"
  echo "finding_groups=$findings"
  exit 1
fi

echo "scan_status=no_obvious_high_risk_patterns"
echo "finding_groups=0"
