// DEMO ONLY, to be deleted before the gift is finished: fills the session log with made-up study blocks
// so the statistics screen has something to show. Everything in this folder is compiled only in the
// `feather_demo` environment (it defines DUMMY_DATA); the normal `feather` build does not contain it.
//
// To remove it for good: delete src/debug/, the `#ifdef DUMMY_DATA` block in main.cpp and the
// [env:feather_demo] section in platformio.ini. To wipe the demo records from a device: Postavke >
// Poništi brojače > Svi predmeti (this clears the session log too).
#pragma once
#include <stdint.h>

namespace DummyData {

// Does nothing unless the session log is empty. nowUnix = the RTC time (0 = clock missing: does nothing).
// Writes about 40 days of blocks ending at nowUnix, on subjects of the current semester. Deterministic:
// the same day always gets the same blocks.
void seedIfEmpty(uint32_t nowUnix);

}  // namespace DummyData
