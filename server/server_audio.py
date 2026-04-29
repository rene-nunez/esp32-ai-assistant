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

model_size = "base"
print("Cargando modelo de IA...")
model = WhisperModel(model_size, device="cpu", compute_type="int8")

GROQ_API_KEY = "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"
groq_client  = Groq(api_key=GROQ_API_KEY)

# ── Configuración ─────────────────────────────────────────────
SERVER_IP  = "192.168.0.5"   # IP de la laptop en la red local
HTTP_PORT  = 8766
IDIOMA_TTS = "es"            # "es" = Google TTS español, "en" = Orpheus inglés

system_prompt = {
    "role": "system",
    "content": "Eres un asistente de voz inteligente conectado a un ESP32, tus respuestas deben ser muy breves y útiles. Máximo 2 oraciones."
}

# ── Servidor HTTP local para servir WAV/MP3 al ESP32 ──────────
AUDIO_DIR = tempfile.mkdtemp()

class SilentHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=AUDIO_DIR, **kwargs)
    def log_message(self, format, *args):
        pass

threading.Thread(
    target=lambda: http.server.HTTPServer(("0.0.0.0", HTTP_PORT), SilentHandler).serve_forever(),
    daemon=True
).start()
print(f"Servidor HTTP de audio en :{HTTP_PORT}")

# ─────────────────────────────────────────────────────────────

async def handle_audio(websocket):
    print("¡ESP32 Conectado!")
    audio_buffer = []

    try:
        while True:
            try:
                message = await asyncio.wait_for(websocket.recv(), timeout=0.5)
                samples = np.frombuffer(message, dtype=np.int16)
                audio_buffer.extend(samples.astype(np.float32) / 32768.0)

                if len(audio_buffer) > 16000 * 7:
                    await process_audio(audio_buffer, websocket)
                    audio_buffer = []

            except asyncio.TimeoutError:
                if len(audio_buffer) > 8000:
                    print("Analizando frase...")
                    await process_audio(audio_buffer, websocket)
                    audio_buffer = []
                continue

    except websockets.exceptions.ConnectionClosed:
        print("ESP32 Desconectado.")
    except Exception as e:
        print(f"Error: {e}")

async def process_audio(audio_data, websocket):
    audio_np = np.array(audio_data)
    volumen = np.max(np.abs(audio_np))
    print(f"Nivel de audio máximo: {volumen:.4f}")

    if volumen < 0.01:
        print("Ruido residual ignorado.")
        return

    audio_normalizado = audio_np / volumen if volumen > 0 else audio_np

    segments, _ = model.transcribe(
        audio_normalizado,
        beam_size=5,
        language="es",
        vad_filter=True,
        vad_parameters=dict(
            min_silence_duration_ms=800,
            threshold=0.6,
            min_speech_duration_ms=250
        ),
        no_speech_threshold=0.7,
        condition_on_previous_text=False
    )

    texto_detectado = ""
    for segment in segments:
        if segment.no_speech_prob < 0.5 and segment.text.strip():
            texto_detectado += segment.text + " "

    texto_limpio = texto_detectado.strip()

    if len(texto_limpio) > 2:
        print(f"[Voz]: {texto_limpio}")
        await get_res(texto_limpio, websocket)
    else:
        print("Sonido detectado, pero no parece ser una frase clara.")

async def get_res(message, websocket):
    inicio = time.time()
    try:
        print("Preguntándole al modelo...")
        chat_completion = groq_client.chat.completions.create(
            messages=[system_prompt, {"role": "user", "content": message}],
            model="llama-3.1-8b-instant",
            temperature=0.7,
            max_tokens=100  # respuestas cortas para cumplir límite de 200 chars en TTS
        )
        respuesta = chat_completion.choices[0].message.content.strip()
        print(f"IA: {respuesta}")
        print(f"Latencia LLM: {time.time() - inicio:.2f}s")

        await generar_y_enviar_audio(respuesta, websocket)

    except Exception as error:
        print(f"Error LLM: {error}")

async def generar_y_enviar_audio(texto, websocket):
    t = time.time()

    if IDIOMA_TTS == "en":
        # ── Orpheus (inglés, alta calidad, WAV) ───────────────
        # Límite: 200 chars por llamada — partir si es necesario
        fragmentos = partir_texto(texto, max_chars=190)
        urls = []
        for i, frag in enumerate(fragmentos):
            try:
                response = groq_client.audio.speech.create(
                    model="canopylabs/orpheus-v1-english",
                    voice="diana",          # voz femenina natural
                    input=frag,
                    response_format="wav"
                )
                filename = f"resp_{i}.wav"
                path = os.path.join(AUDIO_DIR, filename)
                with open(path, "wb") as f:
                    f.write(response.content)
                urls.append(f"http://{SERVER_IP}:{HTTP_PORT}/{filename}")
            except Exception as e:
                print(f"Error Orpheus fragmento {i}: {e}")

        print(f"Latencia TTS: {time.time() - t:.2f}s")
        # Enviar URLs separadas por coma — el ESP32 las reproduce en orden
        if urls:
            await websocket.send(",".join(urls))

    else:
        # ── Google TTS en español via connecttospeech ─────────
        # Mandamos el texto directo; el ESP32 llama a connecttospeech()
        print(f"Latencia total: {time.time() - t:.2f}s")
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
    async with websockets.serve(handle_audio, "0.0.0.0", 8765, ping_timeout=None):
        print(f"Servidor listo. Modo TTS: {'Orpheus (inglés)' if IDIOMA_TTS == 'en' else 'Google TTS (español)'}")
        print("Pon la mano en el sensor IR y habla.")
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())