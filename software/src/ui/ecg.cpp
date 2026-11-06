#include "ui/ecg.h"

namespace Ecg {

namespace {

struct Point { int x; int y; };

// Corner points of one beat; the trace is a straight line between neighbours.
// Small bump (P), tiny dip, tall spike (R), dip (S), rest, rounded bump (T), rest.
constexpr Point kShape[] = {
  {0, 0}, {14, 0}, {20, -6}, {26, 0}, {38, 0}, {42, 5}, {46, -kMaxUp}, {51, kMaxDown}, {56, 0},
  {70, 0}, {80, -9}, {90, 0}, {kBeatWidth - 1, 0},
};
constexpr int kPoints = sizeof kShape / sizeof kShape[0];

}  // namespace

int offset(int phase) {
  if (phase <= 0) return 0;
  for (int i = 1; i < kPoints; i++) {
    if (phase <= kShape[i].x) {
      const Point& a = kShape[i - 1];
      const Point& b = kShape[i];
      return a.y + (b.y - a.y) * (phase - a.x) / (b.x - a.x);
    }
  }
  return 0;
}

}  // namespace Ecg
