# CLAUDE.md: Study Tracker

Project context for Claude Code. Read this fully before changing anything. Update the "Status" and "Open items" sections whenever they change.

## 1. What this is

A desk gadget David is building as a **gift for his girlfriend, a medical student**. Repo: https://github.com/DavidMihic/study-tracker

History of the idea (matters for design decisions):
- It started as a Pomodoro timer. She rejected it because **forced interruptions** break her flow.
- Current concept: a **soft Pomodoro + study tracker**. Nothing may ever force-stop or interrupt her.
- She is **not technical**. UI must be obvious: one dial, one button, clear screens. No exam-countdown-style features.

### Intended behaviour
1. **Study session**: a count-up timer. At the target time (default ~25 min, configurable) it gives a **gentle nudge** (soft beep and/or visual cue) but keeps counting. The session ends only when she **presses the dial**. Then a break phase starts (count-down or count-up, to be decided).
2. **Study tracker**: a scrollable list of subjects, **hard-coded in firmware for all future semesters**. Per subject, store the **number of sessions and total time**. Save to flash **only when a session ends** (limits flash wear).
3. **RTC (DS3231)**: clock, daily and weekly views. Store the **session start timestamp** so a session can be **resumed after power loss** (device is USB-powered only, so it may be unplugged mid-session).
4. **Screensaver** after idle: ECG-heartbeat animation plus clock.
5. Buzzer for nudges and UI feedback. Backlight dimming via PWM (depends on the hardware decision in section 4).

## 2. Hard decisions already made (do not relitigate without asking)

- **MCU: Adafruit Feather RP2040** (the plain one, with headers soldered on). Not ESP32, not Pico.
- **No battery. USB-C powered only.** David owns a 3000 mAh Li-ion cell but kept it for another project. The battery section of the schematic is preserved in git: tag `schematic-with-battery`. **Do not add battery code** (no VBAT divider, no percent display, no charge logic). Note there is no battery fuel gauge on this Feather anyway.
- **Display: 2.0" ST7789, 240x320, SPI** (Estardyn module from AliExpress). The Nokia 5110 and the 3461AS 7-segment display were only used as bench readouts and are **not** part of the design.
- **Input: one EC11 rotary encoder with push switch** (cheap, AliExpress, bare part, not a breakout board). No pots, no extra buttons.
- **Buzzer: MH-FMD active buzzer module, low-level trigger.**
- **RTC: DS3231 module** (ZS-042-style, AT24C32 EEPROM on board). A DS1307 was rejected because it is 5 V only.
- **Power switch: SS12D00 SPDT slide switch** wired between the Feather's **EN pin and GND**. Switch closed = EN low = 3.3 V regulator off. Note USB/BAT pins stay powered; it is a soft "off" only.
- **Toolchain: PlatformIO in VS Code**, arduino-pico core (Earle Philhower), C++. Not the Arduino IDE.
- **Learning-oriented, budget-friendly.** David knows C++, Altium and CAD, owns a Prusa printer. Prefer clear, readable code over clever code, and explain non-obvious choices briefly.
- Enclosure: 3D-printed, with a **bottom panel held by 4 screws into heat-set inserts**. The Feather is mounted upside-down so removing the bottom panel exposes it (BOOTSEL is on the opposite side from the USB-C port).

## 3. Toolchain

- `upload_protocol = mbed` performs an automatic 1200-baud reset into the bootloader and copies the UF2. **No BOOTSEL press is needed normally.** BOOTSEL is only a fallback if the sketch hung, USB is disabled, or deep sleep is active.
- The default picotool route failed on David's Windows machine (needs a Zadig driver). Do not switch back to it.
- Serial: guard prints with `if (Serial)` so the device runs fine with no PC attached.

## 4. Hardware reference

### Pin map (GPIO numbers; the board silkscreen differs, see gotchas)

