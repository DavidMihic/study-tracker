// Screen manager. Routes input events to the active screen and lets it redraw.
// Later build steps add the other screens (subject select, studying, break, stats, ...) here.
#pragma once

namespace App {

void begin();
void update();

}  // namespace App
