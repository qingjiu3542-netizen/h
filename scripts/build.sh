#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${THEOS:-}" ]]; then
  echo "Please export THEOS before building."
  exit 1
fi

make clean package FINALPACKAGE=1
