import asyncio
import logging
from collections.abc import Sequence

import numpy as np
import websockets
from websockets.server import ServerConnection

from server import config
from server import protocol
from server.transcriber import Transcriber
from server import llm_handler
from server import tts_handler

log = logging.getLogger(__name__)

transcriber = Transcriber()


async def handle_audio(websocket: ServerConnection) -> None:
    log.info("ESP32 Connected")
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
                    log.info("Voice started")
                    awaiting_phrase = True
                    audio_buffer = []
                elif text == protocol.CMD_VOICE_END:
                    log.info("Voice ended (%d samples)", len(audio_buffer))
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
        log.warning("ESP32 Disconnected")
    except Exception as e:
        log.error("WebSocket error: %s", e)


async def _process_and_respond(
    audio_data: Sequence[float],
    websocket: ServerConnection,
) -> None:
    audio_np = np.array(audio_data, dtype=np.float32)

    text = transcriber.transcribe(audio_np)
    if not text:
        log.info("Audio detected but not clear speech.")
        return

    log.info("[Voice]: %s", text)
    response = llm_handler.ask(text)
    await tts_handler.generate_and_send(response, websocket)


async def main() -> None:
    async with websockets.serve(
        handle_audio, "0.0.0.0", config.WS_PORT, ping_timeout=None
    ):
        log.info("Server ready on port %d", config.WS_PORT)
        await asyncio.Future()


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
        datefmt="%H:%M:%S",
    )
    asyncio.run(main())
