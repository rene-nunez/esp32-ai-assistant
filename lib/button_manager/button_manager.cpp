#include "button_manager.h"
#include "pins.h"

void ButtonManager::begin(OnStartListening on_start) {
  on_start_ = on_start;
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
}

void ButtonManager::tick() {
  if (millis() < 2000) return;

  bool btn_now = (digitalRead(PIN_BTN) == LOW);

  if (btn_now && !last_state_ && millis() - last_change_ > DEBOUNCE_MS) {
    listening_ = !listening_;
    digitalWrite(PIN_LED, listening_);
    last_change_ = millis();

    Serial.print(listening_ ? "Listening ON" : "Listening OFF");

    if (listening_) {
      if (on_start_) on_start_();
      proto_.sendControl("VOICE_START");
      Serial.println(" — VOICE_START");
    } else {
      proto_.sendControl("VOICE_END");
      Serial.println(" — VOICE_END");
    }
  }
  last_state_ = btn_now;
}

bool ButtonManager::isListening() const {
  return listening_;
}

void ButtonManager::stopListening() {
  listening_ = false;
  digitalWrite(PIN_LED, LOW);
}
