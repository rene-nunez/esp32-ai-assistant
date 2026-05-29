#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoWebsockets.h>
#include <driver/i2s.h>
#include "Audio.h"

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

// Protocolo v2
#define MSG_AUDIO 0x01
#define MSG_TEXT  0x02

// VAD
#define VAD_ENERGY_THRESHOLD 500
#define SILENCE_TIMEOUT_MS   1500

bool escuchando = false;
bool btn_anterior = false;
unsigned long ultimo_cambio_btn = 0;
unsigned long ultimo_audio_con_voz = 0;

bool reproduciendo = false;
unsigned long tiempo_inicio_reproduccion = 0;
unsigned long tiempo_fin_reproduccion = 0;
#define TIMEOUT_REPRODUCCION 15000

#define MAX_COLA 8
String cola[MAX_COLA];
int cola_head = 0;
int cola_tail = 0;

unsigned long ultimo_intento_ws = 0;
#define REINTENTO_WS_MS 3000

void enviar_protocolo(uint8_t tipo, const uint8_t* payload, size_t len) {
    uint8_t header[5];
    header[0] = tipo;
    header[1] = (len >> 24) & 0xFF;
    header[2] = (len >> 16) & 0xFF;
    header[3] = (len >>  8) & 0xFF;
    header[4] = len & 0xFF;

    size_t total = 5 + len;
    uint8_t* buf = (uint8_t*)malloc(total);
    if (!buf) return;
    memcpy(buf, header, 5);
    memcpy(buf + 5, payload, len);
    client.sendBinary((const char*)buf, total);
    free(buf);
}

void enviar_control(const char* comando) {
    enviar_protocolo(MSG_TEXT, (const uint8_t*)comando, strlen(comando));
}

int16_t energia_chunk(const int16_t* samples, size_t count) {
    int32_t suma = 0;
    for (size_t i = 0; i < count; i++) {
        int32_t val = samples[i];
        if (val < 0) val = -val;
        suma += val;
    }
    return (int16_t)(suma / count);
}

void leer_boton() {
    bool btn_actual = (digitalRead(PIN_BTN) == LOW);
    if (btn_actual && !btn_anterior && millis() - ultimo_cambio_btn > DEBOUNCE_MS) {
        escuchando = !escuchando;
        digitalWrite(pin_led, escuchando);
        ultimo_cambio_btn = millis();
        Serial.print(escuchando ? "Escuchando ON" : "Escuchando OFF");

        if (escuchando) {
            ultimo_audio_con_voz = 0;
            enviar_control("VOICE_START");
            Serial.println(" — VOICE_START");
        } else {
            enviar_control("VOICE_END");
            Serial.println(" — VOICE_END");
        }
    }
    btn_anterior = btn_actual;
}

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

    if (payload.startsWith("PLAY_TEXT:")) {
        String texto = payload.substring(10);
        audio.connecttospeech(texto.c_str(), "es");
    } else if (payload.startsWith("PLAY_URL:")) {
        String url = payload.substring(9);
        audio.connecttohost(url.c_str());
    } else if (payload.startsWith("http")) {
        audio.connecttohost(payload.c_str());
    } else {
        audio.connecttospeech(payload.c_str(), "es");
    }
}

static void on_audio_end() {
    reproduciendo = false;
    tiempo_fin_reproduccion = millis();
}
void audio_eof_mp3(const char*)    { on_audio_end(); }
void audio_eof_stream(const char*) { on_audio_end(); }
void audio_eof_speech(const char*) { on_audio_end(); }

void procesar_mensaje_ws(const uint8_t* data, size_t len) {
    if (len < 5) return;
    uint8_t tipo = data[0];
    uint32_t payload_len = ((uint32_t)data[1] << 24) |
                           ((uint32_t)data[2] << 16) |
                           ((uint32_t)data[3] <<  8) |
                           ((uint32_t)data[4]);
    if (5 + payload_len > len) return;

    if (tipo == MSG_TEXT) {
        String texto = String((const char*)(data + 5), payload_len);
        Serial.print("Recibido TEXT: ");
        Serial.println(texto);
        encolar(texto);
    }
}

void registrar_callbacks() {
    client.onMessage([](WebsocketsMessage msg) {
        if (msg.isBinary()) {
            procesar_mensaje_ws(
                (const uint8_t*)msg.data().c_str(),
                msg.data().length()
            );
        }
    });
    client.onEvent([](WebsocketsEvent event, String data) {
        if (event == WebsocketsEvent::ConnectionClosed) {
            Serial.println("WebSocket desconectado.");
        }
    });
}

bool conectar_websocket() {
    if (client.connect(websockets_connection_string)) {
        Serial.println("WebSocket OK");
        return true;
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(pin_led, OUTPUT);

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    if (!wm.autoConnect("ESP32-Assistant")) {
        Serial.println("WiFi no conectado, reiniciando...");
        ESP.restart();
    }
    Serial.println("WiFi OK");

    registrar_callbacks();

    if (!conectar_websocket()) {
        Serial.println("WebSocket no disponible, reintentando en loop...");
    }

    audio.setPinout(AMP_BCLK, AMP_LRC, AMP_DOUT);
    audio.setVolume(24);
    Serial.println("Audio OK");

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

    Serial.println("=== LISTO (v2 + VAD) ===");
}

void loop() {
    if (client.available()) {
        client.poll();
        ultimo_intento_ws = 0;
    } else if (ultimo_intento_ws == 0 || millis() - ultimo_intento_ws > REINTENTO_WS_MS) {
        ultimo_intento_ws = millis();
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
            int16_t energia = energia_chunk(samples, bytes_read / sizeof(int16_t));

            if (energia > VAD_ENERGY_THRESHOLD) {
                ultimo_audio_con_voz = millis();
                enviar_protocolo(MSG_AUDIO, (const uint8_t*)samples, bytes_read);
            }

            if (ultimo_audio_con_voz > 0 &&
                millis() - ultimo_audio_con_voz > SILENCE_TIMEOUT_MS) {
                Serial.println("Silencio sostenido — VOICE_END auto");
                enviar_control("VOICE_END");
                escuchando = false;
                digitalWrite(pin_led, LOW);
                ultimo_audio_con_voz = 0;
            }
        }
    } else {
        delay(5);
    }
}
