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
  } else if (text.startsWith("LANG:")) {
    audio.setTtsLang(text.substring(5));
  } else {
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
    case ESP_RST_BROWNOUT: Serial.println("[sys] reset: brownout");  break;
    case ESP_RST_POWERON:  Serial.println("[sys] reset: power-on");  break;
    case ESP_RST_SW:       Serial.println("[sys] reset: software");  break;
    case ESP_RST_PANIC:    Serial.println("[sys] reset: panic");     break;
    case ESP_RST_WDT:      Serial.println("[sys] reset: watchdog");  break;
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  printResetReason();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("[sys] wifi connecting...");
  int timeout = 30;
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(500);
    timeout--;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[sys] wifi timeout, restarting");
    ESP.restart();
  }

  Serial.print("[sys] wifi ");
  Serial.println(WiFi.localIP());

  delay(100);

  net.begin(onTextMessage, nullptr);
  audio.initSpeaker();
  audio.initMic();
  button.begin(onStartListening);
  vad.begin(onStopListening);

  Serial.println("[sys] ready");
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
