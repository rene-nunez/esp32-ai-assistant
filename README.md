# ESP32 AI Assistant

Voice assistant with ESP32 + Whisper + Groq + TTS.

## Quickstart

```bash
python -m venv .venv
pip install -r requirements.txt
cp .env.example .env   # edit GROQ_API_KEY and SERVER_IP
python -m server.server_audio
```

Flash `src/main.cpp` with PlatformIO. On first boot, connect to the `ESP32-Assistant` hotspot to configure WiFi.

Press the button to toggle listening. Speak — audio is sent only when voice is detected. Silence >1.5s triggers the response.

## Protocol

Binary WebSocket messages: `[1 byte type][4 bytes length BE][payload]`

- `0x01` (AUDIO): PCM/WAV
- `0x02` (TEXT): `VOICE_START`, `VOICE_END`, `PLAY_TEXT:...`, `PLAY_URL:...`
