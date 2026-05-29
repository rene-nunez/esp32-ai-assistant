import os
from dotenv import load_dotenv

load_dotenv()

# Groq
GROQ_API_KEY = os.getenv("GROQ_API_KEY")
if not GROQ_API_KEY:
    raise RuntimeError("GROQ_API_KEY no configurada. Crea un archivo .env basado en .env.example")

# Servidor
SERVER_IP = os.getenv("SERVER_IP", "172.20.10.2")
WS_PORT = int(os.getenv("SERVER_WS_PORT", "8765"))
HTTP_PORT = int(os.getenv("SERVER_HTTP_PORT", "8766"))
TTS_LANG = os.getenv("TTS_LANG", "es")

# Whisper
WHISPER_MODEL = os.getenv("WHISPER_MODEL", "base")
WHISPER_DEVICE = os.getenv("WHISPER_DEVICE", "cpu")
WHISPER_COMPUTE_TYPE = os.getenv("WHISPER_COMPUTE_TYPE", "int8")

# VAD
VAD_MIN_SILENCE_MS = 800
VAD_THRESHOLD = 0.6
VAD_MIN_SPEECH_MS = 250
NO_SPEECH_THRESHOLD = 0.7

# Audio
AUDIO_RATE = 16000
AUDIO_CHUNK_SECONDS = 7
AUDIO_MIN_SILENCE_SAMPLES = 8000
VOLUME_MIN_THRESHOLD = 0.01

# LLM
LLM_MODEL = "groq/compound"
LLM_TEMPERATURE = 0.7
LLM_MAX_TOKENS = 300

# TTS Orpheus
TTS_MAX_CHARS = 190
TTS_VOICE = "diana"

SYSTEM_PROMPT = {
    "role": "system",
    "content": "Eres un asistente de voz inteligente conectado a un ESP32, tus respuestas deben ser muy breves y útiles. Máximo 2 oraciones."
}
