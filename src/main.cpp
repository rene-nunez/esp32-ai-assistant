#include "config.h"
#include <WiFiManager.h>

void setup() {
    Serial.begin(115200);
    Serial.println();

    WiFiManagerParameter server_ip("server_ip", "Server IP", "172.20.10.3", 16);
    WiFiManagerParameter server_port("server_port", "Server Port", "8765", 6);

    WiFiManager wm;
    wm.addParameter(&server_ip);
    wm.addParameter(&server_port);
    wm.setConfigPortalTimeout(180);

    if (!wm.autoConnect("ESP32-Assistant")) {
        Serial.println("WiFi not connected, restarting...");
        ESP.restart();
    }
    Serial.println("WiFi OK");

    network_init(server_ip.getValue(), atoi(server_port.getValue()));
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
