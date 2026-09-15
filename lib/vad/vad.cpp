#include "vad.h"
#include "config.h"

void VAD::begin(OnStopListening on_stop) {
  on_stop_ = on_stop;
}

void VAD::resetTimeout() {
  last_voice_time_ = 0;
  sending_ = false;
  for (size_t i = 0; i < PREROLL_CHUNKS; i++) {
    preroll_bytes_[i] = 0;
  }
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
  int bytes_read = audio_.readMic(samples_, CHUNK_SAMPLES);

  if (bytes_read > 0) {
    int count = bytes_read / (int)sizeof(int16_t);
    int16_t e = energy(samples_, count);

    if (e > VAD_ENERGY_THRESHOLD) {
      last_voice_time_ = millis();
    }

    if (last_voice_time_ > 0) {
      if (!sending_) {
        sending_ = true;
        for (size_t i = 0; i < PREROLL_CHUNKS; i++) {
          size_t idx = (preroll_index_ + i) % PREROLL_CHUNKS;
          if (preroll_bytes_[idx] > 0) {
            proto_.sendAudio(preroll_[idx], preroll_bytes_[idx]);
          }
        }
      }

      proto_.sendAudio(samples_, bytes_read);

      if (millis() - last_voice_time_ > SILENCE_TIMEOUT_MS) {
        Serial.println("[vad] silence timeout");
        proto_.sendControl("VOICE_END");
        if (on_stop_) on_stop_();
        sending_ = false;
        last_voice_time_ = 0;
      }
    } else {
      preroll_bytes_[preroll_index_] = bytes_read;
      memcpy(preroll_[preroll_index_], samples_, bytes_read);
      preroll_index_ = (preroll_index_ + 1) % PREROLL_CHUNKS;
    }
  }
}
