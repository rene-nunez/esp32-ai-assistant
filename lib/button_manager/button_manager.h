#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include "protocol_manager.h"
#include "config.h"

class ButtonManager {
public:
  using OnStartListening = void (*)();

  explicit ButtonManager(ProtocolManager& proto) : proto_(proto) {}

  void begin(OnStartListening on_start);
  void tick();
  bool isListening() const;
  void stopListening();

private:
  void handlePress();

  ProtocolManager& proto_;
  OnStartListening on_start_ = nullptr;
  bool listening_ = false;
  bool debounced_ = false;
  bool pending_ = false;
  unsigned long pending_since_ = 0;
};

#endif
