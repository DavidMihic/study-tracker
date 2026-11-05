// Push switch test: raw bounce logger + debounced click / long press (the logic planned for the product).
// Env: button_test (see platformio.ini). Serial monitor 115200.
//
// Wiring: one pin of the encoder's 2-pin side to GPIO24 (silk "24"), the other to GND.
// Use INPUT_PULLUP: pin reads 1 when released, 0 when pressed. Polarity of the two pins doesn't matter.
//
// Per press it prints:
//  - raw bounce: every edge with the time since the previous edge (no filtering),
//  - the debounced result: "click" (released before 800 ms) or "long press" (fires at 800 ms while held).
// Serial: r = reset counters, t = print counters.
#include <Arduino.h>

constexpr uint8_t PIN_SW = 24;
constexpr uint32_t DEBOUNCE_MS = 25;
constexpr uint32_t LONG_PRESS_MS = 800;

// ---- raw edge logging (collected, printed after the pin has been quiet for 100 ms) ----
constexpr size_t MAX_EDGES = 64;
struct Edge { bool level; uint32_t sinceLastUs; };
Edge edges[MAX_EDGES];
size_t nEdges = 0;
bool rawLevel = true;            // true = released (pulled up)
uint32_t lastEdgeUs = 0;
uint32_t rawFalling = 0, rawRising = 0;

// ---- debounced button ----
bool swDown = false, longFired = false;
uint32_t swChangedAt = 0, swPressedAt = 0;
uint32_t clicks = 0, longPresses = 0;

void printRawBurst() {
  Serial.print(F("  raw edges: "));
  for (size_t i = 0; i < nEdges; i++) {
    Serial.print(edges[i].level ? F("up") : F("DOWN"));
    if (i) { Serial.print('('); Serial.print(edges[i].sinceLastUs); Serial.print(F("us)")); }
    Serial.print(' ');
  }
  Serial.println();
  nEdges = 0;
}

void printTotals() {
  Serial.print(F("clicks ")); Serial.print(clicks);
  Serial.print(F("  long presses ")); Serial.print(longPresses);
  Serial.print(F("  raw falling ")); Serial.print(rawFalling);
  Serial.print(F("  raw rising ")); Serial.println(rawRising);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SW, INPUT_PULLUP);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}
  Serial.println(F("\nPush switch test. Press briefly (click), then hold > 0.8 s (long press)."));
  Serial.println(F("Idle level should be 1 (released). Commands: r = reset, t = totals."));
  Serial.print(F("Start level: ")); Serial.println(digitalRead(PIN_SW));
  rawLevel = digitalRead(PIN_SW);
  lastEdgeUs = micros();
}

void loop() {
  uint32_t nowUs = micros();
  uint32_t nowMs = millis();
  bool level = digitalRead(PIN_SW);

  // raw edges
  if (level != rawLevel) {
    if (nEdges < MAX_EDGES) edges[nEdges++] = {level, nEdges ? nowUs - lastEdgeUs : 0};
    if (level) rawRising++; else rawFalling++;
    rawLevel = level;
    lastEdgeUs = nowUs;
  }
  if (nEdges && (uint32_t)(nowUs - lastEdgeUs) > 100000) printRawBurst();

  // debounced press/release (same logic as the bring-up sketch)
  bool down = !level;
  if (down != swDown && nowMs - swChangedAt > DEBOUNCE_MS) {
    swChangedAt = nowMs;
    swDown = down;
    if (down) {
      swPressedAt = nowMs;
      longFired = false;
    } else if (!longFired) {
      clicks++;
      Serial.print(F("click (held ")); Serial.print(nowMs - swPressedAt); Serial.println(F(" ms)"));
    }
  }
  if (swDown && !longFired && nowMs - swPressedAt > LONG_PRESS_MS) {
    longFired = true;
    longPresses++;
    Serial.println(F("long press"));
  }

  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'r') { clicks = longPresses = rawFalling = rawRising = 0; Serial.println(F("reset")); }
    else if (c == 't') printTotals();
  }
}
