import os
import time
import logging
import tempfile

from groq import Groq

from server import config


log = logging.getLogger(__name__)

AUDIO_DIR = tempfile.mkdtemp()


def split_text(text: str, max_chars: int = 190) -> list[str]:
    palabras = text.split()
    fragmentos = []
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


async def generate_and_send(text: str, websocket) -> None:
    t = time.time()

    if config.TTS_LANG == "en":
        client = Groq(api_key=config.GROQ_API_KEY)
        fragmentos = split_text(text, max_chars=config.TTS_MAX_CHARS)
        urls = []
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
                urls.append(f"http://{config.SERVER_IP}:{config.HTTP_PORT}/{filename}")
            except Exception as e:
                log.error("Error Orpheus fragmento %d: %s", i, e)

        log.info("Latencia TTS: %.2fs", time.time() - t)
        if urls:
            await websocket.send(",".join(urls))
    else:
        log.info("Latencia total: %.2fs", time.time() - t)
        await websocket.send(text)
