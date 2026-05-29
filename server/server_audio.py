import asyncio
import websockets
import numpy as np
from faster_whisper import WhisperModel
from groq import Groq
import time
import http.server
import threading
import os
import tempfile
import logging

from server import config

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S"
)
log = logging.getLogger(__name__)

log.info("Cargando modelo de IA...")
model = WhisperModel(
    config.WHISPER_MODEL,
    device=config.WHISPER_DEVICE,
    compute_type=config.WHISPER_COMPUTE_TYPE
)

groq_client = Groq(api_key=config.GROQ_API_KEY)

# ── Servidor HTTP local para servir WAV/MP3 al ESP32 ──────────
AUDIO_DIR = tempfile.mkdtemp()

class SilentHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=AUDIO_DIR, **kwargs)
    def log_message(self, format, *args):
        pass

threading.Thread(
    target=lambda: http.server.HTTPServer(("0.0.0.0", config.HTTP_PORT), SilentHandler).serve_forever(),
    daemon=True
).start()
log.info("Servidor HTTP de audio en :%d", config.HTTP_PORT)

# ─────────────────────────────────────────────────────────────

async def handle_audio(websocket):
    log.info("¡ESP32 Conectado!")
    audio_buffer = []

    try:
        while True:
            try:
                message = await asyncio.wait_for(websocket.recv(), timeout=0.5)
                samples = np.frombuffer(message, dtype=np.int16)
                audio_buffer.extend(samples.astype(np.float32) / 32768.0)

                if len(audio_buffer) > config.AUDIO_RATE * config.AUDIO_CHUNK_SECONDS:
                    await process_audio(audio_buffer, websocket)
                    audio_buffer = []

            except asyncio.TimeoutError:
                if len(audio_buffer) > config.AUDIO_MIN_SILENCE_SAMPLES:
                    log.info("Analizando frase...")
                    await process_audio(audio_buffer, websocket)
                    audio_buffer = []
                continue

    except websockets.exceptions.ConnectionClosed:
        log.warning("ESP32 Desconectado.")
    except Exception as e:
        log.error("Error en WebSocket: %s", e)

async def process_audio(audio_data, websocket):
    audio_np = np.array(audio_data)
    volumen = np.max(np.abs(audio_np))
    log.debug("Nivel de audio máximo: %.4f", volumen)

    if volumen < config.VOLUME_MIN_THRESHOLD:
        log.info("Ruido residual ignorado.")
        return

    audio_normalizado = audio_np / volumen if volumen > 0 else audio_np

    segments, _ = model.transcribe(
        audio_normalizado,
        beam_size=5,
        language="es",
        vad_filter=True,
        vad_parameters=dict(
            min_silence_duration_ms=config.VAD_MIN_SILENCE_MS,
            threshold=config.VAD_THRESHOLD,
            min_speech_duration_ms=config.VAD_MIN_SPEECH_MS
        ),
        no_speech_threshold=config.NO_SPEECH_THRESHOLD,
        condition_on_previous_text=False
    )

    texto_detectado = ""
    for segment in segments:
        if segment.no_speech_prob < 0.5 and segment.text.strip():
            texto_detectado += segment.text + " "

    texto_limpio = texto_detectado.strip()

    if len(texto_limpio) > 2:
        log.info("[Voz]: %s", texto_limpio)
        await get_res(texto_limpio, websocket)
    else:
        log.info("Sonido detectado, pero no parece ser una frase clara.")

async def get_res(message, websocket):
    inicio = time.time()
    try:
        log.info("Preguntándole al modelo...")
        chat_completion = groq_client.chat.completions.create(
            messages=[config.SYSTEM_PROMPT, {"role": "user", "content": message}],
            model=config.LLM_MODEL,
            temperature=config.LLM_TEMPERATURE,
            max_tokens=config.LLM_MAX_TOKENS
        )
        respuesta = chat_completion.choices[0].message.content.strip()
        log.info("IA: %s", respuesta)
        log.info("Latencia LLM: %.2fs", time.time() - inicio)

        await generar_y_enviar_audio(respuesta, websocket)

    except Exception as error:
        log.error("Error LLM: %s", error)

async def generar_y_enviar_audio(texto, websocket):
    t = time.time()

    if config.TTS_LANG == "en":
        fragmentos = partir_texto(texto, max_chars=config.TTS_MAX_CHARS)
        urls = []
        for i, frag in enumerate(fragmentos):
            try:
                response = groq_client.audio.speech.create(
                    model="canopylabs/orpheus-v1-english",
                    voice=config.TTS_VOICE,
                    input=frag,
                    response_format="wav"
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
        await websocket.send(texto)

def partir_texto(texto, max_chars=190):
    """Divide el texto en fragmentos de max_chars respetando palabras."""
    palabras = texto.split()
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
    return fragmentos if fragmentos else [texto[:max_chars]]

async def main():
    modo_tts = "Orpheus (inglés)" if config.TTS_LANG == "en" else "Google TTS (español)"
    async with websockets.serve(handle_audio, "0.0.0.0", config.WS_PORT, ping_timeout=None):
        log.info("Servidor listo en puerto %d. Modo TTS: %s", config.WS_PORT, modo_tts)
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())