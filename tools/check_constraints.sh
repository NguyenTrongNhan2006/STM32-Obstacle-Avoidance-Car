#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ARG="${1:-}"
if [ -n "$ARG" ] && [[ "$ARG" == Firmware/* ]]; then
  ARG="${ARG#Firmware/}"
fi
cd "$ROOT_DIR/Firmware"
if [ -n "$ARG" ]; then
  exec bash ./tools/check_constraints.sh "$ARG"
else
  exec bash ./tools/check_constraints.sh
fi
