import time
import logging

from websockets.server import ServerConnection

from server import config
from server import protocol

log = logging.getLogger(__name__)


def split_text(text: str, max_chars: int) -> list[str]:
    words = text.split()
    fragments: list[str] = []
    current = ""
    for word in words:
        if len(current) + len(word) + 1 <= max_chars:
            current += (" " if current else "") + word
        else:
            if current:
                fragments.append(current)
            current = word
    if current:
        fragments.append(current)
    return fragments if fragments else [text[:max_chars]]


async def generate_and_send(text: str, websocket: ServerConnection) -> None:
    t = time.time()

    fragments = split_text(text, max_chars=config.TTS_MAX_CHARS)
    for frag in fragments:
        msg = protocol.encode_text(f"{protocol.CMD_PLAY_TEXT}{frag}")
        await websocket.send(msg)

    log.info("Total latency: %.2fs", time.time() - t)
