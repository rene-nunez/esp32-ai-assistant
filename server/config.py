import os
from typing import Final

from dotenv import load_dotenv

load_dotenv()

# Groq
GROQ_API_KEY: Final[str | None] = os.getenv("GROQ_API_KEY")
if not GROQ_API_KEY:
    raise RuntimeError("GROQ_API_KEY no configurada. Crea un archivo .env basado en .env.example")

# Servidor
SERVER_IP: Final[str] = os.getenv("SERVER_IP", "172.20.10.2")
WS_PORT: Final[int] = int(os.getenv("SERVER_WS_PORT", "8765"))
HTTP_PORT: Final[int] = int(os.getenv("SERVER_HTTP_PORT", "8766"))
TTS_LANG: Final[str] = os.getenv("TTS_LANG", "es")

# Whisper
WHISPER_MODEL: Final[str] = os.getenv("WHISPER_MODEL", "base")
WHISPER_DEVICE: Final[str] = os.getenv("WHISPER_DEVICE", "cpu")
WHISPER_COMPUTE_TYPE: Final[str] = os.getenv("WHISPER_COMPUTE_TYPE", "int8")

# VAD (servidor — Whisper VAD)
VAD_MIN_SILENCE_MS: Final[int] = 800
VAD_THRESHOLD: Final[float] = 0.6
VAD_MIN_SPEECH_MS: Final[int] = 250
NO_SPEECH_THRESHOLD: Final[float] = 0.7

# Audio
AUDIO_RATE: Final[int] = 16000
VOLUME_MIN_THRESHOLD: Final[float] = 0.01

# LLM
LLM_MODEL: Final[str] = "groq/compound"
LLM_TEMPERATURE: Final[float] = 0.7
LLM_MAX_TOKENS: Final[int] = 300

# TTS Orpheus
TTS_MAX_CHARS: Final[int] = 190
TTS_VOICE: Final[str] = "diana"

SYSTEM_PROMPT: Final[dict[str, str]] = {
    "role": "system",
    "content": (
        "Eres un asistente de voz inteligente conectado a un ESP32, "
        "tus respuestas deben ser muy breves y útiles. Máximo 2 oraciones."
    ),
}
