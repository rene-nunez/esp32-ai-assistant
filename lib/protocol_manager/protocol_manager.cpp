#include "protocol_manager.h"
#include "config.h"

void ProtocolManager::send(uint8_t type, const uint8_t* payload, size_t len) {
  buf_[0] = type;
  buf_[1] = (len >> 24) & 0xFF;
  buf_[2] = (len >> 16) & 0xFF;
  buf_[3] = (len >> 8)  & 0xFF;
  buf_[4] = len & 0xFF;

  memcpy(buf_ + 5, payload, len);
  net_.sendBinary(buf_, 5 + len);
}

void ProtocolManager::sendAudio(const int16_t* samples, size_t bytes) {
  send(MSG_AUDIO, (const uint8_t*)samples, bytes);
}

void ProtocolManager::sendControl(const char* command) {
  send(MSG_TEXT, (const uint8_t*)command, strlen(command));
}
