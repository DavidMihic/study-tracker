#include "drivers/encoder.h"
#include "config/pins.h"

namespace {

// State table from the verified bring-up test (encoder_decoder_test.cpp).
// Rows: current state. Columns: pin pattern (B << 1) | A.
enum : uint8_t { R_START, R_CW_FINAL, R_CW_BEGIN, R_CW_NEXT, R_CCW_BEGIN, R_CCW_FINAL, R_CCW_NEXT };
constexpr uint8_t kDirCw  = 0x10;
constexpr uint8_t kDirCcw = 0x20;

const uint8_t kTable[7][4] = {
  /* R_START     */ {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  /* R_CW_FINAL  */ {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | kDirCw},
  /* R_CW_BEGIN  */ {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  /* R_CW_NEXT   */ {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  /* R_CCW_BEGIN */ {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  /* R_CCW_FINAL */ {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | kDirCcw},
  /* R_CCW_NEXT  */ {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

uint8_t state = R_START;

}  // namespace

namespace Encoder {

void begin() {
  pinMode(Pins::kEncA, INPUT_PULLUP);
  pinMode(Pins::kEncB, INPUT_PULLUP);
}

int8_t poll() {
  uint8_t pins = (digitalRead(Pins::kEncB) << 1) | digitalRead(Pins::kEncA);
  state = kTable[state & 0x0F][pins];
  uint8_t direction = state & 0x30;
  state &= 0x0F;
  if (direction == kDirCw)  return +1;
  if (direction == kDirCcw) return -1;
  return 0;
}

}  // namespace Encoder
