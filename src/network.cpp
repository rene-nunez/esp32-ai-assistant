#include "config.h"

using namespace websockets;
WebsocketsClient ws_client;

static String ws_url;
static unsigned long last_ws_attempt = 0;

static bool connect_websocket() {
    if (ws_client.connect(ws_url)) {
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

void network_init(const char* host, int port) {
    ws_url = "ws://";
    ws_url += host;
    ws_url += ":";
    ws_url += port;
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
