#!/usr/bin/env bash
# Compile and flash the relay board sketch to an ESP32-WROOM-32E (4MB flash).
# partitions.csv in the sketch folder is copied into the build by the esp32
# core, so the only extra needed is raising the app size check to match it.
#
# Usage:
#   ./scripts/deploy.sh                 # compile + upload
#   ./scripts/deploy.sh --compile-only  # compile, don't upload
#   ./scripts/deploy.sh --erase         # erase the whole chip, then upload
#                                       # (replaces an old bootloader/OTA layout)
#   PORT=/dev/ttyUSB1 ./scripts/deploy.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

COMPILE_ONLY=0
ERASE=0
for arg in "$@"; do
  case "$arg" in
    --compile-only) COMPILE_ONLY=1 ;;
    --erase) ERASE=1 ;;
    *) echo "Unknown option: $arg" >&2; exit 1 ;;
  esac
done

SKETCH=.

# Same step() helper as esp32-bdshare's deploy scripts: on success only the
# interesting lines are shown, on failure the whole output is printed and the
# script stops.
step() {
  local keep=$1 label=$2
  shift 2
  local out status=0
  out=$("$@" 2>&1) || status=$?
  out=$(printf '%s\n' "$out" | tr '\r' '\n')
  if [ "$status" -ne 0 ]; then
    printf '%s\n' "$out" >&2
    echo "failed: $label" >&2
    exit "$status"
  fi
  if [ -n "${SHOW:-}" ]; then
    printf '%s\n' "$out" | grep -E "$SHOW" || true
  else
    printf '%s\n' "$out" | tail -n "$keep"
  fi
}

# ESP32 Dev Module, 4MB flash
FQBN='esp32:esp32:esp32:FlashSize=4M,UploadSpeed=921600'
# factory app size in partitions.csv (0x200000); the core's default check is 1.2MB
APP_MAX=2097152
BUILD_DIR=build

export PATH="$HOME/.local/bin:$PATH"
need() { command -v "$1" >/dev/null 2>&1 || { echo "missing: $1 -- run make setup-env" >&2; exit 1; }; }
need arduino-cli

[ -f secrets.h ] || {
  echo "missing secrets.h -- copy secrets.h.example and fill in WiFi details" >&2
  exit 1
}

echo "=== compile relay_board_api_server ==="
step 2 "compile" \
  arduino-cli compile --fqbn "$FQBN" \
    --build-property upload.maximum_size=$APP_MAX \
    --build-path "$BUILD_DIR" "$SKETCH"

[ "$COMPILE_ONLY" -eq 1 ] && exit 0

PORT=${PORT:-$(ls /dev/cu.usbmodem* /dev/cu.usbserial* /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -1 || true)}
[ -n "$PORT" ] || { echo "no /dev/cu.usb*, /dev/ttyACM*, or /dev/ttyUSB* found; plug the board in, or set PORT=..." >&2; exit 1; }

# Upload always writes bootloader (0x1000), partition table (0x8000),
# boot_app0 (0xe000, unused here -- no otadata) and the app (0x10000).
# EraseFlash=all wipes everything else too: old NVS, OTA slots, filesystem.
UPLOAD_FQBN=$FQBN
if [ "$ERASE" -eq 1 ]; then
  UPLOAD_FQBN="$FQBN,EraseFlash=all"
  echo "=== erase whole flash + upload firmware -> $PORT ==="
else
  echo "=== upload firmware -> $PORT ==="
fi
step 1 "upload firmware (port busy? close any serial monitor)" \
  arduino-cli upload -p "$PORT" --fqbn "$UPLOAD_FQBN" --input-dir "$BUILD_DIR" "$SKETCH"

echo
echo "flashed : relay board api server -> $PORT"
echo "watch boot output with: make monitor PORT=$PORT"
