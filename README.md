# ESP32 AI Assistant

Asistente de voz con ESP32 + Whisper + Groq + TTS.

## Hardware

- ESP32 DevKit
- Micrófono MH-ET LIVE I2S MEMS
- Amplificador MAX98357A + altavoz
- Botón de 4 vías

## Arquitectura

```
ESP32 ──WebSocket (protocolo v2)──▶ Servidor Python
  │ 0x01 = audio PCM                 │ Whisper → Groq → TTS
  │ 0x02 = control (START/END)       │
  │◀── PLAY_TEXT / PLAY_URL ─────────│
```

## Servidor

```bash
python -m venv .venv
pip install -r requirements.txt
cp .env.example .env   # edita GROQ_API_KEY y SERVER_IP
python -m server.server_audio
```

## ESP32

Abrir `src/main.cpp` con PlatformIO (VSCode). En el primer arranque crea un hotspot `ESP32-Assistant` para configurar WiFi.

### Uso

1. Presiona el botón para comenzar a escuchar (toggle ON/OFF)
2. Habla; solo se envía audio cuando detecta voz
3. Silencio >1.5s → procesa la frase y responde por el altavoz
