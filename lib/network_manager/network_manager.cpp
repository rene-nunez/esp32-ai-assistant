#include "network_manager.h"

#ifndef SERVER_IP
  #error "SERVER_IP not defined. Copy include/secrets.h.example to include/secrets.h"
#endif

#ifndef WS_PORT
  #define WS_PORT 8765
#endif

#define STRINGIFY_(x) #x
#define TOSTRING_(x) STRINGIFY_(x)
#define WS_URL "ws://" TOSTRING_(SERVER_IP) ":" TOSTRING_(WS_PORT)

bool NetworkManager::connect_() {
  if (client_.connect(WS_URL)) {
    Serial.println("[net] ws ok");
    return true;
  }
  return false;
}

void NetworkManager::begin(MessageCallback on_text, BinaryCallback on_binary) {
  on_text_ = on_text;
  on_binary_ = on_binary;

  client_.onMessage([this](websockets::WebsocketsMessage msg) {
    if (msg.isText()) {
      String text = msg.data();
      text.trim();
      if (text.length() > 0 && on_text_) {
        on_text_(text);
      }
    } else if (msg.isBinary()) {
      if (on_binary_) {
        on_binary_(
          (const uint8_t*)msg.data().c_str(),
          msg.data().length()
        );
      }
    }
  });

  client_.onEvent([](websockets::WebsocketsEvent event, String) {
    if (event == websockets::WebsocketsEvent::ConnectionClosed) {
      Serial.println("[net] ws disconnected");
    }
  });

  if (!connect_()) {
    Serial.println("[net] ws unavailable, retrying");
  }
}

void NetworkManager::tick() {
  if (client_.available()) {
    client_.poll();
    last_attempt_ = 0;
  } else if (last_attempt_ == 0 || millis() - last_attempt_ > WS_RETRY_MS) {
    last_attempt_ = millis();
    Serial.println("[net] ws reconnect");
    connect_();
  }
}

bool NetworkManager::sendBinary(const uint8_t* data, size_t len) {
  return client_.sendBinary((const char*)data, len);
}
