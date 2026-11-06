// The heartbeat line of the screensaver, as a function: how far above/below the baseline the trace is at
// a given position inside one beat. Plain C++ (no Arduino), so the PC tests can check it.
#pragma once

namespace Ecg {

constexpr int kBeatWidth = 110;   // pixels of one beat (P wave, QRS spike, T wave, rest)
constexpr int kMaxUp     = 46;    // highest point of the spike, pixels above the baseline
constexpr int kMaxDown   = 16;   // lowest point (the dip after the spike), pixels below the baseline

// Vertical offset in pixels for phase 0..kBeatWidth-1; negative = above the baseline (screen y grows
// downwards). Phase 0 and phase kBeatWidth-1 are both on the baseline, so beats join without a step.
int offset(int phase);

}  // namespace Ecg
