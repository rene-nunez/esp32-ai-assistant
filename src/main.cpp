#include "config.h"
#include <WiFi.h>

#ifndef WIFI_SSID
    #error "WIFI_SSID not defined. Copy include/secrets.h.example to include/secrets.h"
#endif

void setup() {
    Serial.begin(115200);
    Serial.println();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi...");
    int timeout = 30; // 15s max, avoids infinite hang

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

    network_init();
    audio_init();
    i2s_init();
    button_init();

    Serial.println("READY (v2 + VAD)");
}

void loop() {
    network_tick(); // keep WebSocket alive
    audio_tick(); // keep TTS playback alive

    if (WiFi.status() != WL_CONNECTED) return; // auto-reconnect in bg
    if (is_playing()) return; // skip VAD while TTS playing (echo)

    button_tick();

    if (is_listening()) {
        vad_tick();
    } else {
        yield(); // cooperative, non-blocking
    }
}