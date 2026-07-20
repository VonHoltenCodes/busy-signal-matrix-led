# Busy Signal — Matrix Portal M4 status board

A red/green busy/available status board for the office door, built on an
Adafruit Matrix Portal M4 + 64x32 RGB HUB75 LED matrix (same hardware as
[Rocket_Launch](https://github.com/VonHoltenCodes/Rocket_Launch)), controlled
over WiFi via the board's onboard ESP32 co-processor from a local FastAPI
control app.

Modes: Available (green), Busy / In a Meeting / On a Call (red + label),
custom message, and a countdown timer.

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

Working: WiFi connection, embedded HTTP command parsing
(`/available`, `/busy`, `/meeting`, `/call`, `/message`, `/timer`), and the
FastAPI backend that proxies to it. Solid green/red fills only for now.

**Not yet designed:** the actual on-matrix visuals for meeting/call/message/
timer, and the web control UI's look — deliberately left as placeholders
pending a design discussion.

**Future:** a subpage on vonholtencodes-site — needs a bridge from starbase1
to this board's LAN IP (or relocating the FastAPI service), not designed yet.
