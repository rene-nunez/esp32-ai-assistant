#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "pins.h"

#define I2S_MIC I2S_NUM_1

#define MSG_AUDIO 0x01
#define MSG_TEXT  0x02

#define VAD_ENERGY_THRESHOLD 100
#define SILENCE_TIMEOUT_MS   1500

#define DEBOUNCE_MS   50
#define PLAY_TIMEOUT  30000
#define WS_RETRY_MS   3000
#define MAX_QUEUE     8

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #error "secrets.h not found. Copy include/secrets.h.example to include/secrets.h"
#endif

#endif
