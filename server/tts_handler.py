import os
import time
import logging
import tempfile
import http.server
import threading

from groq import Groq
from websockets.server import WebSocketServerProtocol

from server import config
from server import protocol

log = logging.getLogger(__name__)

AUDIO_DIR: str = tempfile.mkdtemp()
_http_iniciado: bool = False


class _SilentHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args: object, **kwargs: object) -> None:
        super().__init__(*args, directory=AUDIO_DIR, **kwargs)  # type: ignore[arg-type]

    def log_message(self, format: str, *args: object) -> None:
        pass


def _iniciar_http() -> None:
    global _http_iniciado
    if _http_iniciado:
        return
    threading.Thread(
        target=lambda: http.server.HTTPServer(
            ("0.0.0.0", config.HTTP_PORT), _SilentHandler
        ).serve_forever(),
        daemon=True,
    ).start()
    _http_iniciado = True
    log.info("Servidor HTTP de audio en :%d", config.HTTP_PORT)


def split_text(text: str, max_chars: int = 190) -> list[str]:
    palabras = text.split()
    fragmentos: list[str] = []
    actual = ""
    for palabra in palabras:
        if len(actual) + len(palabra) + 1 <= max_chars:
            actual += (" " if actual else "") + palabra
        else:
            if actual:
                fragmentos.append(actual)
            actual = palabra
    if actual:
        fragmentos.append(actual)
    return fragmentos if fragmentos else [text[:max_chars]]


async def generate_and_send(text: str, websocket: WebSocketServerProtocol) -> None:
    t = time.time()

    if config.TTS_LANG == "en":
        _iniciar_http()
        client = Groq(api_key=config.GROQ_API_KEY)
        fragmentos = split_text(text, max_chars=config.TTS_MAX_CHARS)
        urls: list[str] = []
        for i, frag in enumerate(fragmentos):
            try:
                response = client.audio.speech.create(
                    model="canopylabs/orpheus-v1-english",
                    voice=config.TTS_VOICE,
                    input=frag,
                    response_format="wav",
                )
                filename = f"resp_{i}.wav"
                path = os.path.join(AUDIO_DIR, filename)
                with open(path, "wb") as f:
                    f.write(response.content)
                urls.append(
                    f"http://{config.SERVER_IP}:{config.HTTP_PORT}/{filename}"
                )
            except Exception as e:
                log.error("Error Orpheus fragmento %d: %s", i, e)

        log.info("Latencia TTS: %.2fs", time.time() - t)
        for url in urls:
            msg = protocol.encode_text(f"PLAY_URL:{url}")
            await websocket.send(msg)
    else:
        msg = protocol.encode_text(f"{protocol.CMD_PLAY_TEXT}{text}")
        await websocket.send(msg)
        log.info("Latencia total: %.2fs", time.time() - t)
