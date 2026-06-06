#ifndef CONFIG_H
#define CONFIG_H

// Framework and libraries
#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include <driver/i2s.h>

extern websockets::WebsocketsClient ws_client;

// MH-ET LIVE I2S MEMS Microphone
#define MIC_SCK 32
#define MIC_WS 33
#define MIC_SD 35
#define I2S_MIC I2S_NUM_1

// Amplifier MAX98357A I2S DAC
#define AMP_BCLK 26
#define AMP_LRC 27
#define AMP_DOUT 25

#define PIN_BTN 17 // Button
#define PIN_LED 15 // LED

// Configuration
#define MSG_AUDIO 0x01
#define MSG_TEXT 0x02

#define VAD_ENERGY_THRESHOLD 500
#define SILENCE_TIMEOUT_MS 1500

#define DEBOUNCE_MS 50
#define PLAY_TIMEOUT 15000
#define WS_RETRY_MS 3000
#define MAX_QUEUE 8

// Functions
void send_protocol(uint8_t type, const uint8_t* payload, size_t len);
void send_control(const char* command);

void enqueue(String s);
bool queue_empty();
String dequeue();

void audio_init();
void audio_tick();
bool is_playing();
void i2s_init();
int i2s_read_chunk(int16_t* samples, size_t max_samples);

void button_init();
void button_tick();
bool is_listening();
void stop_listening();

void network_init();
void network_tick();

void vad_reset_timeout();
void vad_tick();

#endif