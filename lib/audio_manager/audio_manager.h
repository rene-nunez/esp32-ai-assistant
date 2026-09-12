#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>

#include "Audio.h"
#include "ring_buffer.h"
#include "config.h"

class AudioManager {
public:
  void initSpeaker();
  void initMic();
  void tick();

  bool isPlaying() const;
  void stopPlaying();

  int readMic(int16_t* samples, size_t max_samples);

  RingBuffer<String, MAX_QUEUE>& ttsQueue() { return queue_; }

private:
  Audio audio_;
  RingBuffer<String, MAX_QUEUE> queue_;
  bool playing_ = false;
  unsigned long play_start_ = 0;
};

#endif
