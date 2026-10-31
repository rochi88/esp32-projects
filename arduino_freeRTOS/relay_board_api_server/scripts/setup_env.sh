#!/usr/bin/env bash
# Check (and optionally install) the toolchain this sketch needs, and report
# what boards are connected. Doesn't compile or flash anything -- once this
# passes, use `make deploy`.
#
# Adapted from esp32-bdshare's scripts/setup_env.sh, trimmed to what a single
# ESP32-WROOM-32E sketch needs (no uv/mosquitto).
#
# Usage:
#   ./scripts/setup_env.sh            # check, and install anything missing
#   ./scripts/setup_env.sh --check    # report only, install nothing
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CHECK_ONLY=0
for arg in "$@"; do
  case "$arg" in
    --check) CHECK_ONLY=1 ;;
    *) echo "Unknown option: $arg" >&2; exit 1 ;;
  esac
done

ok()   { echo "  [ok] $1"; }
miss() { echo "  [--] $1"; }
fail() { echo "  [!!] $1" >&2; }

# esptool on PATH first, then the copy bundled with the arduino esp32 core
find_esptool() {
  command -v esptool >/dev/null 2>&1 && { echo esptool; return; }
  command -v esptool.py >/dev/null 2>&1 && { echo esptool.py; return; }
  ls -d "${ARDUINO_DATA_DIR:-$HOME/.arduino15}"/packages/esp32/tools/esptool_py/*/esptool 2>/dev/null | sort -V | tail -1
}

export PATH="$HOME/.local/bin:$PATH"

echo "=== toolchain ==="

if command -v arduino-cli >/dev/null 2>&1; then
  ok "arduino-cli $(arduino-cli version | head -1)"
else
  miss "arduino-cli not found"
  if [ "$CHECK_ONLY" -eq 0 ]; then
    echo "  installing arduino-cli..."
    if [[ "$(uname)" == "Darwin" ]]; then
      brew install arduino-cli
    else
      mkdir -p "$HOME/.local/bin"
      curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$HOME/.local/bin" sh
    fi
  fi
fi

if command -v arduino-cli >/dev/null 2>&1; then
  if arduino-cli core list 2>/dev/null | grep -q '^esp32:esp32 '; then
    ok "esp32 core installed ($(arduino-cli core list 2>/dev/null | awk '/^esp32:esp32 /{print $2; exit}'))"
  else
    miss "esp32 core not installed"
    if [ "$CHECK_ONLY" -eq 0 ]; then
      echo "  installing esp32 core..."
      arduino-cli config init 2>/dev/null || true
      arduino-cli config add board_manager.additional_urls \
        https://espressif.github.io/arduino-esp32/package_esp32_index.json
      arduino-cli core update-index
      arduino-cli core install esp32:esp32
    fi
  fi

  for lib in "ArduinoJson"; do
    if arduino-cli lib list 2>/dev/null | grep -qi "^$lib"; then
      ok "library $lib installed"
    else
      miss "library $lib not installed"
      [ "$CHECK_ONLY" -eq 0 ] && arduino-cli lib install "$lib"
    fi
  done
else
  fail "arduino-cli unavailable, skipping esp32 core/library checks"
fi

ESPTOOL=$(find_esptool)
if [ -n "$ESPTOOL" ]; then
  ok "esptool available ($ESPTOOL)"
else
  miss "esptool not found (installed with the esp32 core)"
fi

if [ -f secrets.h ]; then
  ok "secrets.h present"
else
  miss "secrets.h missing -- copy secrets.h.example and fill in WiFi details"
fi

echo
echo "=== connected boards ==="
shopt -s nullglob
PORTS=(/dev/cu.usbmodem* /dev/cu.usbserial* /dev/ttyACM* /dev/ttyUSB*)
shopt -u nullglob
if [ "${#PORTS[@]}" -eq 0 ]; then
  miss "no serial device found (looked for /dev/cu.usb*, /dev/ttyACM*, /dev/ttyUSB*)"
else
  for port in "${PORTS[@]}"; do
    echo "  $port:"
    if [ -n "$ESPTOOL" ]; then
      "$ESPTOOL" --port "$port" flash-id 2>&1 \
        | grep -E "Chip type|Chip is|Detected flash size" \
        | sed 's/^/    /' \
        || echo "    (flash-id failed -- board busy, wrong permissions, or not an ESP32?)"
    else
      echo "    (esptool not found, skipping flash-id)"
    fi
  done
fi

echo
[ "$CHECK_ONLY" -eq 1 ] && echo "check-only run: nothing was installed." || echo "done."
