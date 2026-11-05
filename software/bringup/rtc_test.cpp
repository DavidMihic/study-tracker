// RTC bring-up: DS3231 module on I2C (GPIO2 = SDA, GPIO3 = SCL). Serial monitor 115200.
// Env: rtc_test (see platformio.ini).
//
// Wiring: module VCC -> 3V3, GND -> GND, SDA -> GPIO2 (silk "SDA"), SCL -> GPIO3 (silk "SCL").
// SQW and 32K stay unconnected. Insert the CR2032 for the power-loss test (see below).
//
// What it does:
//  - Picks the I2C bus: GPIO2/3 are I2C1 on this chip, which the core may call Wire or Wire1,
//    so the sketch tries both and tells you which one works.
//  - Scans the bus. Expected: 0x57 (AT24C32 EEPROM on the module) and 0x68 (DS3231).
//  - Prints the time, the unix time (the value we will store as a session start) and the chip
//    temperature once a second.
//  - Reports the "oscillator stopped" flag (lostPower): true means the time cannot be trusted.
//
// Commands (serial; Enter is optional, a command also runs 300 ms after the last character):
//   c                      set the clock to the time this firmware was compiled (a few seconds
//                          to a minute behind the real time, fine for a first check)
//   T2026-11-05 18:30:00   set the clock to an exact time (capital T, then date and time)
//   i                      scan the I2C bus again
//   h                      help
//
// Power-loss test (needs the coin cell): set the time, unplug USB for about 30 s, plug it back in.
// The time must have kept running and "oscillator stopped" must say no.
// Without the cell, every power-up shows "oscillator stopped: YES" and a stale or default time. That is expected.
#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

constexpr uint8_t PIN_RTC_SDA = 2;
constexpr uint8_t PIN_RTC_SCL = 3;

RTC_DS3231 rtc;
TwoWire* bus = nullptr;
const char* busName = "none";
bool rtcOk = false;

// setSDA/setSCL return false for the I2C block that cannot use these pins.
TwoWire* pickI2C() {
  if (Wire.setSDA(PIN_RTC_SDA) && Wire.setSCL(PIN_RTC_SCL))   { busName = "Wire";  return &Wire; }
  if (Wire1.setSDA(PIN_RTC_SDA) && Wire1.setSCL(PIN_RTC_SCL)) { busName = "Wire1"; return &Wire1; }
  return nullptr;
}

void scanI2C() {
  Serial.print(F("I2C bus: ")); Serial.println(busName);
  Serial.println(F("I2C scan:"));
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    bus->beginTransmission(a);
    if (bus->endTransmission() == 0) {
      Serial.print(F("  device at 0x")); Serial.print(a, HEX);
      if (a == 0x68) Serial.print(F("  (DS3231)"));
      if (a == 0x57) Serial.print(F("  (AT24C32 EEPROM)"));
      Serial.println();
      found++;
    }
  }
  if (!found) Serial.println(F("  nothing found: check VCC, GND, SDA, SCL"));
}

void printHelp() {
  Serial.println(F("c = set clock to compile time | T2026-11-05 18:30:00 = set exact time | i = scan | h = help"));
}

void setClock(const DateTime& dt) {
  rtc.adjust(dt);
  Serial.print(F("clock set, oscillator stopped flag is now: "));
  Serial.println(rtc.lostPower() ? F("YES (unexpected, try again)") : F("no"));
}

void handleLine(const char* line) {
  if (line[0] == 'c' && line[1] == '\0') {
    setClock(DateTime(F(__DATE__), F(__TIME__)));
  } else if (line[0] == 'T') {
    int y, mo, d, h, mi, s;
    if (sscanf(line + 1, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s) == 6 &&
        mo >= 1 && mo <= 12 && d >= 1 && d <= 31 && h >= 0 && h < 24 && mi >= 0 && mi < 60 && s >= 0 && s < 60) {
      setClock(DateTime(y, mo, d, h, mi, s));
    } else {
      Serial.println(F("could not read that, use: T2026-11-05 18:30:00"));
    }
  } else if (line[0] == 'i' && line[1] == '\0') {
    scanI2C();
  } else if (line[0] == 'h' && line[1] == '\0') {
    printHelp();
  } else if (line[0] != '\0') {
    Serial.println(F("unknown command, h for help"));
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}
  Serial.println(F("\nRTC test (DS3231)."));
  printHelp();

  bus = pickI2C();
  if (!bus) {
    Serial.println(F("I2C pin setup failed for both Wire and Wire1"));
    return;
  }
  Serial.print(F("I2C bus: ")); Serial.println(busName);
  bus->begin();
  scanI2C();

  rtcOk = rtc.begin(bus);
  if (!rtcOk) {
    Serial.println(F("DS3231 not found at 0x68. Check wiring (VCC 3V3, GND, SDA GPIO2, SCL GPIO3)."));
    return;
  }
  Serial.println(F("DS3231 found."));
  Serial.println(F("Note: this sketch never changes the time by itself, only on c or T commands."));
}

void runCommand(char* line, uint8_t& n) {
  line[n] = '\0';
  n = 0;
  if (!line[0]) return;
  Serial.print(F("> ")); Serial.println(line);   // echo what was received
  if (rtcOk || line[0] == 'i' || line[0] == 'h') handleLine(line);
  else Serial.println(F("RTC not found, only i and h work"));
}

void loop() {
  // A command ends at Enter, or when no new character has arrived for 300 ms
  // (some serial monitors send the text without a line ending).
  static char line[40];
  static uint8_t n = 0;
  static uint32_t lastCharAt = 0;
  while (Serial.available()) {
    char c = Serial.read();
    lastCharAt = millis();
    if (c == '\r' || c == '\n') runCommand(line, n);
    else if (n < sizeof(line) - 1) line[n++] = c;
  }
  if (n > 0 && millis() - lastCharAt > 300) runCommand(line, n);

  static uint32_t last = 0;
  if (rtcOk && millis() - last >= 1000) {
    last = millis();
    DateTime now = rtc.now();
    char buf[80];
    snprintf(buf, sizeof buf, "%04d-%02d-%02d %02d:%02d:%02d   unix %lu   ",
             now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(),
             (unsigned long)now.unixtime());
    Serial.print(buf);
    Serial.print(rtc.getTemperature(), 2);
    Serial.print(F(" C   oscillator stopped: "));
    Serial.println(rtc.lostPower() ? F("YES (time not trusted)") : F("no"));
  }
}
