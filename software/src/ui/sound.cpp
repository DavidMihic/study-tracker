#include "ui/sound.h"
#include "data/progress.h"
#include "drivers/buzzer.h"

namespace Sound {

void turn()      { if (Progress::soundTurn())  Buzzer::play(Buzzer::Pattern::Click); }
void press()     { if (Progress::soundPress()) Buzzer::play(Buzzer::Pattern::Short); }
void pressLong() { if (Progress::soundPress()) Buzzer::play(Buzzer::Pattern::Long); }
void nudge()     { if (Progress::soundNudge()) Buzzer::play(Buzzer::Pattern::Nudge); }

}  // namespace Sound
