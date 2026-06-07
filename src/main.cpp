#include "config.h"
#include <WiFi.h>
#include <esp_system.h>

#ifndef WIFI_SSID
    #error "WIFI_SSID not defined. Copy include/secrets.h.example to include/secrets.h"
#endif

static void print_reset_reason() {
    switch (esp_reset_reason()) {
        case ESP_RST_BROWNOUT: Serial.println("Reset: brownout"); break;
        case ESP_RST_POWERON:  Serial.println("Reset: power-on"); break;
        case ESP_RST_SW:       Serial.println("Reset: software"); break;
        case ESP_RST_PANIC:    Serial.println("Reset: panic"); break;
        case ESP_RST_WDT:      Serial.println("Reset: watchdog"); break;
        default: break;
    }
}

void setup() {
    Serial.begin(115200);
    print_reset_reason();
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

    delay(100); // let power rail stabilize before enabling I2S peripherals
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