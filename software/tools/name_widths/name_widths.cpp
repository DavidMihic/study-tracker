// Prints how wide every subject name is in each UI font, and which names do not fit where they are
// shown. Run it after editing subjects.cpp or after changing fonts/layout. Run: tools/name_widths/build.sh (add -v to list the names)
//
// The widths come from the SAME u8g2 font data and width code the device uses (the library's own
// .cpp is compiled with a stub Adafruit_GFX), so they match the display exactly.
#include <stdio.h>
#include <string.h>
#include "U8g2_for_Adafruit_GFX.h"
#include "data/subjects.h"

extern const uint8_t u8g2_font_helvR12_te[], u8g2_font_helvB14_te[], u8g2_font_helvB18_te[];

namespace {

U8G2_FOR_ADAFRUIT_GFX fonts;
Adafruit_GFX gfx;

int width(const uint8_t* font, const char* text) {
  fonts.setFont(font);
  return fonts.getUTF8Width(text);
}

// Room for the text in each place a subject name is drawn (keep in step with the screens' configure() calls).
// A text field's room = its width - 2 * padding - icon slot (22 px).
constexpr int kPickerRow = 320 - 2 * 6;           // picker row, Body font, no star and no study time
constexpr int kPickerRowStarTime = 320 - 2 * 6 - 22 - 70;   // with a star and a time such as "12 h 40 min"
constexpr int kHeading = 300;                     // Home / Studying subject line, Heading font
constexpr int kStatsRow = 210;                    // statistics row name, Body font
constexpr int kCentered = 320;                    // reset screen name, Body font

}  // namespace

int main(int argc, char**) {
  const bool listNames = argc > 1;   // any argument = also list every name that does not fit a picker row
  fonts.begin(gfx);
  const size_t n = Subjects::count();
  int overPicker = 0, overPickerBusy = 0, overHeading = 0, overHeadingBody = 0, overStats = 0, overCentered = 0;
  size_t longestBytes = 0;
  const char* widest = "";
  int widestPx = 0;

  for (size_t i = 0; i < n; i++) {
    const char* name = Subjects::at(i).name;
    const int body = width(u8g2_font_helvB14_te, name);
    const int heading = width(u8g2_font_helvB18_te, name);
    if (body > kPickerRow) overPicker++;
    if (body > kPickerRowStarTime) overPickerBusy++;
    if (heading > kHeading) overHeading++;
    if (body > kHeading) overHeadingBody++;
    if (body > kStatsRow) overStats++;
    if (body > kCentered) overCentered++;
    if (strlen(name) > longestBytes) longestBytes = strlen(name);
    if (body > widestPx) { widestPx = body; widest = name; }
    if (listNames && (body > kPickerRow || heading > kHeading)) {
      printf("%4u %-4s body %4d px  heading %4d px  %s\n", Subjects::at(i).id,
             body > kPickerRow ? "ROW" : "", body, heading, name);
    }
  }

  printf("\n%u subjects, longest name %u bytes (limit %u), widest in Body font %d px:\n  %s\n", (unsigned)n,
         (unsigned)longestBytes, (unsigned)Subjects::kMaxNameBytes, widestPx, widest);
  printf("Names wider than the room (they end in \"...\", or slide when focused in a list):\n");
  printf("  picker row, plain:            %3d\n", overPicker);
  printf("  picker row, star + study time:%3d\n", overPickerBusy);
  printf("  Home/Studying line (Heading): %3d   (would still be %d in the Body font)\n", overHeading, overHeadingBody);
  printf("  statistics row (Body):        %3d\n", overStats);
  printf("  reset screen (Body):          %3d\n", overCentered);
  return 0;
}
