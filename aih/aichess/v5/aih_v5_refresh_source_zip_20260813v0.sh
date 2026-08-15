#!/usr/bin/env bash
set -euo pipefail

ROOT="/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5"
TODAY="$(date +%Y%m%d)"
STAMP="$(date +%Y%m%d_%H%M%S)"
SOURCE_ZIP_DATED="${ROOT}/AIH_V5_SOURCE_${TODAY}.zip"
SOURCE_ZIP_LATEST="${ROOT}/AIH_V5_SOURCE-LATEST.zip"
SOURCE_ZIP_LATEST_UNDERSCORE="${ROOT}/AIH_V5_SOURCE_LATEST.zip"
SOURCE_STAGE="${ROOT}/.aih_v5_source_stage_${STAMP}"

command -v zip >/dev/null 2>&1 || {
  echo "[aih-v5-source-zip] zip is required" >&2
  exit 2
}

rm -rf -- "${SOURCE_STAGE}"
mkdir -p "${SOURCE_STAGE}/aih/aichess/v5"

copy_source_file() {
  local src="$1"
  [[ -f "${src}" ]] || return 0

  local rel="${src#${ROOT}/}"
  local dst="${SOURCE_STAGE}/aih/aichess/v5/${rel}"

  mkdir -p "$(dirname "${dst}")"
  cp -p -- "${src}" "${dst}"
}

copy_source_file "${ROOT}/aih_v5.sh"
copy_source_file "${ROOT}/README.md"

copy_source_file "${ROOT}/AIH_V5_LOCAL_AGENT_REGISTRATION_VERIFIED.csv"
copy_source_file "${ROOT}/AIH_V5_LOCAL_AGENT_REGISTRATION_FAILURES.csv"
copy_source_file "${ROOT}/AIH_V5_LOCAL_AGENT_REGISTRATION_COMM_SETTINGS.csv"
copy_source_file "${ROOT}/AIH_V5_LOCAL_AGENT_REGISTRATION_SUMMARY.csv"
copy_source_file "${ROOT}/AIH_V5_LOCAL_AGENT_REGISTRATION_README.md"
copy_source_file "${ROOT}/AIH_V5_OLLAMA_AGENT_COMMUNICATION_SETTINGS_20260815.csv"
copy_source_file "${ROOT}/AIH_V5_OLLAMA_REGISTRATION_TIMEOUT_PROGRESS_20260815.md"

while IFS= read -r -d '' f; do
  copy_source_file "${f}"
done < <(
  find "${ROOT}" -type f \
    \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o \
       -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o \
       -name '*.pro' -o -name 'CMakeLists.txt' -o \
       -name 'Makefile' -o -name '*.pri' -o -name '*.sh' \) \
    ! -path '*/.git/*' \
    ! -path '*/logs/*' \
    ! -path '*/data/*' \
    ! -path '*/runs/*' \
    ! -path '*/published_results/*' \
    ! -path '*/qualification_cache/*' \
    ! -path '*/node_modules/*' \
    ! -path '*/build/*' \
    ! -path '*/.cache/*' \
    ! -path '*/.aih_v5_source_stage_*/*' \
    ! -path "${SOURCE_STAGE}/*" \
    -print0
)

SOURCE_MANIFEST="${SOURCE_STAGE}/aih/aichess/v5/AIH_V5_SOURCE_MANIFEST.txt"

{
  printf 'AIH v5 source-only archive\n'
  printf 'Generated: %s\n' "$(date '+%Y-%m-%d %H:%M:%S %Z')"
  printf 'Source root: %s\n' "${ROOT}"
  printf '\nPurpose:\n'
  printf 'Current AIH v5 C/C++/Qt and supporting source code for public review/recreation.\n'
  printf '\nExplicitly excluded:\n'
  printf '%s\n' '- compiled executables/binaries'
  printf '%s\n' '- *.o and other object files'
  printf '%s\n' '- raw run CSV data'
  printf '%s\n' '- raw registration response JSON'
  printf '%s\n' '- JSONL event/run data'
  printf '%s\n' '- logs'
  printf '%s\n' '- caches'
  printf '%s\n' '- generated run output'
  printf '\nIncluded files:\n'
  (
    cd "${SOURCE_STAGE}" || exit
    find aih/aichess/v5 -type f -printf '%p\n' | sort
  )
} > "${SOURCE_MANIFEST}"

rm -f -- "${SOURCE_ZIP_DATED}" "${SOURCE_ZIP_LATEST}" "${SOURCE_ZIP_LATEST_UNDERSCORE}"

(
  cd "${SOURCE_STAGE}" || exit 2
  zip -q -r "${SOURCE_ZIP_DATED}" aih
)

cp -f -- "${SOURCE_ZIP_DATED}" "${SOURCE_ZIP_LATEST}"
cp -f -- "${SOURCE_ZIP_DATED}" "${SOURCE_ZIP_LATEST_UNDERSCORE}"

file_count="$(unzip -Z1 "${SOURCE_ZIP_DATED}" | grep -v '/$' | wc -l)"
sha256="$(sha256sum "${SOURCE_ZIP_DATED}" | awk '{print $1}')"

rm -rf -- "${SOURCE_STAGE}"

echo "[aih-v5-source-zip] wrote ${SOURCE_ZIP_DATED}"
echo "[aih-v5-source-zip] wrote ${SOURCE_ZIP_LATEST}"
echo "[aih-v5-source-zip] wrote ${SOURCE_ZIP_LATEST_UNDERSCORE}"
echo "[aih-v5-source-zip] files: ${file_count}"
echo "[aih-v5-source-zip] sha256: ${sha256}"
