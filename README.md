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

Status: scaffolding only — implementation in progress.
