#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "pins.h"

// Mic energy that starts a session. Lower catches quiet speech,
// higher rejects background noise. ~150 works in quiet rooms
#define VAD_ENERGY_THRESHOLD 150

// Quiet time after speaking before the session ends and the
// audio is sent for processing
#define SILENCE_TIMEOUT_MS 1500

// How long the button must stay stable before it counts as a press
#define DEBOUNCE_MS 50

// Abort a stuck playback after this many ms (0 disables the timeout
#define PLAY_TIMEOUT 15000

// Delay between WebSocket reconnect attempts
#define WS_RETRY_MS 3000

// Max spoken fragments buffered on the ESP32
#define MAX_QUEUE 8

// Internal, do not touch
#define I2S_MIC I2S_NUM_1
#define MSG_AUDIO 0x01
#define MSG_TEXT  0x02

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #error "secrets.h not found. Copy include/secrets.h.example to include/secrets.h"
#endif

#endif
