// ALL text shown on the display, in Croatian (UTF-8, with č ć š đ ž). Names in the code are English;
// only the text itself is Croatian. Change wording here and nowhere else.
// Plain C++ (no Arduino), so the PC tests can use it.
#pragma once

namespace Strings {

// ---- Home ----
constexpr const char* kNoSubject         = "Nema predmeta";
constexpr const char* kRtcMissing        = "Sat nije pronađen";
constexpr const char* kClockWasReset     = "Sat je resetiran";
constexpr const char* kMenuHint          = "Klik: izbornik";

// ---- Pop-up cards (the icon is drawn next to the label, see ui/icons.h) ----
constexpr const char* kCardStart         = "Počni";
constexpr const char* kCardSubject       = "Predmet";
constexpr const char* kCardSettings      = "Postavke";
constexpr const char* kCardStats         = "Statistika";
constexpr const char* kCardPause         = "Pauza";
constexpr const char* kCardResume        = "Nastavi";
constexpr const char* kCardStop          = "Završi";
constexpr const char* kCardNewBlock      = "Novi blok";
constexpr const char* kCardHome          = "Početna";
constexpr const char* kCardYes           = "Da";
constexpr const char* kCardNo            = "Ne";

// ---- Subject picker ----
constexpr const char* kElectivesFolder   = "Izborni predmeti";
constexpr const char* kOtherPeriod       = "Druga godina / semestar";
constexpr const char* kBack              = "Natrag";
constexpr const char* kYearHint          = "Klik: odaberi";
constexpr const char* kYearTitle         = "Odaberi godinu";
constexpr const char* kHalfPrompt        = "Odaberi semestar";
constexpr const char* kHalfWinter        = "Zimski";
constexpr const char* kHalfSummer        = "Ljetni";
constexpr const char* kPickerHint        = "Klik: odaberi   Drži: omiljeno";
constexpr const char* kSwitchQuestion    = "Prebaciti na %u. semestar?";

// ---- Studying ----
constexpr const char* kTargetFormat      = "Cilj: %u min";
constexpr const char* kTargetReached     = "Cilj postignut, možeš nastaviti";
constexpr const char* kRestoredNotice  = "Vraćeno nakon prekida napajanja";
constexpr const char* kPaused            = "Pauzirano";

// ---- Break ----
constexpr const char* kBreak             = "Pauza";
constexpr const char* kLastBlockFormat   = "Zadnji blok: %s";
constexpr const char* kBlockNotCounted   = "Prekratko, nije zapisano";

// ---- Settings ----
constexpr const char* kSettingsTitle     = "Postavke";
constexpr const char* kSettingBrightness = "Svjetlina";
constexpr const char* kSettingSemester   = "Semestar";
constexpr const char* kSettingTarget     = "Cilj učenja";
constexpr const char* kSettingSound     = "Zvuk";
constexpr const char* kSoundTitle        = "Zvuk";
constexpr const char* kSoundHint         = "Klik: uključi / isključi";
constexpr const char* kSettingReset      = "Poništi brojače";
constexpr const char* kSettingClock     = "Datum i vrijeme";
constexpr const char* kClockLocked       = "nije moguće";   // while a study block or break is running
constexpr const char* kSettingSoundTurn  = "Zvuk kotačića";
constexpr const char* kSettingSoundPress = "Zvuk tipke";
constexpr const char* kSettingSoundRestore = "Upozorenje o prekidu";
constexpr const char* kSettingSoundNudge = "Zvuk podsjetnika";
constexpr const char* kOn                = "Uključeno";
constexpr const char* kOff               = "Isključeno";
constexpr const char* kBrightnessFormat  = "%u %%";
constexpr const char* kSemesterFormat    = "%u. semestar";
constexpr const char* kSettingsHint      = "Klik: izmijeni";
constexpr const char* kSettingsEditHint  = "Okreni: promijeni   Klik: gotovo";

// ---- Reset counters ----
constexpr const char* kResetTitle        = "Poništi brojače";
constexpr const char* kResetOne          = "Jedan predmet";
constexpr const char* kResetAll          = "Svi predmeti";
constexpr const char* kResetListTitle    = "Odaberi predmet";
constexpr const char* kResetNone         = "Nema zapisanog vremena";
constexpr const char* kResetQuestionOne  = "Poništiti brojače za:";
constexpr const char* kResetQuestionAll  = "Poništiti SVE predmete?";
constexpr const char* kResetDetails      = "Vrijeme: %s   Sesije: %u";
constexpr const char* kCardReset         = "Poništi";

// ---- Statistics ----
constexpr const char* kStatsTitle        = "Statistika";
constexpr const char* kStatsToday        = "Danas";
constexpr const char* kStatsThisWeek     = "Ovaj tjedan";
constexpr const char* kStatsLastWeek     = "Prošli tjedan";
constexpr const char* kStatsNothing      = "Nema zapisanog učenja";
constexpr const char* kStatsTotalFormat  = "%s, %s";            // duration, number of blocks
constexpr const char* kStatsMostStudied  = "Najviše: %s";
constexpr const char* kStatsHint         = "Okreni: dani i tjedni   Klik: natrag";
constexpr const char* kBlockOne          = "blok";              // 1, 21, 31 ...
constexpr const char* kBlockFew          = "bloka";             // 2 to 4, 22 to 24 ...
constexpr const char* kBlockMany         = "blokova";           // 0, 5 to 20, 25 ...
constexpr const char* kWeekdayInitials[7] = {   // Monday first (the order of the week chart)
  "P", "U", "S", "Č", "P", "S", "N",
};

// ---- Set date and time ----
constexpr const char* kClockSetTitle     = "Postavi sat";
constexpr const char* kFieldDay          = "Dan";
constexpr const char* kFieldMonth        = "Mjesec";
constexpr const char* kFieldYear         = "Godina";
constexpr const char* kFieldHour         = "Sat";
constexpr const char* kFieldMinute       = "Minute";
constexpr const char* kSave              = "Spremi";
constexpr const char* kCancel            = "Odustani";

// ---- Dates and periods ----
constexpr const char* kWeekdays[7] = {   // index 0 = Sunday, as in RTClib's dayOfTheWeek()
  "nedjelja", "ponedjeljak", "utorak", "srijeda", "četvrtak", "petak", "subota",
};
constexpr const char* kYearFormat        = "%u. godina";
constexpr const char* kWinterSemester    = "zimski";
constexpr const char* kSummerSemester    = "ljetni";
constexpr const char* kPeriodFormat      = "%u. godina, %s";   // year, semester name

// ---- Units ----
constexpr const char* kMinutes           = "min";
constexpr const char* kHours             = "h";

}  // namespace Strings
