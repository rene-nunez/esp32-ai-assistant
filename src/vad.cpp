#include "config.h"

static unsigned long last_voice_time = 0;

void vad_reset_timeout() {
    last_voice_time = 0;
}

static int16_t energy_chunk(const int16_t* samples, size_t count) {
    int32_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        int32_t val = samples[i];
        if (val < 0) val = -val;
        sum += val;
    }
    return (int16_t)(sum / count);
}

void vad_tick() {
    int16_t samples[512];
    int bytes_read = i2s_read_chunk(samples, 512);

    if (bytes_read > 0) {
        int count = bytes_read / (int)sizeof(int16_t);
        int16_t energy = energy_chunk(samples, count);

        if (energy > VAD_ENERGY_THRESHOLD) {
            last_voice_time = millis();
            send_protocol(MSG_AUDIO, (const uint8_t*)samples, bytes_read);
        }

        if (last_voice_time > 0 &&
            millis() - last_voice_time > SILENCE_TIMEOUT_MS) {
            Serial.println("Silence timeout — auto VOICE_END");
            send_control("VOICE_END");
            stop_listening();
            last_voice_time = 0;
        }
    }
}
