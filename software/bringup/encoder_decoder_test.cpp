// Encoder decoder test: full-step quadrature state table, one event per click.
// Env: encoder_decoder (see platformio.ini). Serial monitor 115200.
//
// Expected after the A/B wires were swapped: clockwise = +1 per click, counter-clockwise = -1.
// Turn one full revolution: the position should change by 20 (the EC11 has 20 detents).
// The table only reports a click after a complete 11 > ... > 11 cycle, so contact bounce and
// half-finished clicks are ignored. Cycles that start but return to rest without a click are counted
// as "aborted" so you can see how often that happens.
//
// Serial: r = reset position and counters, t = print summary.
#include <Arduino.h>

constexpr uint8_t PIN_A = 7;    // silk "5"
constexpr uint8_t PIN_B = 8;    // silk "6"
constexpr uint8_t CLICK_LED = LED_BUILTIN;
constexpr int CLICKS_PER_REV = 20;

// Full-step decoder (state table). One result per detent = 4 quadrature steps.
enum : uint8_t { R_START, R_CW_FINAL, R_CW_BEGIN, R_CW_NEXT, R_CCW_BEGIN, R_CCW_FINAL, R_CCW_NEXT };
constexpr uint8_t DIR_CW = 0x10, DIR_CCW = 0x20;
const uint8_t ttable[7][4] = {
  /* R_START     */ {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  /* R_CW_FINAL  */ {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  /* R_CW_BEGIN  */ {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  /* R_CW_NEXT   */ {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  /* R_CCW_BEGIN */ {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  /* R_CCW_FINAL */ {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  /* R_CCW_NEXT  */ {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
uint8_t state = R_START;

int32_t position = 0;
uint32_t cwClicks = 0, ccwClicks = 0, aborted = 0;
uint32_t ledOffAt = 0;

// Returns +1 (CW click), -1 (CCW click) or 0.
int8_t readEncoder() {
  uint8_t pins = (digitalRead(PIN_B) << 1) | digitalRead(PIN_A);
  uint8_t prev = state & 0x0F;
  state = ttable[prev][pins];
  uint8_t dir = state & 0x30;
  state &= 0x0F;
  if (dir == DIR_CW)  return +1;
  if (dir == DIR_CCW) return -1;
  if (prev != R_START && state == R_START) aborted++;   // left rest, came back without a click
  return 0;
}

void printSummary() {
  Serial.println(F("---- summary ----"));
  Serial.print(F("position: ")); Serial.print(position);
  Serial.print(F("  (CW clicks ")); Serial.print(cwClicks);
  Serial.print(F(", CCW clicks ")); Serial.print(ccwClicks);
  Serial.print(F(", aborted cycles ")); Serial.print(aborted); Serial.println(')');
  Serial.println(F("-----------------"));
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(CLICK_LED, OUTPUT);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}
  Serial.println(F("\nDecoder test. CW should print +1, CCW -1. One full turn = 20 clicks."));
  Serial.println(F("Commands: r = reset, t = summary"));
}

void loop() {
  int8_t step = readEncoder();
  if (step) {
    position += step;
    if (step > 0) cwClicks++; else ccwClicks++;
    digitalWrite(CLICK_LED, HIGH);
    ledOffAt = millis() + 40;
    // position within the current revolution, 0..19
    int32_t inRev = ((position % CLICKS_PER_REV) + CLICKS_PER_REV) % CLICKS_PER_REV;
    Serial.print(step > 0 ? F("CW  +1") : F("CCW -1"));
    Serial.print(F("   position ")); Serial.print(position);
    Serial.print(F("   click ")); Serial.print(inRev); Serial.print('/'); Serial.println(CLICKS_PER_REV);
  }
  if (ledOffAt && (int32_t)(millis() - ledOffAt) >= 0) { digitalWrite(CLICK_LED, LOW); ledOffAt = 0; }

  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'r') { position = 0; cwClicks = ccwClicks = aborted = 0; state = R_START; Serial.println(F("reset")); }
    else if (c == 't') printSummary();
  }
}
