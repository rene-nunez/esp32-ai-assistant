import os
import time
import logging
import tempfile
import http.server
import threading

from websockets.server import WebSocketServerProtocol

from server import config
from server import protocol
from server.client import groq

log = logging.getLogger(__name__)

_audio_dir: str | None = None
_http_started: bool = False


class _SilentHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args: object, **kwargs: object) -> None:
        super().__init__(*args, directory=_audio_dir, **kwargs)  # type: ignore[arg-type]

    def log_message(self, format: str, *args: object) -> None:
        pass


def _ensure_http() -> None:
    global _http_started, _audio_dir
    if _http_started:
        return
    _audio_dir = tempfile.mkdtemp()
    threading.Thread(
        target=lambda: http.server.HTTPServer(
            ("0.0.0.0", config.HTTP_PORT), _SilentHandler
        ).serve_forever(),
        daemon=True,
    ).start()
    _http_started = True
    log.info("HTTP audio server on :%d", config.HTTP_PORT)


def split_text(text: str, max_chars: int = 190) -> list[str]:
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


async def generate_and_send(text: str, websocket: WebSocketServerProtocol) -> None:
    t = time.time()

    if config.TTS_LANG == "en":
        _ensure_http()
        fragments = split_text(text, max_chars=config.TTS_MAX_CHARS)
        urls: list[str] = []
        for i, frag in enumerate(fragments):
            try:
                response = groq.audio.speech.create(
                    model="canopylabs/orpheus-v1-english",
                    voice=config.TTS_VOICE,
                    input=frag,
                    response_format="wav",
                )
                filename = f"resp_{i}.wav"
                path = os.path.join(_audio_dir, filename)
                with open(path, "wb") as f:
                    f.write(response.content)
                urls.append(
                    f"http://{config.SERVER_IP}:{config.HTTP_PORT}/{filename}"
                )
            except Exception as e:
                log.error("Orpheus fragment %d error: %s", i, e)

        log.info("TTS latency: %.2fs", time.time() - t)

        if not urls:
            msg = protocol.encode_text(f"{protocol.CMD_PLAY_TEXT}I'm sorry, please try again.")
            await websocket.send(msg)
        else:
            for url in urls:
                msg = protocol.encode_text(f"PLAY_URL:{url}")
                await websocket.send(msg)
    else:
        fragments = split_text(text, max_chars=config.TTS_MAX_CHARS)
        for frag in fragments:
            msg = protocol.encode_text(f"{protocol.CMD_PLAY_TEXT}{frag}")
            await websocket.send(msg)
        log.info("Total latency: %.2fs", time.time() - t)
