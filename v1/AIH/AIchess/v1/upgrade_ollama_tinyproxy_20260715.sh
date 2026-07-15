#!/usr/bin/env bash
set -euo pipefail

version_from_text() {
  grep -Eo '[0-9]+([.][0-9]+)+' | tail -n 1
}

ollama_local_version() {
  if command -v ollama >/dev/null 2>&1; then
    ollama --version 2>&1 | version_from_text || true
  fi
}

ollama_latest_version() {
  curl -fsSL https://api.github.com/repos/ollama/ollama/releases/latest \
    | sed -n 's/.*"tag_name":[[:space:]]*"v\([^"]*\)".*/\1/p' \
    | head -n 1
}

tinyproxy_installed_version() {
  dpkg-query -W -f='${Version}' tinyproxy 2>/dev/null || true
}

tinyproxy_candidate_version() {
  apt-cache policy tinyproxy | awk '/Candidate:/ {print $2}'
}

echo "== Before =="
if command -v ollama >/dev/null 2>&1; then
  ollama --version || true
else
  echo "ollama not found"
fi

if command -v tinyproxy >/dev/null 2>&1; then
  tinyproxy -v || true
else
  echo "tinyproxy not found"
fi

echo
echo "== Updating apt metadata =="
sudo apt-get update

echo
echo "== Checking Ollama latest available version =="
ollama_local="$(ollama_local_version)"
ollama_latest="$(ollama_latest_version || true)"

echo "Local Ollama:  ${ollama_local:-not installed or unknown}"
echo "Latest Ollama: ${ollama_latest:-unknown}"

if [[ -z "${ollama_latest}" ]]; then
  echo "Could not determine latest Ollama version. Trying official installer anyway."
  curl -fsSL https://ollama.com/install.sh | sh
elif [[ -z "${ollama_local}" ]]; then
  echo "Ollama is not installed or local version is unknown. Installing latest available Ollama."
  curl -fsSL https://ollama.com/install.sh | sh
elif dpkg --compare-versions "$ollama_latest" gt "$ollama_local"; then
  echo "Ollama upgrade available: $ollama_local -> $ollama_latest"
  curl -fsSL https://ollama.com/install.sh | sh
else
  echo "Ollama is already at the latest detected version."
fi

if systemctl list-unit-files | grep -q '^ollama\.service'; then
  echo
  echo "== Restarting ollama service =="
  sudo systemctl restart ollama
fi

echo
echo "== Checking available tinyproxy package =="
apt-cache policy tinyproxy || true

echo
echo "== Checking Tinyproxy latest available apt version =="
tinyproxy_installed="$(tinyproxy_installed_version)"
tinyproxy_candidate="$(tinyproxy_candidate_version)"

echo "Installed Tinyproxy: ${tinyproxy_installed:-not installed}"
echo "Candidate Tinyproxy: ${tinyproxy_candidate:-unknown}"

if [[ -z "${tinyproxy_candidate}" || "${tinyproxy_candidate}" == "(none)" ]]; then
  echo "No Tinyproxy apt candidate is available from configured repositories."
elif [[ -z "${tinyproxy_installed}" ]]; then
  echo "Tinyproxy is not installed. Installing latest available apt candidate."
  sudo apt-get install -y tinyproxy
elif dpkg --compare-versions "$tinyproxy_candidate" gt "$tinyproxy_installed"; then
  echo "Tinyproxy upgrade available: $tinyproxy_installed -> $tinyproxy_candidate"
  sudo apt-get install --only-upgrade -y tinyproxy
else
  echo "Tinyproxy is already at the latest apt candidate."
fi

if systemctl list-unit-files | grep -q '^tinyproxy\.service'; then
  echo
  echo "== Restarting tinyproxy service =="
  sudo systemctl restart tinyproxy
fi

echo
echo "== After =="
if command -v ollama >/dev/null 2>&1; then
  ollama --version || true
else
  echo "ollama not found after upgrade"
fi

if command -v tinyproxy >/dev/null 2>&1; then
  tinyproxy -v || true
else
  echo "tinyproxy not found after upgrade"
fi

echo
echo "Done."
