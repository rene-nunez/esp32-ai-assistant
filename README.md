# ESP32 AI Assistant

Real-time voice assistant: ESP32 captures audio → server transcribes (Whisper) → responds (Groq LLM) → plays back (Google TTS via ESP32).

## Hardware

- ESP32 DevKit
- MH-ET LIVE I2S MEMS Microphone (INMP441 compatible) — pins 32/33/35
- MAX98357A I2S DAC Amplifier — pins 26/27/25
- 4Ω 3W Speaker
- *(Optional)* TP4056 + Li-Po 502030 3.7V + AMS1117-3.3 for portable power

## Architecture

```
ESP32 (VAD + I2S mic) ──WebSocket binary──→ Server
  VOICE_START / AUDIO chunks / VOICE_END
                                            → Whisper STT
                                            → Groq LLM
                                            → PLAY_TEXT:<text> fragments
ESP32 ←──WebSocket binary────────────────────
  audio.connecttospeech("en") → MAX98357A → Speaker
```

## Server Installation

```bash
python -m venv .venv
.venv\Scripts\activate      # or source .venv/bin/activate
pip install -r requirements.txt
cp .env.example .env        # edit GROQ_API_KEY
python -m server.server_audio
```

## ESP32 Installation

```bash
cp include/secrets.h.example include/secrets.h
# edit include/secrets.h — set WIFI_SSID, WIFI_PASSWORD, SERVER_IP
pio run -t upload
```

## Usage / Configuration

- Press the button to toggle listening (push-on/push-off, LED indicates state).
- Speak — VAD (energy threshold 500) streams audio while voice is detected.
- Silence >1.5s → auto VOICE_END → server responds.
- Press the button again to cancel early.

Tune VAD in `include/config.h`:
- `VAD_ENERGY_THRESHOLD` — increase if mic picks up noise, decrease if quiet speech is missed.
- `SILENCE_TIMEOUT_MS` — response delay after speech ends.

## Protocol

Binary WebSocket messages: `[1 byte type][4 bytes big-endian len][payload]`

- `0x01` — AUDIO (PCM 16-bit 16 kHz)
- `0x02` — TEXT (UTF-8)

Client → Server: `VOICE_START` / `VOICE_END`
Server → Client: `PLAY_TEXT:<text>`

Binary instead of JSON: ESP32 has no heap for a JSON parser; the 5-byte header is parsed with bit-shifts — zero allocation, deterministic latency.

## License

MIT
