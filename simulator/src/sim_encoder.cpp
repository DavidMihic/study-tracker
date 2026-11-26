// SIMULATOR ONLY. Replaces drivers/encoder.cpp: the page reports whole dial clicks.
#include "drivers/encoder.h"
#include "sim.h"

namespace {
int pending = 0;
}

namespace Encoder {

void begin() {}

int8_t poll() {
  if (pending > 0) { pending--; return 1; }
  if (pending < 0) { pending++; return -1; }
  return 0;
}

}  // namespace Encoder

namespace Sim {
void turn(int clicks) { pending += clicks; }
}
