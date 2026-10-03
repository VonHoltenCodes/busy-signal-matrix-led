# Busy Signal — Matrix Portal M4 status board

A red/green busy/available status board for the office door, built on an
Adafruit Matrix Portal M4 + 64x32 RGB HUB75 LED matrix (same hardware as
[Rocket_Launch](https://github.com/VonHoltenCodes/Rocket_Launch)). The board
joins home WiFi via its onboard ESP32 co-processor and is controlled from a
PIN-gated page on vonholtencodes.com.

Statuses: In a Meeting, On a Call, Racing, Recording (busy/red), Working
Available, Come on In (available/green) — plus a custom scrolling message and
a countdown timer (numeric + a depleting pepperoni pizza, always shown
together). Retro on-air sign look: solid color slide alternating with an
icon+label status slide every 5s, amber bulb-dot border throughout.

![MEETING slide glowing above the office door](screenshots/meeting-above-door.jpg)

| | |
|---|---|
| ![Come on In slide: doorway + arrow icon](screenshots/come-in-slide.jpg) | ![Pizza timer: 8 slices vanish as time runs out](screenshots/pizza-timer-closeup.jpg) |

![Pizza timer running above the door](screenshots/pizza-timer-above-door.jpg)

Alongside the everyday status board there is a **scene engine**: full-frame
animated shows selected from the same control panel. The first set is a
Jurassic Park playlist built for Nolan's sixth birthday — the park gates
swinging open, a T-Rex roaring at sunset, Blue, Jeep 10 with something big
following it, a hatching egg, the park emblem, and a happy-birthday finale.

## Architecture

```
Browser (anywhere, HTTPS)
  └─> vonholtencodes.com/pages/busy-signal/   (Apache+PHP on starbase1)
        auth.php   PIN gate (deploy-only, not in this repo)
        index.php  control panel UI            <- website/
        api.php    whitelisted proxy           <- website/
          └─> http://<board LAN IP>/...        (server-side, same LAN)
                └─> Matrix Portal M4 firmware  <- firmware/
```

The board's own HTTP API is plain, unauthenticated LAN HTTP — the PHP layer
on the always-on web server is the security boundary and the only public
entry point. No port needs to be open on any desktop machine.

## Layout

```
firmware/BusySignal/   Arduino sketch for the Matrix Portal M4
  Gfx.{h,cpp}          Software framebuffer + drawing primitives
  Scenes.{h,cpp}       The animated scenes
  Font5x7.h            Adafruit_GFX's glyphs, copied so the sim can match
sim/                   Browser + headless simulator for the same scenes
tools/flash.sh         Build and flash, over USB or over WiFi
website/               Control panel + proxy, deployed to the web server
server/                Optional local FastAPI app (dev/testing only)
```

## Scenes

A scene is a pure function of elapsed seconds drawing into a 64x32 RGB
framebuffer (`Gfx.cpp`), which is blitted to the panel once per frame. Being
pure means the board can be dropped into any scene at any moment with no
state to restore, and the simulator can scrub anywhere in the timeline.

| Route | Scene | Length |
|---|---|---|
| `/scene?name=gate` | The park gates open on a lit paddock | 9s |
| `/scene?name=welcome` | Welcome sign, typed out | 6s |
| `/scene?name=trex` | T-Rex roaring against a sunset | 9s |
| `/scene?name=jeep` | Jeep 10 on the park road, being followed | 7s |
| `/scene?name=blue` | Blue, in close-up | 7s |
| `/scene?name=egg` | An egg hatching | 8s |
| `/scene?name=logo` | The park emblem | 6s |
| `/scene?name=birthday` | Happy birthday finale | 11s |
| `/scene?name=party` | All eight on a loop | 63s |
| `/scene/stop` | Back to the status board | |

A scene holds until a status route is called or `/scene/stop` is hit.
`/state` reports what is showing (`scene:trex`, `status:meeting`, ...).

### Simulator

Designing pixel art by flashing the board is unworkable, so `sim/` runs the
**same scene source** in a browser and headless:

```
cd sim
node render.js all 8 6              # contact sheet per scene -> /tmp/jpshots
TIMES=0.5,2.8,5.0 node render.js trex 3 14   # specific moments, zoomed
node rawframes.js trex 12 /tmp/js   # unscaled frames, for parity checks
```

`sim/scenes.js` and `firmware/BusySignal/Scenes.cpp` are line-for-line ports
of each other and must be changed together. To prove they still agree, render
the same frames from both and diff them — a host build of the firmware's own
C++ is the reference:

```
g++ -O1 -std=gnu++17 -I. -o hostsim main.cpp Gfx.cpp Scenes.cpp
```

Across 96 sampled frames the two differ by 76 pixels of 196,608, all of them
single-step alpha-blend rounding that 4-bit panel quantisation mostly erases.
Two gotchas that caused real divergence and will again: colour channels must
**clamp**, never mask (`0xFF &` wraps a scaled highlight into a new hue), and
a JS loop starting at a fractional coordinate must stay fractional in C++ —
an `int` loop lands on different pixel centres and shifts a whole glow.

## Setup

### Firmware

1. arduino-cli (or Arduino IDE) with the Adafruit board index, plus
   `Adafruit Protomatter`, `Adafruit GFX Library`, and `WiFiNINA`.
   Note: upstream WiFiNINA works as-is — the matrixportal_m4 variant defines
   the NINA pins at compile time; do not call `WiFi.setPins()` (that's an
   Adafruit-fork-only API).
2. `cp firmware/BusySignal/secrets.h.example firmware/BusySignal/secrets.h`
   and fill in your WiFi SSID/password (gitignored).
   Also set `OTA_PASSWORD`, which guards wireless upload.
3. Flash over USB and read the IP from serial (9600 baud — the sketch prints
   a network scan and connection diagnostics at boot):
   ```
   ./tools/flash.sh usb
   ```
4. Give the board a DHCP reservation so its IP can't drift.

Firmware HTTP routes: `/meeting`, `/call`, `/racing`, `/recording`,
`/working`, `/comein`, `/message?text=`, `/message/clear`,
`/timer?minutes=&attached=`, `/timer/cancel`, `/scene?name=`, `/scene/stop`,
`/state`. WiFi self-heals (reconnect retry every 30s).

### Flashing over WiFi

The firmware carries an OTA agent on port 65280, so the board only has to
come off the wall once. After the first USB flash:

```
./tools/flash.sh ota            # defaults to 192.168.68.74
./tools/flash.sh ota <board-ip>
```

The board stages the incoming image in the top half of its flash and copies
it down on reset, which caps a wirelessly-deliverable sketch at **253,952
bytes**; `flash.sh` warns if a build crosses that. The panel shows UPDATING
while the transfer runs — rendering is blocked until the board reboots.

The protocol is a plain authenticated POST, so nothing beyond curl is needed:

```
curl -u arduino:<OTA_PASSWORD> -H "Expect:" \
     --data-binary @build/BusySignal.ino.bin \
     http://<board-ip>:65280/sketch
```

### Website

Deploy `website/*` to the gated directory on the web server and set
`BOARD_BASE` in `api.php` to the board's LAN IP. The PIN gate (`auth.php`,
bcrypt PIN hash + PHP session) and `logout.php` are deploy-only — kept out
of this repo so the hash never goes public.

### Local dev server (optional)

A standalone FastAPI equivalent of the website layer, useful when hacking on
firmware without touching production:

```
cd server
pip install -r requirements.txt
cp .env.example .env   # set BOARD_IP
uvicorn app.main:app --host 0.0.0.0 --port 3001
```

## Status

**LIVE** (2026-07-20). Firmware flashed and verified on hardware; the
website control panel drives the physical sign end-to-end over HTTPS
(PIN login → api.php → board), tested against every route. The previous
devbase1-hosted control path (systemd service + open port 3001) is
decommissioned — the unit file remains for optional local dev, disabled.
