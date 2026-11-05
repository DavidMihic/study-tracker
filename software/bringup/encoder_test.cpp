// Encoder bring-up: raw A/B/SW logger with per-gesture analysis.
// Env: encoder_test (see platformio.ini). Serial monitor 115200.
//
// What it does:
//  - Polls A, B and SW as fast as possible. NO debounce, so you see what the contacts really do.
//  - Groups edges into "gestures": everything until the encoder is quiet for 300 ms.
//  - After each gesture prints the A/B state sequence, the time spent in each state,
//    edge counts per pin, and the net quadrature steps (+ = A leads, - = B leads).
//
// How to use: turn ONE detent, wait, read the line. Repeat clockwise and counter-clockwise.
// Serial commands:  v = toggle live printing of every raw edge,  r = reset totals,  t = print totals.
// Press the encoder knob = print and reset totals (same as t then r).
#include <Arduino.h>

constexpr uint8_t PIN_A = 7;    // silk "5"
constexpr uint8_t PIN_B = 8;    // silk "6"
constexpr uint8_t PIN_SW = 24;  // silk "24"
constexpr uint32_t GESTURE_GAP_US = 300000;

struct Edge { uint8_t state; uint32_t dwellUs; };   // state = (A<<1)|B, dwell = time spent in the PREVIOUS state
constexpr size_t MAX_EDGES = 96;
Edge edges[MAX_EDGES];
size_t nEdges = 0;
bool overflow = false;

uint8_t cur;                 // current raw state
uint32_t lastEdgeUs = 0;
uint8_t gestureStart;        // state when the gesture began
bool inGesture = false;
bool liveEdges = false;
bool lastSw = false;

// totals since last reset
uint32_t aFall, aRise, bFall, bRise, gestures;
int32_t netTotal;

// Gray-code position for A<<1|B: sequence 00 -> 10 -> 11 -> 01 -> 00 counts as +1 (A leads B)
const int8_t POS[4] = {0, 3, 1, 2};

uint8_t readState() { return (digitalRead(PIN_A) << 1) | digitalRead(PIN_B); }

// +1 / -1 for a valid single step, 0 for no change, 99 for an illegal jump (both bits changed = missed edge)
int8_t stepDelta(uint8_t from, uint8_t to) {
  int d = (POS[to] - POS[from] + 4) & 3;
  if (d == 1) return +1;
  if (d == 3) return -1;
  if (d == 2) return 99;
  return 0;
}

void printState(uint8_t s) { Serial.print((s >> 1) & 1); Serial.print(s & 1); }

void printTotals() {
  Serial.println(F("---- totals ----"));
  Serial.print(F("gestures: ")); Serial.print(gestures);
  Serial.print(F("   net steps: ")); Serial.println(netTotal);
  Serial.print(F("A falling: ")); Serial.print(aFall); Serial.print(F("  A rising: ")); Serial.println(aRise);
  Serial.print(F("B falling: ")); Serial.print(bFall); Serial.print(F("  B rising: ")); Serial.println(bRise);
  Serial.println(F("----------------"));
}

void resetTotals() { aFall = aRise = bFall = bRise = gestures = 0; netTotal = 0; }

void finishGesture() {
  inGesture = false;
  if (nEdges == 0) return;
  gestures++;

  uint32_t gA = 0, gB = 0, glitches = 0, illegal = 0;
  int32_t net = 0;
  uint8_t prev = gestureStart;
  uint32_t spanUs = 0;
  for (size_t i = 0; i < nEdges; i++) {
    if ((prev ^ edges[i].state) & 2) gA++;
    if ((prev ^ edges[i].state) & 1) gB++;
    int8_t d = stepDelta(prev, edges[i].state);
    if (d == 99) illegal++; else net += d;
    if (i > 0 && edges[i].dwellUs < 1000) glitches++;   // previous state lasted < 1 ms
    if (i > 0) spanUs += edges[i].dwellUs;
    prev = edges[i].state;
  }
  netTotal += net;

  Serial.print(F("seq: "));
  printState(gestureStart);
  for (size_t i = 0; i < nEdges; i++) {
    Serial.print(F(" > ")); printState(edges[i].state);
  }
  if (overflow) Serial.print(F(" ...(truncated)"));
  Serial.println();

  Serial.print(F("  dwell(us) in each state: "));
  for (size_t i = 0; i < nEdges; i++) { Serial.print(edges[i].dwellUs); Serial.print(' '); }
  Serial.println(F("(first = time in start state before 1st edge; not shown if 0)"));

  Serial.print(F("  A edges: ")); Serial.print(gA);
  Serial.print(F("  B edges: ")); Serial.print(gB);
  Serial.print(F("  net steps: ")); Serial.print(net > 0 ? "+" : ""); Serial.print(net);
  Serial.print(F("  glitches(<1ms): ")); Serial.print(glitches);
  Serial.print(F("  illegal jumps: ")); Serial.println(illegal);
  Serial.print(F("  rest state after: ")); printState(prev); Serial.println();
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_SW, INPUT_PULLUP);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}   // wait briefly for the monitor, but run without a PC too
  cur = readState();
  Serial.println(F("\nEncoder test. Turn ONE detent at a time, wait for the result."));
  Serial.println(F("State shown as AB (1 = open/pulled up, 0 = closed to common)."));
  Serial.println(F("Commands: v = live edges, r = reset totals, t = totals. Knob press = totals + reset."));
  Serial.print(F("Start state: ")); printState(cur); Serial.println();
}

void loop() {
  uint32_t now = micros();
  uint8_t s = readState();

  if (s != cur) {
    uint32_t dwell = now - lastEdgeUs;
    if (!inGesture) {
      inGesture = true;
      gestureStart = cur;
      nEdges = 0;
      overflow = false;
      dwell = 0;
    }
    if ((cur ^ s) & 2) { if (s & 2) aRise++; else aFall++; }
    if ((cur ^ s) & 1) { if (s & 1) bRise++; else bFall++; }
    if (nEdges < MAX_EDGES) { edges[nEdges++] = {s, dwell}; } else overflow = true;
    if (liveEdges) {
      Serial.print(F("  edge -> ")); printState(s);
      Serial.print(F("  after ")); Serial.print(dwell); Serial.println(F(" us"));
    }
    cur = s;
    lastEdgeUs = now;
  }

  if (inGesture && (uint32_t)(now - lastEdgeUs) > GESTURE_GAP_US) finishGesture();

  // Push switch (debounced just enough to avoid repeated prints)
  static uint32_t swAt = 0;
  bool sw = digitalRead(PIN_SW) == LOW;
  if (sw != lastSw && millis() - swAt > 25) {
    swAt = millis();
    lastSw = sw;
    if (sw) { printTotals(); resetTotals(); }
  }

  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'v') { liveEdges = !liveEdges; Serial.println(liveEdges ? F("live edges ON") : F("live edges OFF")); }
    else if (c == 'r') { resetTotals(); Serial.println(F("totals reset")); }
    else if (c == 't') printTotals();
  }
}
