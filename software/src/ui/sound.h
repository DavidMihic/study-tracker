// Every beep of the user interface goes through here, so the sound settings (Progress::sound...) are
// respected in one place. Screens never call the buzzer driver directly.
#pragma once

namespace Sound {

void turn();        // tick for one click of the dial
void press();       // short beep: a button press / a choice was made
void pressLong();   // long beep: something finished (block stopped, clock saved)
void nudge();       // the three soft beeps at the target time
void powerRestored();   // three longer beeps: power came back while a study block was running

}  // namespace Sound
