// Display bring-up: ST7789 240x320 on SPI. No encoder needed, everything is controlled over serial.
// Env: display_test (see platformio.ini). Serial monitor 115200.
//
// Wiring (GPIO numbers, silk in brackets):
//   LCD SCL -> GPIO18 [SCK]   LCD SDA -> GPIO19 [MO]   LCD CS -> GPIO10 [10]
//   LCD DC  -> GPIO11 [11]    LCD RES -> GPIO12 [12]   LCD BL -> see BL_MODE
//   LCD VDD -> 3V3            LCD GND -> GND
//
// The test screens auto-advance every 4 s until you navigate manually (n, c, o) or send a.
// Commands:
//   n = next screen        c = next solid colour (black, white, red, green, blue)
//   a = auto-advance on/off (n, c, o turn it off; s, i, +, - leave it as it is)
//   o = next rotation      i = toggle colour inversion
//   s = next SPI speed     + / - = backlight brightness (only if BL_MODE is 1 or 2)
//   h = help
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Backlight (LCD BL pin):
//   0 = BL wired straight to 3V3 (no control). START HERE.
//   1 = BL switched by Q1 (PNP): GPIO LOW = on, so PWM is inverted.
//   2 = BL driven straight from the GPIO: HIGH = on. Only if the BL current is small.
#define BL_MODE 2   // measured BL pin current is about 1 mA, so a GPIO can drive it directly

constexpr uint8_t PIN_LCD_SCK  = 18;
constexpr uint8_t PIN_LCD_MOSI = 19;
constexpr uint8_t PIN_LCD_CS   = 10;
constexpr uint8_t PIN_LCD_DC   = 11;
constexpr uint8_t PIN_LCD_RST  = 12;
constexpr uint8_t PIN_LCD_BL   = 9;
constexpr uint8_t PIN_BUZZER   = 25;   // active, low-level trigger: keep HIGH so it stays silent

Adafruit_ST7789 tft(&SPI, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);

const uint32_t SPEEDS[] = {8000000UL, 16000000UL, 32000000UL, 62500000UL};
constexpr uint8_t N_SPEEDS = sizeof(SPEEDS) / sizeof(SPEEDS[0]);
uint8_t speedIdx = 0;                  // start slow (8 MHz): safest on breadboard/jumper wiring

constexpr uint8_t N_PATTERNS = 5;
uint8_t pattern = 0;
uint8_t rotation = 1;                  // landscape
uint8_t solidIdx = 0;
bool inverted = false;                 // only meaningful after the first 'i'
bool invertTouched = false;
uint8_t brightness = 255;
bool autoAdvance = true;
uint32_t lastAdvance = 0;

const uint16_t SOLIDS[] = {ST77XX_BLACK, ST77XX_WHITE, ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE};
const char* SOLID_NAMES[] = {"black", "white", "red", "green", "blue"};

void applyBacklight() {
#if BL_MODE == 1
  analogWrite(PIN_LCD_BL, 255 - brightness);
#elif BL_MODE == 2
  analogWrite(PIN_LCD_BL, brightness);
#endif
}

void printStatus() {
  Serial.print(F("screen ")); Serial.print(pattern);
  Serial.print(F("  rotation ")); Serial.print(rotation);
  Serial.print(F("  size ")); Serial.print(tft.width()); Serial.print('x'); Serial.print(tft.height());
  Serial.print(F("  SPI ")); Serial.print(SPEEDS[speedIdx] / 1000000UL); Serial.print(F(" MHz"));
  Serial.print(F("  inversion "));
  Serial.print(invertTouched ? (inverted ? F("forced ON") : F("forced OFF")) : F("library default"));
  Serial.print(F("  brightness ")); Serial.println(brightness);
}

// ---- screens ----
void labelBar(int x, int y, int w, const char* txt, uint16_t fg) {
  tft.setTextSize(2);
  tft.setTextColor(fg);
  tft.setCursor(x + (w - 12 * (int)strlen(txt)) / 2, y);
  tft.print(txt);
}

// 0: colour bars. Top row should read RED GREEN BLUE WHITE in those colours.
void drawBars() {
  const int W = tft.width(), H = tft.height();
  const uint16_t top[4] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_WHITE};
  const char* topN[4] = {"RED", "GRN", "BLU", "WHT"};
  const uint16_t bot[4] = {ST77XX_CYAN, ST77XX_MAGENTA, ST77XX_YELLOW, ST77XX_BLACK};
  const char* botN[4] = {"CYN", "MAG", "YEL", "BLK"};
  int bw = W / 4;
  for (int i = 0; i < 4; i++) {
    tft.fillRect(i * bw, 0, bw, H / 2, top[i]);
    tft.fillRect(i * bw, H / 2, bw, H - H / 2, bot[i]);
    labelBar(i * bw, H / 4 - 8, bw, topN[i], i == 3 ? ST77XX_BLACK : ST77XX_WHITE);
    labelBar(i * bw, H * 3 / 4 - 8, bw, botN[i], i == 3 ? ST77XX_WHITE : ST77XX_BLACK);
  }
}

