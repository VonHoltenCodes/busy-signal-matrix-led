from pathlib import Path

from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles

from .routes import status

app = FastAPI(title="Busy Signal")
app.include_router(status.router)

# UI not built yet - pending a design discussion. The mount is wired up
# so static/index.html + friends can just be dropped in later.
STATIC_DIR = Path(__file__).parent / "static"
app.mount("/", StaticFiles(directory=STATIC_DIR, html=True), name="static")
