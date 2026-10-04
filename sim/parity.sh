#!/usr/bin/env bash
# Renders the same frames from the simulator and from a host build of the
# firmware's drawing code, and counts the pixels that differ.
#
#   ./parity.sh [frames] [key...]     default: 48 frames of every scene + lab
set -euo pipefail
SIM="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FW="$SIM/../firmware/BusySignal"
FRAMES="${1:-48}"; shift || true
KEYS=("$@")
[ ${#KEYS[@]} -eq 0 ] && KEYS=(gate welcome trex jeep blue egg logo birthday lab labmsg)
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

g++ -O1 -std=gnu++17 -I"$SIM/host" -I"$FW" -o "$WORK/hostsim" \
    "$SIM/host/main.cpp" "$FW/Gfx.cpp" "$FW/Scenes.cpp" "$FW/Lab.cpp"

for k in "${KEYS[@]}"; do
  node "$SIM/rawframes.js" "$k" "$FRAMES" "$WORK/js" >/dev/null
  mkdir -p "$WORK/cpp"
  "$WORK/hostsim" "$k" "$FRAMES" "$WORK/cpp"
done

python3 - "$WORK" "$FRAMES" "${KEYS[@]}" <<'PY'
import sys, os
work, frames, keys = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
def load(p):
    b = open(p, 'rb').read()
    return b[len(b) - 64 * 32 * 3:]
total = bad = worst = 0
for k in keys:
    n = mx = 0
    for f in range(frames):
        name = f"{k}_{f:02d}.ppm"
        a, b = load(os.path.join(work, 'js', name)), load(os.path.join(work, 'cpp', name))
        for i in range(0, len(a), 3):
            d = max(abs(a[i + c] - b[i + c]) for c in range(3))
            if d:
                n += 1
                mx = max(mx, d)
    total += frames * 64 * 32
    bad += n
    worst = max(worst, mx)
    print(f"{k:10s} {n:6d} px differ  (largest channel step {mx})")
print(f"{'all':10s} {bad:6d} of {total} px")
PY
