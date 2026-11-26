// SIMULATOR ONLY. The small part of the Arduino API that the firmware and its graphics libraries use,
// so they compile to WebAssembly. The functions are implemented in src/sim_arduino.cpp.
#pragma once
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <string>

#include "Print.h"

using std::max;
using std::min;

typedef bool boolean;
typedef uint8_t byte;

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define PROGMEM
#define PGM_P const char*
#define pgm_read_byte(address) (*(const uint8_t*)(address))
#define pgm_read_word(address) (*(const uint16_t*)(address))
#define pgm_read_dword(address) (*(const uint32_t*)(address))
#define radians(degrees) ((degrees) * 0.017453292519943295)

#define constrain(value, low, high) ((value) < (low) ? (low) : ((value) > (high) ? (high) : (value)))

uint32_t millis();
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);
void analogWrite(uint8_t pin, int value);
void analogWriteFreq(uint32_t hz);
