import os
from typing import Final
from dotenv import load_dotenv

load_dotenv()

# Groq
GROQ_API_KEY: Final[str | None] = os.getenv("GROQ_API_KEY")
if not GROQ_API_KEY:
    raise RuntimeError("GROQ_API_KEY not set. Create a .env file from .env.example")

WS_PORT: Final[int] = int(os.getenv("SERVER_WS_PORT", "8765"))

# Whisper
WHISPER_MODEL: Final[str] = os.getenv("WHISPER_MODEL", "base")
WHISPER_DEVICE: Final[str] = os.getenv("WHISPER_DEVICE", "cpu")
WHISPER_COMPUTE_TYPE: Final[str] = os.getenv("WHISPER_COMPUTE_TYPE", "int8")

# Whisper VAD
VAD_MIN_SILENCE_MS: Final[int] = 800
VAD_THRESHOLD: Final[float] = 0.6
VAD_MIN_SPEECH_MS: Final[int] = 150  # lower = less aggressive filtering of short chunks
NO_SPEECH_THRESHOLD: Final[float] = 0.7
NO_SPEECH_PROB_THRESHOLD: Final[float] = 0.5

VOLUME_MIN_THRESHOLD: Final[float] = 0.005  # lower = more sensitive to quiet speech

# LLM
LLM_MODEL: Final[str] = os.getenv("LLM_MODEL", "groq/compound")
LLM_TEMPERATURE: Final[float] = 0.7
LLM_MAX_TOKENS: Final[int] = 80

TTS_MAX_CHARS: Final[int] = 300  # Google TTS URL limit ~200 chars per fragment

SYSTEM_PROMPT: Final[dict[str, str]] = {
    "role": "system",
    "content": (
        "You are a smart voice assistant. Keep responses very short. "
        "Max 2 short sentences. Always respond in English."
    ),
}