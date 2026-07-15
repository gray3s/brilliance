#!/usr/bin/env bash
set -euo pipefail

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
echo "== Checking Ubuntu-supported Ollama package availability =="
apt-cache policy ollama || true

if apt-cache policy ollama | grep -q 'Candidate: (none)'; then
  echo "No Ubuntu-supported ollama package candidate is available from configured apt sources."
  echo "Skipping Ollama upgrade to avoid installing unsupported upstream/vendor software."
else
  echo
  echo "== Attempting Ubuntu-supported Ollama package upgrade =="
  sudo apt-get install --only-upgrade -y ollama

  if systemctl list-unit-files | grep -q '^ollama\.service'; then
    echo
    echo "== Restarting ollama service =="
    sudo systemctl restart ollama
  fi
fi

echo
echo "== Checking available tinyproxy package =="
apt-cache policy tinyproxy || true

echo
echo "== Attempting tinyproxy package upgrade =="
sudo apt-get install --only-upgrade -y tinyproxy

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
