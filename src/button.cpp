#include "config.h"

static bool listening = false;
static bool last_btn_state = false;
static unsigned long last_btn_change = 0;

void button_init() {
    pinMode(PIN_BTN, INPUT_PULLUP); // LOW = pressed, no external resistor
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
}

void button_tick() {
    bool btn_now = (digitalRead(PIN_BTN) == LOW);

    if (btn_now && !last_btn_state && millis() - last_btn_change > DEBOUNCE_MS) {
        listening = !listening;
        digitalWrite(PIN_LED, listening);
        last_btn_change = millis();

        Serial.print(listening ? "Listening ON" : "Listening OFF");

        // toggle mode, frees hands during speech
        if (listening) { 
            vad_reset_timeout();
            send_control("VOICE_START");
            Serial.println(" — VOICE_START");
        } else {
            send_control("VOICE_END");
            Serial.println(" — VOICE_END");
        }
    }
    last_btn_state = btn_now;
}

bool is_listening() {
    return listening;
}

void stop_listening() {
    listening = false;
    digitalWrite(PIN_LED, LOW);
}