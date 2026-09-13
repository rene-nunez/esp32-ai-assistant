#ifndef VAD_H
#define VAD_H

#include <Arduino.h>
#include "protocol_manager.h"
#include "audio_manager.h"
#include "config.h"

class VAD {
public:
  using OnStopListening = void (*)();

  VAD(ProtocolManager& proto, AudioManager& audio) : proto_(proto), audio_(audio) {}

  void begin(OnStopListening on_stop);
  void resetTimeout();
  void tick();

private:
  static int16_t energy(const int16_t* samples, size_t count);

  static constexpr size_t CHUNK_SAMPLES = 512;

  ProtocolManager& proto_;
  AudioManager& audio_;
  OnStopListening on_stop_ = nullptr;
  unsigned long last_voice_time_ = 0;
  int16_t samples_[CHUNK_SAMPLES];
};

#endif
