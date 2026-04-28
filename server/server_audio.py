import asyncio
import websockets
import numpy as np
from faster_whisper import WhisperModel
from groq import Groq
import time

# Cargamos el modelo base de faster whisper
model_size = "base"
print("Cargando modelo de IA...")
model = WhisperModel(model_size, device="cpu", compute_type="int8")

# API key de Groq
GROQ_API_KEY = ""
groq_client  = Groq(api_key=GROQ_API_KEY)

system_prompt = {
    "role": "system",
    "content": "Eres un asistente de voz inteligente conectado a un ESP32, tus respuestas deben ser muy breves y útiles" 
}

async def handle_audio(websocket):
    print("¡ESP32 Conectado! (Esperando señal del sensor IR...)")
    audio_buffer = []
    
    try:
        # Usamos un timeout corto para detectar cuando dejas de enviar datos (quitas la mano)
        while True:
            try:
                # Esperamos datos. Si en 0.5 seg no llega nada, asumimos que quitaste la mano.
                message = await asyncio.wait_for(websocket.recv(), timeout = 0.5)
                
                samples = np.frombuffer(message, dtype=np.int16)

                # Multiplicamos por 8.0 para que el modelo escuche mucho mejor
                audio_buffer.extend(samples.astype(np.float32) / 32768.0)
                
                # Si el buffer es muy largo (más de 7 segundos), procesamos para no saturar
                if len(audio_buffer) > 16000 * 7:
                    await process_audio(audio_buffer)
                    audio_buffer = []

            except asyncio.TimeoutError:
                # SI HUBO TIMEOUT: significa que ya no estás enviando audio (quitaste la mano)
                if len(audio_buffer) > 8000: # Solo si hay al menos medio segundo de audio
                    print("Analizando frase...")
                    await process_audio(audio_buffer)
                    audio_buffer = [] # Limpiamos para la próxima vez que pongas la mano
                continue

    except websockets.exceptions.ConnectionClosed:
        print("ESP32 Desconectado.")
    except Exception as e:
        print(f"Error: {e}")

async def process_audio(audio_data):
    audio_np = np.array(audio_data)
    
    # Aplicar filtros nativos de Whisper para reducir alucinaciones
    segments, info = model.transcribe(
        audio_np * 8.0, 
        beam_size = 5, 
        language = "es",
        vad_filter = True, # Filtro de actividad de voz
        vad_parameters = dict(min_silence_duration_ms=500),
        no_speech_threshold = 0.6,
        log_prob_threshold = -1.0 # Si la probabilidad es muy baja, lo ignora
    )
    
    texto_detectado = ""

    for segment in segments:
        # Solo aceptamos el texto si la IA está al menos 50% segura y no es texto vacio
        if segment.no_speech_prob < 0.5 and segment.text.strip():
            texto_detectado += segment.text + " "

    texto_limpio = texto_detectado.strip()

    # Si el texto es muy corto, ignorar
    if len(texto_limpio) > 2: 
        print(f"[Voz]: {texto_limpio}")
        await(get_res(texto_limpio))
        
    else:
        print("Sonido detectado, pero no parece ser una frase clara.")

async def get_res(message):
    inicio_latencia = time.time() # Contador para medir latencia

    try:
        print(f"Preguntandole al modelo...")

        chat_completion = groq_client.chat.completions.create(
            messages = [
                system_prompt,
                {"role": "user", "content": message}
            ],
            model = "llama-3.1-8b-instant",
            temperature = 0.7,
            max_tokens = 300
        )

        respuesta = chat_completion.choices[0].message.content
        fin_latencia = time.time() - inicio_latencia

        print(f"IA: {respuesta}")
        print(f"Latencia de respuesta: {fin_latencia:.2f} segundos")

        return respuesta
    
    except Exception as error:
        print(f"Error IA: {error}")
        return None

async def main():
    # ping_timeout = None para evitar desconexiones accidentales
    async with websockets.serve(handle_audio, "0.0.0.0", 8765, ping_timeout=None):
        print("Servidor listo. Pon la mano en el sensor IR y habla.")
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())