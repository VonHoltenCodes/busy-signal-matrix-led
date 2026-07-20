import os

from dotenv import load_dotenv

load_dotenv()

BOARD_IP = os.getenv("BOARD_IP", "")
BOARD_PORT = int(os.getenv("BOARD_PORT", "80"))
BOARD_TIMEOUT = float(os.getenv("BOARD_TIMEOUT", "3"))
