#include "config.h"
#include <WiFiManager.h>

void setup() {
    Serial.begin(115200);
    Serial.println();

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    if (!wm.autoConnect("ESP32-Assistant")) {
        Serial.println("WiFi not connected, restarting...");
        ESP.restart();
    }
    Serial.println("WiFi OK");

    network_init();
    audio_init();
    i2s_init();
    button_init();

    Serial.println("=== READY (v2 + VAD) ===");
}

void loop() {
    network_tick();
    audio_tick();

    if (is_playing()) return;

    button_tick();

    if (is_listening()) {
        vad_tick();
    } else {
        yield();
    }
}
