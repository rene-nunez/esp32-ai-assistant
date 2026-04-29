#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <driver/i2s.h>
#include "Audio.h"

const char* ssid     = "gusgus";
const char* password = "gusgus19++";
const char* websockets_connection_string = "ws://172.20.10.3:8765";

using namespace websockets;
WebsocketsClient client;
Audio audio;

// ── Micrófono en I2S_NUM_1 (Audio.h ocupa NUM_0) ─────────────
#define MIC_WS   15
#define MIC_SD   32
#define MIC_SCK  14
#define I2S_MIC  I2S_NUM_1

// ── MAX98357A ─────────────────────────────────────────────────
#define AMP_BCLK  27
#define AMP_LRC   26
#define AMP_DOUT  25

#define PIN_IR 13

// ── Cola de URLs para reproducción en orden ──────────────────
#define MAX_COLA 8
String cola[MAX_COLA];
int cola_head = 0;
int cola_tail = 0;
bool reproduciendo = false;

void encolar(String url) {
    int next = (cola_tail + 1) % MAX_COLA;
    if (next != cola_head) {
        cola[cola_tail] = url;
        cola_tail = next;
    }
}

bool cola_vacia() { return cola_head == cola_tail; }

String desencolar() {
    String url = cola[cola_head];
    cola_head = (cola_head + 1) % MAX_COLA;
    return url;
}

// ── Callbacks de Audio.h ──────────────────────────────────────
void audio_eof_mp3(const char* info)    { reproduciendo = false; }
void audio_eof_stream(const char* info) { reproduciendo = false; }
void audio_eof_speech(const char* info) { reproduciendo = false; }

void setup() {
    Serial.begin(115200);
    pinMode(PIN_IR, INPUT);

    // 1) WiFi
    WiFi.begin(ssid, password);
    Serial.print("Conectando WiFi");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi OK");

    // 2) WebSocket ANTES de Audio.h
    client.onMessage([](WebsocketsMessage msg) {
        String payload = msg.data();
        payload.trim();
        if (payload.length() == 0) return;

        Serial.print("Recibido: ");
        Serial.println(payload);

        // Si empieza con "http" → es una URL (Orpheus WAV o MP3)
        // Si no → es texto plano → usar connecttospeech (Google TTS español)
        if (payload.startsWith("http")) {
            // Puede ser una o varias URLs separadas por coma
            int start = 0;
            while (start < (int)payload.length()) {
                int comma = payload.indexOf(',', start);
                String url = (comma == -1) ? payload.substring(start) : payload.substring(start, comma);
                url.trim();
                if (url.length() > 0) encolar(url);
                if (comma == -1) break;
                start = comma + 1;
            }
        } else {
            // Texto plano → Google TTS directamente
            reproduciendo = true;
            audio.connecttospeech(payload.c_str(), "es");
        }
    });

    Serial.println("Conectando WebSocket...");
    while (!client.connect(websockets_connection_string)) { delay(1000); Serial.print("."); }
    Serial.println("WebSocket OK");

    // 3) Audio.h
    audio.setPinout(AMP_BCLK, AMP_LRC, AMP_DOUT);
    audio.setVolume(18);
    Serial.println("Audio OK");

    // 4) Micrófono en I2S_NUM_1
    i2s_config_t i2s_config = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate          = 16000,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = 1024,
        .use_apll             = false
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num   = MIC_SCK,
        .ws_io_num    = MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = MIC_SD
    };
    i2s_driver_install(I2S_MIC, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_MIC, &pin_config);

    Serial.println("=== LISTO ===");
}

void loop() {
    audio.loop();
    client.poll();

    // Reproducir siguiente URL de la cola cuando el audio termina
    if (!reproduciendo && !cola_vacia()) {
        String url = desencolar();
        Serial.print("Reproduciendo: ");
        Serial.println(url);
        reproduciendo = true;
        audio.connecttohost(url.c_str());
    }

    // No grabar mientras se reproduce
    if (reproduciendo) return;

    if (digitalRead(PIN_IR) == LOW) {
        int16_t samples[512];
        size_t bytes_read = 0;
        i2s_read(I2S_MIC, &samples, sizeof(samples), &bytes_read, pdMS_TO_TICKS(10));
        if (bytes_read > 0) {
            client.sendBinary((const char*)samples, bytes_read);
        }
    } else {
        delay(5);
    }
}