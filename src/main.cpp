#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <driver/i2s.h>
#include "Audio.h"

const char* ssid     = "ssid";
const char* password = "password";
const char* websockets_connection_string = "ws://172.20.10.3:8765";

using namespace websockets;
WebsocketsClient client;
Audio audio;

#define MIC_SCK  32
#define MIC_WS   33
#define MIC_SD   35
#define I2S_MIC  I2S_NUM_1

#define AMP_BCLK  26
#define AMP_LRC   27
#define AMP_DOUT  25

#define PIN_BTN 17
#define DEBOUNCE_MS 50
#define pin_led 15

bool escuchando = false;
bool btn_anterior = false;
unsigned long ultimo_cambio_btn = 0;

void leer_boton() {
    bool btn_actual = (digitalRead(PIN_BTN) == LOW);
    if (btn_actual && !btn_anterior && millis() - ultimo_cambio_btn > DEBOUNCE_MS) {
        escuchando = !escuchando;
        digitalWrite(pin_led, escuchando);
        ultimo_cambio_btn = millis();
        Serial.println(escuchando ? "Escuchando ON" : "Escuchando OFF");
    }
    btn_anterior = btn_actual;
}

bool reproduciendo = false;
unsigned long tiempo_inicio_reproduccion = 0;
unsigned long tiempo_fin_reproduccion = 0;
#define TIMEOUT_REPRODUCCION 15000

#define MAX_COLA 8
String cola[MAX_COLA];
int cola_head = 0;
int cola_tail = 0;

void encolar(String s) {
    int next = (cola_tail + 1) % MAX_COLA;
    if (next != cola_head) { cola[cola_tail] = s; cola_tail = next; }
}
bool cola_vacia() { return cola_head == cola_tail; }
String desencolar() {
    String s = cola[cola_head];
    cola_head = (cola_head + 1) % MAX_COLA;
    return s;
}

void reproducir(String payload) {
    reproduciendo = true;
    tiempo_inicio_reproduccion = millis();
    Serial.print("Reproduciendo: ");
    Serial.println(payload);
    if (payload.startsWith("http")) {
        audio.connecttohost(payload.c_str());
    } else {
        audio.connecttospeech(payload.c_str(), "es");
    }
}

void audio_eof_mp3(const char* info)    { reproduciendo = false; tiempo_fin_reproduccion = millis(); }
void audio_eof_stream(const char* info) { reproduciendo = false; tiempo_fin_reproduccion = millis(); }
void audio_eof_speech(const char* info) { reproduciendo = false; tiempo_fin_reproduccion = millis(); }

void registrar_callbacks() {
    client.onMessage([](WebsocketsMessage msg) {
        String payload = msg.data();
        payload.trim();
        if (payload.length() == 0) return;
        Serial.print("Recibido: "); Serial.println(payload);

        int start = 0;
        while (start < (int)payload.length()) {
            int comma = payload.indexOf(',', start);
            String frag = (comma == -1) ? payload.substring(start) : payload.substring(start, comma);
            frag.trim();
            if (frag.length() > 0) encolar(frag);
            if (comma == -1) break;
            start = comma + 1;
        }
    });

    client.onEvent([](WebsocketsEvent event, String data) {
        if (event == WebsocketsEvent::ConnectionClosed) {
            Serial.println("WebSocket desconectado.");
            Serial.print("WiFi estado: ");
            Serial.println(WiFi.status());
        }
    });
}

void conectar_websocket() {
    while (!client.connect(websockets_connection_string)) {
        delay(1000); Serial.print(".");
    }
    Serial.println("\nWebSocket OK");
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(pin_led, OUTPUT);

    // 1) WiFi
    WiFi.begin(ssid, password);
    Serial.print("Conectando WiFi");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi OK");

    // 2) Callbacks solo una vez
    registrar_callbacks();

    // 3) WebSocket
    conectar_websocket();

    // 4) Audio
    audio.setPinout(AMP_BCLK, AMP_LRC, AMP_DOUT);
    audio.setVolume(24);
    Serial.println("Audio OK");

    // 5) Micrófono I2S
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
    if (client.available()) {
        client.poll();
    } else {
        Serial.println("Reconectando WebSocket...");
        conectar_websocket();
    }

    audio.loop();

    if (!reproduciendo && !cola_vacia()) {
        reproducir(desencolar());
    }

    if (reproduciendo && (millis() - tiempo_inicio_reproduccion > TIMEOUT_REPRODUCCION)) {
        Serial.println("TIMEOUT reproducción — liberando");
        audio.stopSong();
        reproduciendo = false;
    }
    if (reproduciendo) return;

    leer_boton();

    if (escuchando) {
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