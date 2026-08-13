#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="/home/sag/RPA2/myLLC/AI/brilliance"
V5_REL="aih/aichess/v5"
V5_DIR="${REPO_ROOT}/${V5_REL}"
PUBLISHED_DIR="${V5_DIR}/published_results"
BRANCH="main"
TODAY="$(date +%Y%m%d)"
REMOTE_BLOB_BASE="https://github.com/gray3s/brilliance/blob/${BRANCH}"
HTMLPREVIEW_BASE="https://htmlpreview.github.io/?${REMOTE_BLOB_BASE}"
COMMIT_MESSAGE="Update AIH v5 public benchmark artifacts"

cd "${V5_DIR}"

echo "[aih-v5-push] refreshing public AIH v5 artifacts"
echo "[aih-v5-push] source: ${V5_DIR}"
echo "[aih-v5-push] destination: ${PUBLISHED_DIR}"

mkdir -p "${PUBLISHED_DIR}"

echo "[aih-v5-push] building current AIH v5 binaries"
./tools/build_aih_v5.sh

if [[ -x ./bin/aih_v5_repeat_html ]]; then
  echo "[aih-v5-push] running CSV HTML processor"
  ./bin/aih_v5_repeat_html
else
  echo "[aih-v5-push] missing executable CSV HTML processor: ./bin/aih_v5_repeat_html" >&2
  exit 2
fi

if [[ -x ./aih_v5_refresh_source_zip_20260813v0.sh ]]; then
  echo "[aih-v5-push] refreshing source archive"
  ./aih_v5_refresh_source_zip_20260813v0.sh
else
  echo "[aih-v5-push] missing executable source zip refresher: ./aih_v5_refresh_source_zip_20260813v0.sh" >&2
  exit 2
fi

cp -f AIH_V5_PROJECT_GOALS.md "${PUBLISHED_DIR}/AIH_V5_PROJECT_GOALS_20260813.md"
cp -f AIH_V5_IMPLEMENTATION_PLAN.md "${PUBLISHED_DIR}/AIH_V5_IMPLEMENTATION_PLAN_20260813.md"
cp -f data/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html "${PUBLISHED_DIR}/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html"
cp -f data/AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html "${PUBLISHED_DIR}/AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html"
cp -f data/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html AIH_V5_CSV_AGGREGATE_LATEST.html
cp -f data/AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html

relpaths=(
  "${V5_REL}/README.md"
  "${V5_REL}/RUN_AIH_V5_BINARY_INSTRUCTIONS_20260810.md"
  "${V5_REL}/AIH_V5_PROJECT_GOALS.md"
  "${V5_REL}/AIH_V5_IMPLEMENTATION_PLAN.md"
  "${V5_REL}/AIH_V5_CSV_AGGREGATE_LATEST.html"
  "${V5_REL}/AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html"
  "${V5_REL}/AIH_V5_SOURCE_LATEST.zip"
  "${V5_REL}/AIH_V5_SOURCE_${TODAY}.zip"
  "${V5_REL}/data/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html"
  "${V5_REL}/data/AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html"
  "${V5_REL}/published_results/AIH_V5_PROJECT_GOALS_20260813.md"
  "${V5_REL}/published_results/AIH_V5_IMPLEMENTATION_PLAN_20260813.md"
  "${V5_REL}/published_results/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html"
  "${V5_REL}/published_results/AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html"
  "${V5_REL}/aih_v5.sh"
  "${V5_REL}/aih_v5_push_current_20260813v0.sh"
  "${V5_REL}/aih_v5_refresh_source_zip_20260813v0.sh"
  "${V5_REL}/tools/build_aih_v5.sh"
  "${V5_REL}/tools/generate_aih_v5_html_report.cpp"
  "${V5_REL}/tools/generate_aih_v5_repeat_html.cpp"
  "${V5_REL}/tools/run_aih_v5_single_game.cpp"
)

cd "${REPO_ROOT}"

current_branch="$(git branch --show-current)"
if [[ "${current_branch}" != "${BRANCH}" ]]; then
  echo "[aih-v5-push] refusing to push from branch '${current_branch}', expected '${BRANCH}'" >&2
  exit 2
fi

echo
echo "[aih-v5-push] file set:"
for relpath in "${relpaths[@]}"; do
  if [[ ! -r "${relpath}" ]]; then
    echo "[aih-v5-push] missing required file: ${relpath}" >&2
    exit 2
  fi
  echo "  ${relpath}"
done

echo
echo "[aih-v5-push] links after push:"
echo "  Project goals: ${REMOTE_BLOB_BASE}/${V5_REL}/AIH_V5_PROJECT_GOALS.md"
echo "  Project implementation plan: ${REMOTE_BLOB_BASE}/${V5_REL}/AIH_V5_IMPLEMENTATION_PLAN.md"
echo "  Demo / current HTML: ${HTMLPREVIEW_BASE}/${V5_REL}/AIH_V5_CSV_AGGREGATE_LATEST.html"
echo "  AIH v5 Code: ${REMOTE_BLOB_BASE}/${V5_REL}/AIH_V5_SOURCE_LATEST.zip"
echo "  Diagnostic companion HTML: ${HTMLPREVIEW_BASE}/${V5_REL}/AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html"

echo
echo "[aih-v5-push] git status before staging:"
git status --short -- "${relpaths[@]}"

echo
echo "[aih-v5-push] staging exact AIH v5 file set"
git add -- "${relpaths[@]}"

if git diff --cached --quiet -- "${relpaths[@]}"; then
  echo "[aih-v5-push] no staged changes; nothing to commit or push"
  exit 0
fi

echo "[aih-v5-push] committing"
git commit -m "${COMMIT_MESSAGE}" -- "${relpaths[@]}"

echo "[aih-v5-push] fetching origin/${BRANCH}"
git fetch origin "${BRANCH}"

if ! git merge-base --is-ancestor "origin/${BRANCH}" HEAD; then
  echo "[aih-v5-push] merging remote ${BRANCH} before push"
  git merge --no-edit "origin/${BRANCH}"
fi

echo "[aih-v5-push] pushing origin/${BRANCH}"
git push origin "${BRANCH}"

echo
echo "[aih-v5-push] pushed AIH v5 public artifacts"
