#!/usr/bin/env bash
# Build and flash the Busy Signal firmware.
#
#   ./tools/flash.sh usb [port]     first flash, or recovery - needs the cable
#   ./tools/flash.sh ota [ip]       every flash after that, over WiFi
#   ./tools/flash.sh build          compile only
#
# OTA needs firmware that already contains the OTA agent, so the very first
# upload of this firmware has to go over USB. After that the board listens on
# port 65280 and takes the image over the LAN.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SKETCH="$REPO/firmware/BusySignal"
FQBN="adafruit:samd:adafruit_matrixportal_m4"
BUILD="$REPO/build"
BIN="$BUILD/BusySignal.ino.bin"
BOARD_IP_DEFAULT="192.168.68.67"

# The OTA password lives in secrets.h, which is gitignored.
ota_password() {
  sed -n 's/^#define OTA_PASSWORD "\(.*\)"/\1/p' "$SKETCH/secrets.h" | head -1
}

build() {
  echo ">> compiling"
  arduino-cli compile --fqbn "$FQBN" --output-dir "$BUILD" "$SKETCH"
  local size
  size=$(stat -c%s "$BIN")
  echo ">> image: $size bytes"
  # InternalStorage stages the update in the top half of flash, so anything
  # over half of 507904 can never be delivered over the air.
  if [ "$size" -gt 253952 ]; then
    echo "!! too large for OTA (limit 253952 bytes) - USB only" >&2
  fi
}

case "${1:-build}" in
  build)
    build
    ;;

  usb)
    PORT="${2:-}"
    if [ -z "$PORT" ]; then
      # Pick the port whose matching_boards actually names our FQBN, rather
      # than grepping for ttyACM - the JSON is pretty-printed, and a serial
      # port being present is not the same as our board being on it.
      PORT=$(arduino-cli board list --format json 2>/dev/null | python3 -c '
import json, sys
fqbn = sys.argv[1]
try:
    ports = json.load(sys.stdin).get("detected_ports") or []
except Exception:
    sys.exit(0)
for p in ports:
    for b in p.get("matching_boards") or []:
        if b.get("fqbn") == fqbn:
            print(p["port"]["address"]); sys.exit(0)
' "$FQBN" || true)
    fi
    if [ -z "$PORT" ]; then
      echo "No board found. Plug the Matrix Portal in over USB." >&2
      echo "If it still is not seen, double-tap RESET to enter the bootloader." >&2
      exit 1
    fi
    build
    echo ">> uploading to $PORT"
    arduino-cli upload --fqbn "$FQBN" -p "$PORT" --input-dir "$BUILD" "$SKETCH"
    echo ">> done"
    ;;

  ota)
    IP="${2:-$BOARD_IP_DEFAULT}"
    PASS="$(ota_password)"
    if [ -z "$PASS" ]; then
      echo "No OTA_PASSWORD in $SKETCH/secrets.h" >&2
      exit 1
    fi
    build
    echo ">> uploading to $IP over WiFi"
    # The board's OTA agent wants a plain POST: basic auth, Content-Length,
    # raw image body. Expect: is suppressed because it never answers 100-continue.
    code=$(curl -sS --fail-with-body -o /tmp/ota-response.txt -w '%{http_code}' \
             --connect-timeout 5 --max-time 180 \
             -u "arduino:$PASS" -H "Expect:" \
             --data-binary "@$BIN" "http://$IP:65280/sketch") || {
      echo "!! upload failed (HTTP $code)" >&2
      cat /tmp/ota-response.txt >&2 || true
      echo "   If the sign is up but this times out, the board needs a USB flash first." >&2
      exit 1
    }
    echo ">> board accepted the image (HTTP $code); it reboots into it now"
    echo -n ">> waiting for it to come back"
    for _ in $(seq 1 40); do
      sleep 2
      if curl -s -m 2 "http://$IP/state" >/dev/null 2>&1; then
        echo
        echo ">> back up, showing: $(curl -s -m 2 "http://$IP/state")"
        exit 0
      fi
      echo -n .
    done
    echo
    echo "!! it did not answer within 80s - check the panel" >&2
    exit 1
    ;;

  *)
    sed -n '2,9p' "${BASH_SOURCE[0]}" | sed 's/^# \?//'
    exit 1
    ;;
esac
