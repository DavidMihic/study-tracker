#include <Arduino.h>

constexpr uint8_t PIN_BUZZER = 25;  // silk "25"

// 0 = never touch the buzzer pin: shows what the pin does by itself at power-up
// 1 = drive the pin HIGH (silent) at the very start of setup()
#define BOOT_TEST_MODE 1

void setup() {
#if BOOT_TEST_MODE == 1
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, HIGH);
#endif
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);      // LED on = the 3 s listening window
}

void loop() {
  static bool done = false;
  if (!done && millis() >= 3000) {
    done = true;
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);      // proof beep: the wiring works
    delay(150);
    digitalWrite(PIN_BUZZER, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
  }
}