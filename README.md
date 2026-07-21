# Busy Signal — Matrix Portal M4 status board

A red/green busy/available status board for the office door, built on an
Adafruit Matrix Portal M4 + 64x32 RGB HUB75 LED matrix (same hardware as
[Rocket_Launch](https://github.com/VonHoltenCodes/Rocket_Launch)), controlled
over WiFi via the board's onboard ESP32 co-processor from a local FastAPI
control app.

Statuses: In a Meeting, On a Call, Racing, Recording (busy/red), Working
Available, Come on In (available/green) — plus a custom scrolling message and
a countdown timer (numeric + a depleting pepperoni pizza, always shown
together). Retro on-air sign look: wig-free solid color slide alternating
with an icon+label status slide every 5s, amber bulb-dot border throughout.

See `/home/devbase1/.claude/plans/snuggly-wishing-hennessy.md` for the full
implementation plan (firmware + server design).

## Layout

```
firmware/BusySignal/   Arduino sketch for the Matrix Portal M4
server/                Local FastAPI control app + web UI
```

## Setup

### Firmware

1. Arduino IDE, "Adafruit Matrix Portal M4" board support + `Adafruit_Protomatter`,
   `Adafruit_GFX`, and `WiFiNINA` libraries (same base setup as Rocket_Launch,
   plus WiFiNINA).
2. `cp firmware/BusySignal/secrets.h.example firmware/BusySignal/secrets.h`
   and fill in your WiFi SSID/password (gitignored).
3. Upload `BusySignal.ino`. Open the Serial Monitor to read the board's
   DHCP-assigned IP once it connects.

### Server

```
cd server
pip install --break-system-packages -r requirements.txt
cp .env.example .env   # set BOARD_IP to the IP printed above
uvicorn app.main:app --host 0.0.0.0 --port 3001
```

## Status

**Firmware: fully implemented**, design validated in an interactive browser
simulator before porting. WiFi connection, embedded HTTP command parsing, and
full rendering for every mode: the wig-free color slide, six status icons
(meeting/call/racing/recording/working/come-in), the scrolling message
marquee, and the pizza+countdown timer (attached to a status's cycle, or
standalone full-screen). Routes: `/meeting`, `/call`, `/racing`,
`/recording`, `/working`, `/comein`, `/message?text=`, `/message/clear`,
`/timer?minutes=&attached=`, `/timer/cancel`.

**Not yet done:**
- The FastAPI backend (`server/app/routes/status.py`) still targets the old
  route set (`/available`, `/busy`, generic `meeting`/`call` only) from
  before the status/icon design was finalized — needs updating to the six
  real statuses and the message/timer routes above before the server can
  drive the board.
- The web control UI (`server/app/static/`) — themed to match the retro
  on-air look, not built yet.
- Firmware is untested on real hardware — needs Arduino IDE upload + Serial
  Monitor verification (no compiler for the Matrix Portal M4 core is
  installed on this machine yet, so this was reviewed but not compiled).

**Future:** a subpage on vonholtencodes-site — needs a bridge from starbase1
to this board's LAN IP (or relocating the FastAPI service), not designed yet.
