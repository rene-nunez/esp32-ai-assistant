import asyncio
import logging
import time
from collections.abc import Sequence

import numpy as np
import websockets
from websockets.server import ServerConnection

from server import config
from server import protocol
from server.transcriber import Transcriber
from server import llm_handler
from server import tts_handler
from server import device_tts

log = logging.getLogger("sys")

_SAMPLE_RATE_HZ = 16000

_NO_SPEECH_MSG = {
    "en": "Sorry, I didn't catch that. Please repeat.",
    "es": "Lo siento, no capté eso. Repite, por favor.",
}

transcriber = Transcriber()

async def handle_audio(websocket: ServerConnection) -> None: # pipeline: STT LLM TTS (sequential by data dependency)
    log.info("connected")
    await websocket.send(f"LANG:{config.LANGUAGE}")
    audio_buffer: list[float] = []
    awaiting_phrase = False

    try:
        async for message in websocket:
            if not message:
                continue

            msg_type, payload = protocol.decode(message)

            if msg_type == protocol.MessageType.TEXT:
                text = payload.decode("utf-8")
                if text == protocol.CMD_VOICE_START:
                    log.info("voice start")
                    awaiting_phrase = True
                    audio_buffer = []
                elif text == protocol.CMD_VOICE_END:
                    log.info("voice end (%.1fs)", len(audio_buffer) / _SAMPLE_RATE_HZ)
                    awaiting_phrase = False
                    if audio_buffer:
                        await _process_and_respond(audio_buffer, websocket)
                    audio_buffer = []

            elif msg_type == protocol.MessageType.AUDIO:
                if awaiting_phrase:
                    samples = (
                        np.frombuffer(payload, dtype=np.int16).astype(np.float32)
                        / 32768.0
                    )
                    audio_buffer.extend(samples)

    except websockets.exceptions.ConnectionClosed:
        log.warning("disconnected")
    except Exception as e:
        log.error("WebSocket error: %s", e)

async def _process_and_respond(
    audio_data: Sequence[float],
    websocket: ServerConnection,
) -> None:
    await websocket.send("[log] [ai] thinking...")

    audio_np = np.array(audio_data, dtype=np.float32)

    start = time.time()
    text = transcriber.transcribe(audio_np)
    stt_time = time.time() - start

    if not text:
        log.info("[you] (no speech detected)")
        await websocket.send("[log] [you] (no speech detected)")
        await websocket.send(_NO_SPEECH_MSG[config.LANGUAGE])
        return

    log.info("[you] %s (%.1fs)", text, stt_time)
    await websocket.send(f"[log] [you] {text}")

    start = time.time()
    response = llm_handler.ask(text)
    llm_time = time.time() - start

    log.info("[ai] %s (%.1fs)", response, llm_time)
    await websocket.send(f"[log] [ai] {response}")

    if config.PLAYBACK_TARGET == "device":
        if not await device_tts.play(response):
            log.info("[tts] esp32 fallback")
            await websocket.send("[log] [tts] esp32 fallback")
            await tts_handler.generate_and_send(response, websocket)
        else:
            log.info("[tts] ok")
            await websocket.send("[log] [tts] ok")

class _NoInfoFormatter(logging.Formatter):
    """INFO lines drop the badge except on tts/stt; WARNING/ERROR keep [LEVEL]."""

    _plain_sys = "%(asctime)s %(message)s"
    _plain_tag = "%(asctime)s [%(name)s] %(message)s"
    _leveled_sys = "%(asctime)s [%(levelname)s] %(message)s"
    _leveled_tag = "%(asctime)s [%(name)s] [%(levelname)s] %(message)s"

    def format(self, record: logging.LogRecord) -> str:
        if record.name == "sys":
            self._style._fmt = (
                self._leveled_sys if record.levelno >= logging.WARNING else self._plain_sys
            )
        else:
            self._style._fmt = (
                self._leveled_tag if record.levelno >= logging.WARNING else self._plain_tag
            )
        return super().format(record)

async def main() -> None:
    async with websockets.serve(
        handle_audio, "0.0.0.0", config.WS_PORT, ping_timeout=None # ESP32 can be silent minutes when idle
    ):
        log.info("server on 0.0.0.0:%d", config.WS_PORT)
        log.info(
            "playback %s (%s) | whisper %s/%s/%s | llm %s | lang %s",
            config.PLAYBACK_TARGET,
            device_tts.player_name(),
            config.WHISPER_MODEL,
            config.WHISPER_DEVICE,
            config.WHISPER_COMPUTE_TYPE,
            config.LLM_MODEL,
            config.LANGUAGE,
        )
        await asyncio.Future()

if __name__ == "__main__":
    handler = logging.StreamHandler()
    handler.setFormatter(_NoInfoFormatter(datefmt="%H:%M:%S"))
    logging.basicConfig(
        level=logging.INFO,
        handlers=[handler],
    )
    asyncio.run(main())