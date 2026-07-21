import requests
from fastapi import APIRouter, HTTPException

from ..config import BOARD_IP, BOARD_PORT, BOARD_TIMEOUT

router = APIRouter(prefix="/api", tags=["status"])

# Must match the route table in firmware/BusySignal/BusySignal.ino
STATUSES = {
    "meeting":   {"group": "busy",      "label": "In a Meeting"},
    "call":      {"group": "busy",      "label": "On a Call"},
    "racing":    {"group": "busy",      "label": "Racing"},
    "recording": {"group": "busy",      "label": "Recording"},
    "working":   {"group": "available", "label": "Working, Available"},
    "comein":    {"group": "available", "label": "Come on In"},
}


def _board_url(path: str) -> str:
    return f"http://{BOARD_IP}:{BOARD_PORT}{path}"


def _call_board(path: str, params: dict | None = None) -> None:
    try:
        response = requests.get(_board_url(path), params=params, timeout=BOARD_TIMEOUT)
        response.raise_for_status()
    except requests.RequestException as exc:
        raise HTTPException(status_code=502, detail=f"Board unreachable: {exc}") from exc


@router.get("/statuses")
def list_statuses():
    return STATUSES


@router.post("/status/{key}")
def set_status(key: str):
    if key not in STATUSES:
        raise HTTPException(status_code=400, detail=f"Unknown status: {key}")
    _call_board(f"/{key}")
    return {"status": key, **STATUSES[key]}


@router.post("/message")
def send_message(text: str):
    if not text.strip():
        raise HTTPException(status_code=400, detail="Message text is empty")
    _call_board("/message", params={"text": text})
    return {"text": text}


@router.post("/message/clear")
def clear_message():
    _call_board("/message/clear")
    return {"cleared": True}


@router.post("/timer")
def start_timer(minutes: int, attached: bool = True):
    if not 0 < minutes <= 120:
        raise HTTPException(status_code=400, detail="minutes must be 1-120")
    _call_board("/timer", params={"minutes": minutes, "attached": "1" if attached else "0"})
    return {"minutes": minutes, "attached": attached}


@router.post("/timer/cancel")
def cancel_timer():
    _call_board("/timer/cancel")
    return {"cancelled": True}
