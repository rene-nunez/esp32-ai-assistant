import asyncio
import http.server
import logging
import threading

import numpy as np
import websockets

from server import config
from server import protocol
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

# Servidor HTTP auxiliar (se eliminará en Commit 8 con el HTTP cleanup)


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
    esperando_frase = False

    try:
        async for message in websocket:
            if not message:
                continue

            msg_type, payload = protocol.decode(message)

            if msg_type == protocol.MessageType.TEXT:
                text = payload.decode("utf-8")
                if text == protocol.CMD_VOICE_START:
                    log.info("Inicio de frase")
                    esperando_frase = True
                    audio_buffer = []
                elif text == protocol.CMD_VOICE_END:
                    log.info("Fin de frase (%d samples)", len(audio_buffer))
                    esperando_frase = False
                    if audio_buffer:
                        await _process_and_respond(audio_buffer, websocket)
                    audio_buffer = []

            elif msg_type == protocol.MessageType.AUDIO:
                if esperando_frase:
                    samples = (
                        np.frombuffer(payload, dtype=np.int16).astype(np.float32)
                        / 32768.0
                    )
                    audio_buffer.extend(samples)

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