// 1: geometry. White border on the very edge, red border one pixel in, corner labels,
// and a big F (shows mirroring/rotation).
void drawGeometry() {
  const int W = tft.width(), H = tft.height();
  tft.drawRect(0, 0, W, H, ST77XX_WHITE);
  tft.drawRect(1, 1, W - 2, H - 2, ST77XX_RED);
  tft.drawLine(0, 0, W - 1, H - 1, ST77XX_WHITE);
  tft.drawLine(W - 1, 0, 0, H - 1, ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(4, 4);                    tft.print(F("TOP-LEFT"));
  tft.setCursor(W - 4 - 12 * 9, 4);       tft.print(F("TOP-RIGHT"));
  tft.setCursor(4, H - 20);               tft.print(F("BOTTOM-LEFT"));
  tft.setCursor(W - 4 - 12 * 12, H - 20); tft.print(F("BOTTOM-RIGHT"));
  tft.setTextSize(8);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(W / 2 - 24, H / 2 - 28);  tft.print('F');
  char buf[24];
  snprintf(buf, sizeof buf, "%dx%d rot %u", W, H, rotation);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(W / 2 - 6 * (int)strlen(buf), H / 2 + 40);
  tft.print(buf);
}

// 2: colour ramps (red, green, blue, grey). Look for banding or a wrong channel order.
void drawRamps() {
  const int W = tft.width(), H = tft.height();
  int rowH = H / 4;
  for (int x = 0; x < W; x++) {
    uint8_t v = (uint32_t)x * 255 / (W - 1);
    tft.fillRect(x, 0 * rowH, 1, rowH, tft.color565(v, 0, 0));
    tft.fillRect(x, 1 * rowH, 1, rowH, tft.color565(0, v, 0));
    tft.fillRect(x, 2 * rowH, 1, rowH, tft.color565(0, 0, v));
    tft.fillRect(x, 3 * rowH, 1, H - 3 * rowH, tft.color565(v, v, v));
  }
}

// 3: text sizes on black, for readability at arm's length.
void drawText() {
  tft.setTextColor(ST77XX_WHITE);
  int y = 6;
  for (uint8_t sz = 1; sz <= 5; sz++) {
    tft.setTextSize(sz);
    tft.setCursor(6, y);
    // 11 characters * 6 px * size must fit in the 320 px width: size 5 needs a shorter string
    tft.print(sz < 5 ? F("Study 12:34") : F("12:34"));
    y += 8 * sz + 6;
  }
}

// 4: solid colour (pick with 'c'). Good for dead pixels and backlight bleed.
void drawSolid() { tft.fillScreen(SOLIDS[solidIdx]); }

void drawPattern() {
  uint32_t t0 = millis();
  tft.fillScreen(ST77XX_BLACK);
  uint32_t fillMs = millis() - t0;
  switch (pattern) {
    case 0: drawBars(); break;
    case 1: drawGeometry(); break;
    case 2: drawRamps(); break;
    case 3: drawText(); break;
    case 4: drawSolid(); Serial.print(F("solid: ")); Serial.println(SOLID_NAMES[solidIdx]); break;
  }
  uint32_t totalMs = millis() - t0;
  printStatus();
  Serial.print(F("  full-screen black fill: ")); Serial.print(fillMs);
  Serial.print(F(" ms,  whole screen drawn in ")); Serial.print(totalMs); Serial.println(F(" ms"));
}

void applySpeed() { tft.setSPISpeed(SPEEDS[speedIdx]); }

void printHelp() {
  Serial.println(F("a auto-advance on/off | n next screen | c next solid colour | o next rotation | i toggle inversion"));
  Serial.println(F("s next SPI speed | + / - brightness (BL_MODE 1/2) | h help"));
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, HIGH);
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}

#if BL_MODE != 0
  pinMode(PIN_LCD_BL, OUTPUT);
  analogWriteFreq(20000);   // above hearing range
  applyBacklight();
#endif

  SPI.setSCK(PIN_LCD_SCK);
  SPI.setTX(PIN_LCD_MOSI);
  tft.init(240, 320);       // sets its own default SPI speed, so change it afterwards
  applySpeed();
  tft.setRotation(rotation);
  Serial.println(F("\nDisplay test."));
  printHelp();
  drawPattern();
  lastAdvance = millis();
}

void loop() {
  if (autoAdvance && millis() - lastAdvance > 4000) {
    lastAdvance = millis();
    pattern = (pattern + 1) % N_PATTERNS;
    drawPattern();
  }

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') continue;
    switch (c) {
      case 'a': autoAdvance = !autoAdvance; lastAdvance = millis(); Serial.println(autoAdvance ? F("auto-advance ON") : F("auto-advance OFF")); break;
      case 'n': autoAdvance = false; pattern = (pattern + 1) % N_PATTERNS; drawPattern(); break;
      case 'c': autoAdvance = false; solidIdx = (solidIdx + 1) % 5; pattern = 4; drawPattern(); break;
      case 'o': autoAdvance = false; rotation = (rotation + 1) % 4; tft.setRotation(rotation); drawPattern(); break;
      case 'i':
        inverted = invertTouched ? !inverted : false;   // first press forces OFF (the fix named in CLAUDE.md)
        invertTouched = true;
        tft.invertDisplay(inverted);
        drawPattern();
        break;
      case 's': speedIdx = (speedIdx + 1) % N_SPEEDS; applySpeed(); drawPattern(); break;
#if BL_MODE != 0
      case '+': brightness = min(255, brightness + 16); applyBacklight(); printStatus(); break;
      case '-': brightness = max(8, brightness - 16); applyBacklight(); printStatus(); break;
#endif
      case 'h': printHelp(); break;
      default: break;
    }
  }
}
