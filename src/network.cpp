#include "config.h"

#ifndef SERVER_IP
#error "SERVER_IP not defined. Create .env from .env.example"
#endif

#ifndef WS_PORT
#define WS_PORT 8765
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define WS_URL "ws://" TOSTRING(SERVER_IP) ":" TOSTRING(WS_PORT)

using namespace websockets;
WebsocketsClient ws_client;

static unsigned long last_ws_attempt = 0;

static bool connect_websocket() {
    if (ws_client.connect(WS_URL)) {
        Serial.println("WebSocket OK");
        return true;
    }
    return false;
}

static void process_ws_message(const uint8_t* data, size_t len) {
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

void network_init() {
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

    if (!connect_websocket()) {
        Serial.println("WebSocket unavailable, retrying in loop...");
    }
}

void network_tick() {
    if (ws_client.available()) {
        ws_client.poll();
        last_ws_attempt = 0;
    } else if (last_ws_attempt == 0 || millis() - last_ws_attempt > WS_RETRY_MS) {
        last_ws_attempt = millis();
        Serial.println("Reconnecting WebSocket...");
        connect_websocket();
    }
}
