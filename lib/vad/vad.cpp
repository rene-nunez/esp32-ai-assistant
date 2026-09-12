#include "vad.h"
#include "config.h"

void VAD::begin(OnStopListening on_stop) {
  on_stop_ = on_stop;
}

void VAD::resetTimeout() {
  last_voice_time_ = 0;
}

int16_t VAD::energy(const int16_t* samples, size_t count) {
  int32_t sum = 0;
  for (size_t i = 0; i < count; i++) {
    int32_t val = samples[i];
    if (val < 0) val = -val;
    sum += val;
  }
  return (int16_t)(sum / count);
}

void VAD::tick() {
  int16_t samples[512];
  int bytes_read = audio_.readMic(samples, 512);

  if (bytes_read > 0) {
    int count = bytes_read / (int)sizeof(int16_t);
    int16_t e = energy(samples, count);

    if (e > VAD_ENERGY_THRESHOLD) {
      last_voice_time_ = millis();
      proto_.sendAudio(samples, bytes_read);
    }

    if (last_voice_time_ > 0 &&
        millis() - last_voice_time_ > SILENCE_TIMEOUT_MS) {
      Serial.println("Silence timeout, auto VOICE_END");
      proto_.sendControl("VOICE_END");
      if (on_stop_) on_stop_();
      last_voice_time_ = 0;
    }
  }
}
