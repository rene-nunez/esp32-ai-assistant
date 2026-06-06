#include "config.h"
#include <WiFiManager.h>
#include "Audio.h"

using namespace websockets;
WebsocketsClient ws_client;
Audio audio;

bool listening = false;
bool last_btn_state = false;
unsigned long last_btn_change = 0;
unsigned long last_voice_time = 0;

bool playing = false;
unsigned long play_start_time = 0;

unsigned long last_ws_attempt = 0;

int16_t energy_chunk(const int16_t* samples, size_t count) {
    int32_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        int32_t val = samples[i];
        if (val < 0) val = -val;
        sum += val;
    }
    return (int16_t)(sum / count);
}

void read_button() {
    bool btn_now = (digitalRead(PIN_BTN) == LOW);
    if (btn_now && !last_btn_state && millis() - last_btn_change > DEBOUNCE_MS) {
        listening = !listening;
        digitalWrite(PIN_LED, listening);
        last_btn_change = millis();
        Serial.print(listening ? "Listening ON" : "Listening OFF");

        if (listening) {
            last_voice_time = 0;
            send_control("VOICE_START");
            Serial.println(" — VOICE_START");
        } else {
            send_control("VOICE_END");
            Serial.println(" — VOICE_END");
        }
    }
    last_btn_state = btn_now;
}

void play(String payload) {
    playing = true;
    play_start_time = millis();
    Serial.print("Playing: ");
    Serial.println(payload);

    if (payload.startsWith("PLAY_TEXT:")) {
        String text = payload.substring(10);
        audio.connecttospeech(text.c_str(), "es");
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
    playing = false;
}
void audio_eof_mp3(const char*)    { on_audio_end(); }
void audio_eof_stream(const char*) { on_audio_end(); }
void audio_eof_speech(const char*) { on_audio_end(); }

void process_ws_message(const uint8_t* data, size_t len) {
    if (len < 5) return;
    uint8_t type = data[0];
    uint32_t payload_len = ((uint32_t)data[1] << 24) |
                           ((uint32_t)data[2] << 16) |
                           ((uint32_t)data[3] << 8) |
                           ((uint32_t)data[4]);
    if (5 + payload_len > len) return;

    if (type == MSG_TEXT) {
        String text = String((const char*)(data + 5), payload_len);
        Serial.print("Received TEXT: ");
        Serial.println(text);
        enqueue(text);
    }
}

void register_callbacks() {
    ws_client.onMessage([](WebsocketsMessage msg) {
        if (msg.isBinary()) {
            process_ws_message(
                (const uint8_t*)msg.data().c_str(),
                msg.data().length()
            );
        }
    });
    ws_client.onEvent([](WebsocketsEvent event, String data) {
        if (event == WebsocketsEvent::ConnectionClosed) {
            Serial.println("WebSocket disconnected.");
        }
    });
}

bool connect_websocket() {
    if (ws_client.connect("ws://172.20.10.3:8765")) {
        Serial.println("WebSocket OK");
        return true;
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    if (!wm.autoConnect("ESP32-Assistant")) {
        Serial.println("WiFi not connected, restarting...");
        ESP.restart();
    }
    Serial.println("WiFi OK");

    register_callbacks();

    if (!connect_websocket()) {
        Serial.println("WebSocket unavailable, retrying in loop...");
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

    Serial.println("=== READY (v2 + VAD) ===");
}

void loop() {
    if (ws_client.available()) {
        ws_client.poll();
        last_ws_attempt = 0;
    } else if (last_ws_attempt == 0 || millis() - last_ws_attempt > WS_RETRY_MS) {
        last_ws_attempt = millis();
        Serial.println("Reconnecting WebSocket...");
        connect_websocket();
    }

    audio.loop();

    if (!playing && !queue_empty()) {
        play(dequeue());
    }

    if (playing && (millis() - play_start_time > PLAY_TIMEOUT)) {
        Serial.println("Playback TIMEOUT — releasing");
        audio.stopSong();
        playing = false;
    }
    if (playing) return;

    read_button();

    if (listening) {
        int16_t samples[512];
        size_t bytes_read = 0;
        i2s_read(I2S_MIC, &samples, sizeof(samples), &bytes_read, pdMS_TO_TICKS(10));

        if (bytes_read > 0) {
            int16_t energy = energy_chunk(samples, bytes_read / sizeof(int16_t));

            if (energy > VAD_ENERGY_THRESHOLD) {
                last_voice_time = millis();
                send_protocol(MSG_AUDIO, (const uint8_t*)samples, bytes_read);
            }

            if (last_voice_time > 0 &&
                millis() - last_voice_time > SILENCE_TIMEOUT_MS) {
                Serial.println("Silence timeout — auto VOICE_END");
                send_control("VOICE_END");
                listening = false;
                digitalWrite(PIN_LED, LOW);
                last_voice_time = 0;
            }
        }
    } else {
        yield();
    }
}