| Function | Net | GPIO | Feather silk |
|---|---|---|---|
| LCD clock | LCD_SCK | 18 | SCK |
| LCD data (MOSI) | LCD_MOSI | 19 | MO |
| LCD chip select | LCD_CS | 10 | 10 |
| LCD data/command | LCD_DC | 11 | 11 |
| LCD reset | LCD_RST | 12 | 12 |
| LCD backlight | LCD_BL | 9 | 9 |
| Encoder A | ENC_A | 7 | 5 |
| Encoder B | ENC_B | 8 | 6 |
| Encoder push | ENC_SW | 24 | 24 |
| Buzzer | BUZZ | 25 | 25 |
| RTC SDA | RTC_SDA | 2 | SDA |
| RTC SCL | RTC_SCL | 3 | SCL |
| Power switch | RP_EN | n/a | EN to GND |

Free pins: A0..A3 (GPIO26..29), MISO (20), RX (1), TX (0), silk 4 (GPIO6), D13.

### Gotchas (all learned the hard way)
- **Silkscreen is not GPIO.** Silk 4 = GPIO6, silk 5 = GPIO7, silk 6 = GPIO8, silk 9..13 = GPIO9..13, A0..A3 = GPIO26..29, SCK/MO/MI = 18/19/20. The code uses GPIO numbers.
- **I2C:** SDA/SCL pins are I2C1 on this board, in practice `Wire` works with `setSDA(2)`/`setSCL(3)` (verified on the bench); `Wire1` is only a fallback. The bring-up sketch tries both (`pickI2C()`).
- **PWM slice sharing:** GPIO9 (backlight) and GPIO25 (buzzer) are both on PWM slice 4 channel B. Do not run PWM on both at different frequencies. The buzzer is active (no PWM tone needed), so drive it digitally.
- **Buzzer is active and low-level triggered:** HIGH = silent, LOW = beep. Power it from **3V3, not 5V**. GPIO pins default low at boot, which would beep, so schematic has **R5 = 4.7k pull-up to 3V3** on BUZZ and the code sets HIGH first in `setup()`. The sticker on the buzzer must be peeled (it was the cause of the faint sound).
- **ST7789 library inverts colours by default** on these modules; if colours look wrong use `tft.invertDisplay(false)` (`TFT_UNINVERT` toggle in bring-up sketch). Use `tft.init(240, 320)`, set SPI speed **after** init (16 MHz works as default), `setRotation(1)` for landscape.
- **Backlight:** options chosen by `BL_MODE` in the bring-up sketch: 0 = BL pin tied to 3V3 (always on); 1 = via Q1 PNP high-side (GPIO LOW = on, PWM must be inverted: `analogWrite(pin, 255 - brightness)`); 2 = direct from GPIO (HIGH = on). **Backlight current has not been measured yet** (meter in series from 3V3 to BL). If it is under about 10 mA, Q1 can be skipped. Use `analogWriteFreq(20000)` to avoid audible whine.
- **Display, verified on the bench (2026-11-05):** colours, rotation (`setRotation(1)`, 320x240 landscape) and inversion at the library default are correct. Bring-up code: `software/bringup/display_test.cpp` (env `display_test`; control over serial, `h` for help). The datasheet `ST7789VI.PDF` (Sitronix v1.4, in the repo root) has no instant-refresh mode: pixels are written serially into the controller's frame memory, and the TE (tearing-effect) pin is not broken out on this 8-pin module, so the sweep cannot be synced away. Min write clock cycle is 16 ns (62.5 MHz) per the datasheet. At size 5 text, 11 characters (330 px) do not fit the 320 px width, so keep strings short in the UI (6 px per character per text size).
- **DS3231 module:** I2C addresses 0x68 (RTC) and 0x57 (EEPROM). Onboard 4.7k pull-ups. Has a charging diode plus 200 ohm ("201") path to the coin cell. At 3.3 V with a CR2032 it is harmless; optionally remove the diode for the final build. RTClib: `rtc.begin(&Wire)` (see the verified note in open item 5); if `lostPower()` set to compile time. Pins 32K and SQW are unconnected.
- **EN pin low** disables the 3.3 V regulator only; the Feather does not draw from USB for the MCU then, but BAT/USB pins stay live.
- **3V3 regulator limit:** about 500 mA peak. Current budget is comfortably below that.
- BOOTSEL is on GPIO4 (Rev D+) and is **not exposed**; RESET/BOOTSEL are only reachable by removing the bottom panel.

