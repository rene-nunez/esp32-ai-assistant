# ESP32 AI Assistant

## Material

- ESP32 DevKit.
- Micrófono MH-ET LIVE I2S MEMS.
- Amplificador MAX98357A I2S DAC.
- Altavoz 4Ω 3W.
- Sensor reflectivo.
- Módulo TP4056.
- Batería Li-Po 502030 3.7V.
- Regulador AMS1117-3.3.

## Plan

### Fase 1: STT

- [x] Hardware: conexión de micrófono vía I2S.
- [x] Trigger: implementación de sensor IR para activación por proximidad.
- [x] Firmware: envío de audio RAW (PCM) mediante WebSockets para baja latencia.
- [x] Servidor Local: procesamiento con Faster-Whisper (Modelo base).
- [ ] Optimización: limpieza de "alucinaciones" y ruidos residuales en la transcripción.

### Fase 2: Groq API

- [x] Integración de IA: conectar la salida de Whisper con la API de Groq (Llama 3).
- [x] Lógica de conversación: configurar el "System Prompt" para que la IA responda de forma breve y concisa.
- [x] Prueba de latencia: validar que el tiempo de respuesta entre el fin del audio y el texto de la IA sea menor a 1 segundo.

### Fase 3: TTS & DAC

- [ ] Hardware: conexión del DAC/Amplificador MAX98357A y altavoz.
- [ ] Generación de voz: integración de Edge-TTS en Python para convertir la respuesta de Groq en audio.
- [ ] Streaming de audio: implementar el envío de datos desde Python al ESP32 para su reproducción.

### Fase 4: autonomía

- [ ] Energía: circuito de carga con TP4056 y batería de litio.
- [ ] Filtro de ruido: implementación de capacitores y reguladores para limpiar el audio de interferencias del Wi-Fi.
- [ ] Ensamblaje final del prototipo.