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

**LIVE** (2026-07-20). Firmware flashed and verified on the physical board;
full pipeline (web UI → FastAPI → board HTTP → matrix) tested end-to-end.

- **Board**: on WiFi (GrabaBucket) at `192.168.68.74`. Routes: `/meeting`,
  `/call`, `/racing`, `/recording`, `/working`, `/comein`,
  `/message?text=`, `/message/clear`, `/timer?minutes=&attached=`,
  `/timer/cancel`. Self-heals WiFi drops (retries every 30s) and prints a
  network scan + connection diagnostics over serial at boot.
- **Server**: FastAPI on devbase1 port 3001, running as systemd service
  `busy-signal.service` (enabled at boot, restarts on failure; UFW allows
  3001). Manage with `sudo systemctl {status,restart} busy-signal`.
  Proxies to the board per `.env`.
- **UI**: `http://<devbase1>:3001/` — retro on-air themed control panel
  (status grid, message, timer with attach toggle), phone-friendly.

**Flashing** (arduino-cli, replaces the Arduino IDE flow):
```
arduino-cli compile --fqbn adafruit:samd:adafruit_matrixportal_m4 firmware/BusySignal
arduino-cli upload -p /dev/ttyACM0 --fqbn adafruit:samd:adafruit_matrixportal_m4 firmware/BusySignal
```

`192.168.68.74` is DHCP-reserved for the board on the Deco (set 2026-07-20),
so `BOARD_IP` in `.env` is stable.

**Why port 3001 is UFW-open:** the public vonholtencodes.com page is only a
PIN-gated placeholder — the real control UI is this server, and the open
port is what lets phones on the home LAN use it. LAN-only exposure (no
router port-forward to devbase1).

**Future:** a subpage on vonholtencodes-site — needs a bridge from starbase1
to this board's LAN IP (or relocating the FastAPI service), not designed yet.
The Mission Control card + PIN gate (003331) already exist on
vonholtencodes.com/pages/busy-signal/, currently a "coming soon" page.
