import asyncio
import io
import logging
import os
import shutil
import subprocess
import tempfile
import threading

import edge_tts

from server import config

log = logging.getLogger("tts")

_PLAYERS = [
    ("mpv", ["mpv", "--no-terminal", "--volume=100"]),
    ("ffplay", ["ffplay", "-nodisp", "-autoexit", "-loglevel", "quiet"]),
]

RETRIES = 2
RETRY_DELAY_S = 1.0


def _available_player() -> tuple[str, list[str]] | None:
    for name, cmd in _PLAYERS:
        if shutil.which(name):
            return name, list(cmd)
    return None


def player_name() -> str:
    player = _available_player()
    return player[0] if player else "none"


async def _synthesize(text: str) -> bytes:
    last_exc: BaseException | None = None
    for attempt in range(RETRIES + 1):
        try:
            audio = io.BytesIO()
            communicate = edge_tts.Communicate(
                text, config.TTS_SERVER_VOICE, rate=config.TTS_SERVER_RATE
            )
            async for chunk in communicate.stream():
                if chunk["type"] == "audio":
                    audio.write(chunk["data"])

            if audio.tell() == 0:
                last_exc = RuntimeError("edge-tts produced no audio")
            else:
                return audio.getvalue()
        except Exception as e:
            last_exc = e

        if attempt < RETRIES:
            await asyncio.sleep(RETRY_DELAY_S)

    raise last_exc


def _wait_and_cleanup(proc: subprocess.Popen, path: str) -> None:
    try:
        proc.wait()
    finally:
        try:
            os.remove(path)
        except OSError:
            pass


async def play(text: str) -> bool:
    player = _available_player()
    if player is None:
        log.warning("no audio player found (need mpv or ffplay)")
        return False

    try:
        audio_data = await _synthesize(text)
    except Exception as e:
        log.warning("playback failed, esp32 fallback: %s", e)
        return False

    try:
        name, cmd = player
        fd, path = tempfile.mkstemp(prefix="tts_", suffix=".mp3")
        try:
            with os.fdopen(fd, "wb") as f:
                f.write(audio_data)
            proc = subprocess.Popen(
                cmd + [path],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
        except BaseException:
            try:
                os.remove(path)
            except OSError:
                pass
            raise

        threading.Thread(
            target=_wait_and_cleanup, args=(proc, path), daemon=True
        ).start()

        log.debug("played (%s)", name)
        return True

    except Exception as e:
        log.warning("playback failed, esp32 fallback: %s", e)
        return False