### Schematic (Altium, final, reviewed)
Blocks/parts: U1 Feather; SW1 SS12D00 (EN to GND); BZ1 MH-FMD + R5 4k7 pull-up; J1 ST7789 header (pins in order: GND, VDD, SCL, SDA, RES, DC, CS, BL); Q1 BC557 + R3 1k (base) + R4 10k (pull-up from base to emitter side) for optional backlight switching; SW2 EC11 with C2..C4 100 nF (debounce, from A, B, SW to GND; see EC11 note, they are not fitted); U2 DS3231 module. Power ports 3V3 and GND. The battery section exists only in the tagged commit.

Display to perfboard: 8-pin female header on the perfboard plus short male-to-female Dupont jumpers with strain relief (the module cannot be soldered flat because it is angled). Male header is soldered on the display module.

- **EC11:** do not fit debounce caps on A/B (the encoder skipped clicks with them). Measurements and wiring: skill `hardware-notes`.

## 5. Firmware plan

Principles:
- **Non-blocking**: `millis()`-based state machine in `loop()`; no `delay()` outside bring-up code.
- Debounce in software; inputs polled or interrupt-fed into an event queue (encoder delta, button click, long press).
- Encoder decoder: full-step quadrature state table (bounce tolerant), with a `STEPS_PER_DETENT` constant to adapt to the real part once the A/B sequence is logged. Push switch: 25 ms debounce, long press 800 ms.
- UI via Adafruit GFX; **never redraw the whole screen** (decision 2026-11-05): redraw only changed regions. A full-screen fill takes 239 / 179 / 120 / 120 ms at 8 / 16 / 32 / 62.5 MHz SPI, which shows as a visible sweep. Use **32 MHz** as the default SPI speed (62.5 MHz is no faster; the Adafruit library is CPU-bound, not SPI-bound). Future option, not needed now: draw into a RAM framebuffer (153.6 KB of the RP2040's 264 KB) and push it in one bulk SPI transfer, or switch to a DMA-capable library; revisit only if the real UI looks bad.

**Only `drivers/` touches hardware**; `data/`, `app/session`, `app/semester`, `app/subject_list`, `ui/format|strings|colors` contain no Arduino code on purpose, so the PC tests can build them. Includes are relative to `src/`. All displayed text is in `ui/strings.h`; every beep goes through `ui/sound`. Bring-up sketches stay in `software/bringup/` with their own envs.

Build order: (1) drivers + demo home screen [done]; (2) subjects + session + screens + card menus + a basic settings screen, RAM only at the time [**written, compiles, 34 PC tests pass, partly seen on the device**]; (3) flash storage [**written, PC-tested, verified on the device by David (2026-11-06)**]; (4) stats screen [**seen on the device and approved by David, 2026-11-06, incl. the 'Statistika' card**], settings polish; (5) screensaver [**seen on the device and approved by David, 2026-11-06 (ECG trace thickened)**]. Also approved on the device: gear icon, nudge. David plans to restyle the UI later (not urgent). **Firmware is considered feature-complete for now.** Remaining before the gift is wrapped up: reset all counters before real use, then remove the demo data (`src/debug/`, `#ifdef DUMMY_DATA` in `main.cpp`, `[env:feather_demo]`). David is now doing the case design and perfboard wiring himself (open items 9 and 10).

**Language (decision 2026-11-05):** everything shown on the display is Croatian with diacritics (č ć š đ ž); code, names and comments are English. All text lives in `ui/strings.h`. The Adafruit built-in font cannot draw these letters, so text uses the `U8g2_for_Adafruit_GFX` library with u8g2 fonts (`display.cpp`: helvR12_te, helvB14_te, helvB18_te, logisoso42_tn). I decoded the font data and confirmed the `_te` fonts contain all ten letters (and that `_tr` fonts do not); `logisoso42_tn` has only digits, ':' and space (used for the big timers). Still to confirm by eye on the real display. Fonts are fixed pixel sizes, so layouts use pixel positions, not text sizes.

**Subjects, controls, screensaver:** details in skill `ui-and-subjects`. Never delete a subject id whose saved data should be kept; keep ids and names in `subjects.json` and `subjects.cpp` identical.

**Š and Ž (2026-11-25):** the u8g2 fonts squeeze these capitals so they look lowercase, so `Display::drawText` / `textWidth` draw them as S / Z plus a caron bitmap (`drivers/display.cpp`; redo the bitmaps if a font changes). `tools/name_widths` still measures the raw glyphs, so its widths can differ slightly for such names. Not yet checked on the device.

**u8g2 gotcha:** `U8G2_FOR_ADAFRUIT_GFX::setFont()` resets the text to opaque mode (black box behind every letter) whenever the font changes, so `Display::drawText` calls `setFontMode(1)` after every `setFont()`.

**PC tests:** `pio test -e native`, plus `pio test -e native_curriculum` on the real subject list (run it after editing `subjects.cpp`). The `native` tests use their own subject list (`test/support/test_subjects.cpp`). Screens and drivers are not covered.

**Storage, statistics, power-cut recovery:** details in skill `storage-and-power-cut`. If the save layout changes, bump `kVersion` and add a migration. Never discard recorded study time.


## 6. Status (as of 2026-11-05)

Components arrived; David is doing **bench bring-up on a breadboard, no soldering yet** (only the male header on the display is soldered). Bring-up sketch: `bringup2/main.cpp` (toggles `TEST_DISPLAY`, `TEST_ENCODER`, `TEST_BUZZER`, `TEST_RTC`, `BL_MODE`, `TFT_UNINVERT`, `TFT_SPI_HZ`). It matches the final schematic's pin map and was compile-checked against stubs only, **not yet run on hardware beyond partial tests**.

Done and verified: board choice, upload workflow, buzzer verified (sticker removed, silent at boot, patterns tested), schematic reviewed, display header soldered, power switch (EN to GND) verified, RTC verified, encoder push switch verified on hardware (see below), display works on breadboard (colours, rotation, backlight dimming, SPI up to 32 MHz), encoder rotation verified (decoder test, 20 clicks per revolution, no misses).

**After soldering (2026-11-25):** the display module is mounted turned by 180 degrees (neater cables), so the firmware uses `Config::kRotation = 3` (the breadboard notes saying `setRotation(1)` describe the old mounting; `bringup/display_test.cpp` still starts at its own rotation). Uploaded to the soldered device, David says it looks good. **RTC problem, fixed (2026-11-25):** 'Sat nije pronađen' was caused by SDA and SCL being swapped in the soldering. The set-clock screen still discards the time without saying so when the DS3231 does not answer.

## 7. Open items

0. (a) ~~DROPPED 2026-11-06: David accepts a few minutes of counting error after a power cut; the DS3231 EEPROM 'still running' flag is not built~~; (b) ~~saving to TWO flash sectors~~ done 2026-11-06 differently: LittleFS with atomic replace plus the per-day session log (see section 5, Storage v2); **tested on the device by David (2026-11-06): everything works, nothing lost**. Note for the record: David has had the CR2032 in the RTC the whole time since the breadboard wiring, so the 00:00 after an early unplug was the checkpoint rule, not a lost clock.

1. ~~Encoder rotation and push switch~~ done (see section 4). Remaining: after soldering, recheck rotation and push switch together (pressing while turning, no accidental triggers).
2. ~~Display bring-up~~ done on the breadboard (see section 4, Display). Remaining: recheck after the real wiring is built (SPI stability at 32 MHz on short wires).
3. ~~Backlight~~ done 2026-11-05: BL pin current is about 1 mA (10.4 mV across a 10 ohm series resistor, BL to 3V3), so GPIO9 drives it directly (`BL_MODE 2`), no Q1/R3/R4 (the schematic still has them, flag when revising it). PWM dimming verified at 20 kHz: 8 is the dimmest (still readable), 255 is bright. Brightness must be a **user setting adjustable on the fly** in the product (clamp to a minimum of 8 so the screen is never dark).
4. Verify BC557 orientation on the breadboard (see above) if Q1 is used.
5. RTC verified 2026-11-05 (`software/bringup/rtc_test.cpp`, env `rtc_test`): DS3231 found on the I2C bus with the CR2032 fitted; time can be set over serial (`T2026-11-05 18:30:00`, or `c` for compile time; the serial monitor in use sends text without a line ending, so the sketch also runs a command 300 ms after the last character); time survives USB unplug, the EN power switch and re-uploads. Drift check (2026-11-05): after about 3 hours with USB unplugged for part of it, the time still matched the PC clock (David compared by eye, so seconds-level accuracy only; a longer check over days is optional). In the product only call `rtc.adjust()` when `lostPower()` is true or the user sets the time; poll the RTC several times per second and redraw only when the second changes (the test sketch prints on its own 1 s timer, so its line can appear up to a second off the real tick; that is not clock error). **I2C bus: `Wire`** (verified), with `Wire.setSDA(2); Wire.setSCL(3);` before `rtc.begin(&Wire)`. `Wire1` is not needed.
6. ~~Buzzer~~ verified 2026-11-05 (`software/bringup/buzzer_test.cpp`, env `buzzer_test`): silent at boot/power cycle, all beep patterns clean (click 15 ms, short 60 ms, long 300 ms, nudge = 3 x 80 ms beeps with 120 ms gaps, train), unaffected by backlight PWM on GPIO9 (shared PWM slice is not a problem with the buzzer driven digitally). **R5 (4.7k pull-up to 3V3) was fitted for these tests. Without it the buzzer beeps at boot (confirmed on the bench); with it, silent. R5 is required.** Nudge is a bit loud on the open bench; judge again inside the enclosure, and if still too loud consider a series resistor or damping foam (not tested).
7. Git: the tag `schematic-with-battery` must point at the **last commit that still contains the battery schematic**, not the commit that removed it. Check with `git log --oneline -- hardware/`, then `git tag -d`, retag the right commit, and fix the remote with `git push origin --delete schematic-with-battery` followed by `git push origin schematic-with-battery`.
8. Firmware: steps 1 to 3 are written and compile (see section 5). Step 1 and parts of 2 have been seen on the device; step 3 has not. Test step 3 on the hardware: change a setting, study a block, unplug and replug (also mid-block, and mid-pause), check totals/favourites/brightness/sound switches survive. Test the rest of step 2 on the hardware when everything is connected (card menus and icons, picker, semester question, favourites, pause/stop, nudge, break, settings, Croatian letters), then do step 3 (flash storage).
9. Enclosure CAD: measure modules with calipers (display, perfboard, Feather, DS3231), USB-C cutout, coin-cell access for RTC, ventilation (optional), M-screw and heat-set insert bosses, knob for EC11 (mounting washers/hex nuts included with the encoder), double-sided foam tape for mounting.
10. Perfboard layout (or later a custom Altium PCB).

## 8. How to work with David

- He prefers **VS Code + PlatformIO**; do not suggest the Arduino IDE.
- He is learning: briefly explain the *why*, keep steps concrete, avoid assuming electronics experience beyond what he has shown (schematics in Altium are comfortable for him).
- **Do not assert facts about hardware from memory when it matters.** Where a datasheet or measurement is needed (pinouts, currents), say so and ask him to check. Several earlier mistakes came from confident guesses.
- Do not make changes to the schematic or pin map without flagging it. The pin map above is the reference; if code and schematic disagree, tell him.
- Small, testable steps. Keep bring-up code separate from the product firmware (e.g. `firmware/bringup/`). Current layout is `software/` (PlatformIO project): `software/bringup/encoder_test.cpp` (raw A/B gesture logger, env `encoder_test`) and `software/bringup/encoder_decoder_test.cpp` (decoder test, env `encoder_decoder`), `software/bringup/button_test.cpp` (push switch, env `button_test`). Each bring-up env uses `build_src_filter` to build only its file; upload with `pio run -e <env> -t upload`.
- Do not commit or push unless asked. Hardware files in `hardware/` are David's (Altium); do not edit them.
