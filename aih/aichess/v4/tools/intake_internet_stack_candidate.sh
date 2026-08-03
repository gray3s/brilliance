#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  ./tools/intake_internet_stack_candidate.sh URL

Downloads a local AI stack candidate into quarantine and runs pre-integration
hostility checks. The downloaded artifact is not executed and is not made
executable by this tool.

If the candidate requires an API key, provider token, cloud account
entitlement, or cloud licensing grant for inference, it is invalidated as
local AI and must be handled as a cloud-backed provider path instead.

Required follow-up before integration:
  1. review provenance
  2. review checksums and file metadata
  3. inspect archive contents before extraction
  4. extract only into quarantine
  5. scan extracted contents
  6. run sandboxed liveness only after review

Sandboxing is required for internet-downloaded stack candidates. This intake
tool intentionally stops before extraction and execution.
EOF
}

if (($# != 1)); then
  usage >&2
  exit 2
fi

URL="$1"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="$(date -u +%Y%m%d%H%M%S)"
QUARANTINE_ROOT="${AIH_V4_QUARANTINE_ROOT:-$ROOT_DIR/quarantine/internet_stack_candidates}"
RUN_DIR="$QUARANTINE_ROOT/$STAMP"
mkdir -p "$RUN_DIR"

case "$URL" in
  http://*|https://*) ;;
  *)
    echo "intake_internet_stack_candidate: only http/https URLs are accepted" >&2
    exit 2
    ;;
esac

artifact_name="$(basename "${URL%%\?*}")"
if [[ -z "$artifact_name" || "$artifact_name" == "/" || "$artifact_name" == "." ]]; then
  artifact_name="downloaded_stack_candidate"
fi
artifact_path="$RUN_DIR/$artifact_name"
meta_path="$RUN_DIR/intake.meta"

{
  echo "url=$URL"
  echo "timestamp_utc=$STAMP"
  echo "quarantine_dir=$RUN_DIR"
  echo "artifact_path=$artifact_path"
  echo "policy=download_only_no_execute_no_chmod"
} > "$meta_path"

curl --fail --location --show-error --output "$artifact_path" "$URL"
chmod 0644 "$artifact_path"

{
  echo
  echo "downloaded_bytes=$(wc -c < "$artifact_path")"
  echo "sha256=$(sha256sum "$artifact_path" | awk '{print $1}')"
  echo "file_type=$(file -b "$artifact_path")"
} >> "$meta_path"

echo "AIH v4 internet stack candidate intake"
cat "$meta_path"
echo

case "$artifact_path" in
  *.zip)
    echo "archive_listing=zip"
    unzip -l "$artifact_path" | tee "$RUN_DIR/archive_listing.txt"
    ;;
  *.tar|*.tar.gz|*.tgz|*.tar.xz|*.txz|*.tar.bz2|*.tbz2)
    echo "archive_listing=tar"
    tar -tf "$artifact_path" | tee "$RUN_DIR/archive_listing.txt"
    ;;
  *)
    echo "archive_listing=not_archive_or_unknown"
    ;;
esac

echo
echo "intake_status=quarantined_review_required"
echo "sandbox_required=true"
echo "next_scan=$ROOT_DIR/tools/scan_local_stack_candidate.sh EXTRACTED_QUARANTINE_DIR"
