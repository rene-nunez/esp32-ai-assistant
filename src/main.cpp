#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <driver/i2s.h>

const char* ssid = "";
const char* password = "";
const char* websockets_connection_string = "ws://192.168.0.5:8765";

using namespace websockets;
WebsocketsClient client;

// Pines (los mismos que ya te funcionaron)
#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14
#define I2S_PORT I2S_NUM_0

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado a WiFi");

    // Configuración I2S (la que ya probamos)
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512, // Buffer más grande para streaming
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);

    Serial.println("Intentando conectar al servidor Python...");
    
    bool connected = client.connect(websockets_connection_string);
    
    if(connected) {
        Serial.println("¡Conectado exitosamente!");
    } else {
        Serial.println("Fallo inicial, reintentando...");
        while(!client.connect(websockets_connection_string)) {
            delay(1000);
            Serial.print(".");
        }
        Serial.println("\nConectado al servidor Python!");
    }
}

void loop() {
    int16_t samples[512];
    size_t bytes_read;
    
    i2s_read(I2S_PORT, &samples, sizeof(samples), &bytes_read, portMAX_DELAY);
    
    if (bytes_read > 0) {
        client.sendBinary((const char*)samples, bytes_read);
    }
}