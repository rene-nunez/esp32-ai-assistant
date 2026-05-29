import asyncio
import http.server
import logging
import threading

import numpy as np
import websockets

from server import config
from server.transcriber import Transcriber
from server import llm_handler
from server import tts_handler

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger(__name__)

transcriber = Transcriber()
transcriber.ensure_loaded()

# Servidor HTTP auxiliar (se eliminará en Commit 8 con protocolo v2)


class SilentHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=tts_handler.AUDIO_DIR, **kwargs)

    def log_message(self, format, *args):
        pass


threading.Thread(
    target=lambda: http.server.HTTPServer(
        ("0.0.0.0", config.HTTP_PORT), SilentHandler
    ).serve_forever(),
    daemon=True,
).start()
log.info("Servidor HTTP de audio en :%d", config.HTTP_PORT)


async def handle_audio(websocket):
    log.info("ESP32 Conectado")
    audio_buffer = []

    try:
        while True:
            try:
                message = await asyncio.wait_for(websocket.recv(), timeout=0.5)
                samples = np.frombuffer(message, dtype=np.int16)
                audio_buffer.extend(samples.astype(np.float32) / 32768.0)

                if len(audio_buffer) > config.AUDIO_RATE * config.AUDIO_CHUNK_SECONDS:
                    await _process_and_respond(audio_buffer, websocket)
                    audio_buffer = []

            except asyncio.TimeoutError:
                if len(audio_buffer) > config.AUDIO_MIN_SILENCE_SAMPLES:
                    log.info("Analizando frase...")
                    await _process_and_respond(audio_buffer, websocket)
                    audio_buffer = []
                continue

    except websockets.exceptions.ConnectionClosed:
        log.warning("ESP32 Desconectado")
    except Exception as e:
        log.error("Error en WebSocket: %s", e)


async def _process_and_respond(audio_data, websocket):
    audio_np = np.array(audio_data)

    texto = transcriber.transcribe(audio_np)
    if not texto:
        log.info("Sonido detectado pero no es una frase clara.")
        return

    log.info("[Voz]: %s", texto)
    respuesta = llm_handler.ask(texto)
    await tts_handler.generate_and_send(respuesta, websocket)


async def main():
    modo_tts = "Orpheus (inglés)" if config.TTS_LANG == "en" else "Google TTS (español)"
    async with websockets.serve(
        handle_audio, "0.0.0.0", config.WS_PORT, ping_timeout=None
    ):
        log.info("Servidor listo en puerto %d. Modo TTS: %s", config.WS_PORT, modo_tts)
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
