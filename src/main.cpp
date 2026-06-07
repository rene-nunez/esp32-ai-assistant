#include "config.h"
#include <WiFi.h>

#ifndef WIFI_SSID
#error "WIFI_SSID not defined. Copy include/secrets.h.example -> include/secrets.h"
#endif

void setup() {
    Serial.begin(115200);
    Serial.println();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi");
    int timeout = 30;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(500);
        timeout--;
        Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi timeout — restarting");
        ESP.restart();
    }
    Serial.println();
    Serial.print("WiFi OK — IP: ");
    Serial.println(WiFi.localIP());

    network_init();
    audio_init();
    i2s_init();
    button_init();

    Serial.println("=== READY (v2 + VAD) ===");
}

void loop() {
    network_tick();
    audio_tick();

    if (WiFi.status() != WL_CONNECTED) return;

    if (is_playing()) return;

    button_tick();

    if (is_listening()) {
        vad_tick();
    } else {
        yield();
    }
}
