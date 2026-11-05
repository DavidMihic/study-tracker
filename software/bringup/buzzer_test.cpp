// Buzzer bring-up: MH-FMD active buzzer module, LOW-level trigger (HIGH = silent, LOW = beep).
// Env: buzzer_test (see platformio.ini). Serial monitor 115200.
//
// Wiring: module VCC -> 3V3 (NOT 5V), GND -> GND, I/O -> GPIO25 (silk "25").
// The schematic has R5 = 4.7k pull-up from the I/O line to 3V3 so it stays silent while the
// RP2040 is booting (GPIO pins are low at reset, which would trigger a low-level module).
//
// Commands (serial):
//   b = short beep (60 ms)          l = long beep (300 ms)
//   k = UI click (15 ms)            n = nudge: three soft beeps (the "target time reached" cue)
//   t = train: 10 short beeps       s = stop and go silent
//   p = toggle backlight PWM at brightness 8 on GPIO9 (checks that dimming and the buzzer do not
//       disturb each other: GPIO9 and GPIO25 share PWM slice 4, channel B)
//   h = help
//
// Boot silence test: unplug USB, plug it back in, and listen. Also try the board's RESET button and
// an upload. There must be no beep at any of these times.
#include <Arduino.h>

constexpr uint8_t PIN_BUZZER = 25;
constexpr uint8_t PIN_LCD_BL = 9;

// Non-blocking beep sequencer: a list of (on ms, off ms) steps.
struct Step { uint16_t onMs, offMs; };
const Step SHORT_BEEP[] = {{60, 0}};
const Step LONG_BEEP[]  = {{300, 0}};
const Step CLICK[]      = {{15, 0}};
const Step NUDGE[]      = {{80, 120}, {80, 120}, {80, 0}};
const Step TRAIN[]      = {{40, 60}, {40, 60}, {40, 60}, {40, 60}, {40, 60},
                           {40, 60}, {40, 60}, {40, 60}, {40, 60}, {40, 0}};

const Step* seq = nullptr;
uint8_t seqLen = 0, seqPos = 0;
bool buzzerOn = false;
uint32_t phaseEnd = 0;
bool blDimmed = false;

void buzzerOff() { digitalWrite(PIN_BUZZER, HIGH); buzzerOn = false; }

void startSeq(const Step* s, uint8_t n) {
  seq = s; seqLen = n; seqPos = 0;
  digitalWrite(PIN_BUZZER, LOW);        // LOW = beep
  buzzerOn = true;
  phaseEnd = millis() + seq[0].onMs;
}

void stopSeq() { seq = nullptr; buzzerOff(); }

void updateBuzzer() {
  if (!seq || (int32_t)(millis() - phaseEnd) < 0) return;
  if (buzzerOn) {
    buzzerOff();
    if (seq[seqPos].offMs == 0) { seq = nullptr; return; }
    phaseEnd = millis() + seq[seqPos].offMs;
  } else {
    if (++seqPos >= seqLen) { seq = nullptr; return; }
    digitalWrite(PIN_BUZZER, LOW);
    buzzerOn = true;
    phaseEnd = millis() + seq[seqPos].onMs;
  }
}

#define START(name) startSeq(name, sizeof(name) / sizeof(name[0]))

void printHelp() {
  Serial.println(F("b short | l long | k click | n nudge (3 soft beeps) | t train | s stop"));
  Serial.println(F("p backlight PWM on/off (dim to 8 on GPIO9) | h help"));
}

void setup() {
  // First thing: idle level is HIGH (the module triggers on LOW).
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, HIGH);

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}
  Serial.println(F("\nBuzzer test. Idle = HIGH (silent). Did you hear anything at boot? It should not beep."));
  printHelp();
}

void loop() {
  updateBuzzer();
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'b': START(SHORT_BEEP); break;
      case 'l': START(LONG_BEEP); break;
      case 'k': START(CLICK); break;
      case 'n': START(NUDGE); break;
      case 't': START(TRAIN); break;
      case 's': stopSeq(); Serial.println(F("silent")); break;
      case 'p':
        blDimmed = !blDimmed;
        if (blDimmed) {
          pinMode(PIN_LCD_BL, OUTPUT);
          analogWriteFreq(20000);
          analogWrite(PIN_LCD_BL, 8);
          Serial.println(F("backlight PWM on (8/255), now try the beeps"));
        } else {
          analogWrite(PIN_LCD_BL, 255);
          Serial.println(F("backlight full"));
        }
        break;
      case 'h': printHelp(); break;
      default: break;
    }
  }
}
