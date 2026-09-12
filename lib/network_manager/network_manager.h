#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <ArduinoWebsockets.h>
#include "config.h"

class NetworkManager {
public:
  using MessageCallback = void (*)(const String& text);
  using BinaryCallback  = void (*)(const uint8_t* data, size_t len);

  void begin(MessageCallback on_text, BinaryCallback on_binary);
  void tick();
  bool sendBinary(const uint8_t* data, size_t len);

private:
  websockets::WebsocketsClient client_;
  unsigned long last_attempt_ = 0;

  bool connect_();
};

#endif
