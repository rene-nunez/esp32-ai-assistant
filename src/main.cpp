#include <WiFi.h>
#include <esp_system.h>

#include "config.h"

#include "network_manager.h"
#include "protocol_manager.h"
#include "audio_manager.h"
#include "button_manager.h"
#include "vad.h"

#ifndef WIFI_SSID
  #error "WIFI_SSID not defined. Copy include/secrets.h.example to include/secrets.h"
#endif

static NetworkManager  net;
static ProtocolManager proto(net);
static AudioManager    audio;
static ButtonManager   button(proto);
static VAD             vad(proto, audio);

static void onTextMessage(const String& text) {
  if (text.startsWith("[log] ")) {
    Serial.println(text.substring(6));
  } else {
    audio.ttsQueue().enqueue(text);
  }
}

static void onBinaryMessage(const uint8_t* data, size_t len) {
  if (len < 5) return;

  uint8_t type = data[0];
  uint32_t payload_len =
    ((uint32_t)data[1] << 24) | ((uint32_t)data[2] << 16) |
    ((uint32_t)data[3] << 8)  | ((uint32_t)data[4]);

  if (5 + payload_len > len) return;

  if (type == MSG_TEXT) {
    String text = String((const char*)(data + 5), payload_len);
    audio.ttsQueue().enqueue(text);
  }
}

static void onStartListening() {
  vad.resetTimeout();
}

static void onStopListening() {
  button.stopListening();
}

static void printResetReason() {
  switch (esp_reset_reason()) {
    case ESP_RST_BROWNOUT: Serial.println("Reset: brownout"); break;
    case ESP_RST_POWERON:  Serial.println("Reset: power-on");  break;
    case ESP_RST_SW:       Serial.println("Reset: software");  break;
    case ESP_RST_PANIC:    Serial.println("Reset: panic");     break;
    case ESP_RST_WDT:      Serial.println("Reset: watchdog");  break;
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  printResetReason();
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi...");
  int timeout = 30;
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(500);
    timeout--;
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi timeout restarting");
    ESP.restart();
  }

  Serial.println();
  Serial.print("WiFi OK, your IP: ");
  Serial.println(WiFi.localIP());

  delay(100);

  net.begin(onTextMessage, onBinaryMessage);
  audio.initSpeaker();
  audio.initMic();
  button.begin(onStartListening);
  vad.begin(onStopListening);

  Serial.println("READY (v2 + VAD)");
}

void loop() {
  net.tick();
  audio.tick();

  if (WiFi.status() != WL_CONNECTED) return;
  if (audio.isPlaying()) return;

  button.tick();

  if (button.isListening()) {
    vad.tick();
  } else {
    yield();
  }
}
