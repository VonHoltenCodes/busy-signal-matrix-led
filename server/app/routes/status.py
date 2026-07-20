import requests
from fastapi import APIRouter, HTTPException

from ..config import BOARD_IP, BOARD_PORT, BOARD_TIMEOUT

router = APIRouter(prefix="/api", tags=["status"])

MODES = {"available", "busy", "meeting", "call"}


def _board_url(path: str) -> str:
    return f"http://{BOARD_IP}:{BOARD_PORT}{path}"


def _call_board(path: str, params: dict | None = None) -> None:
    try:
        response = requests.get(_board_url(path), params=params, timeout=BOARD_TIMEOUT)
        response.raise_for_status()
    except requests.RequestException as exc:
        raise HTTPException(status_code=502, detail=f"Board unreachable: {exc}") from exc


@router.post("/status/{mode}")
def set_status(mode: str):
    if mode not in MODES:
        raise HTTPException(status_code=400, detail=f"Unknown mode: {mode}")
    _call_board(f"/{mode}")
    return {"mode": mode}


@router.post("/message")
def send_message(text: str):
    _call_board("/message", params={"text": text})
    return {"text": text}


@router.post("/timer")
def start_timer(minutes: int):
    if minutes <= 0:
        raise HTTPException(status_code=400, detail="minutes must be positive")
    _call_board("/timer", params={"minutes": minutes})
    return {"minutes": minutes}
