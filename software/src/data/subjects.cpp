#include "data/subjects.h"

namespace Subjects {

namespace {

// ===========================================================================================
// Curriculum data (from subjects.json). Electives are listed as "both" so they show up in
// either semester. Rules for editing:
//  - Give every new subject the next unused id and never change an existing id.
//  - RENAMING is safe: only the id is saved (totals, favourites, session log), never the name. Edit
//    the name here and reflash; all data stays with the subject. Do not delete a subject you want to
//    keep the data of: its totals are dropped at the next save.
//  - Fields: {id, name, year, semesters, elective}
//  - Electives are entered like other subjects (their year and semester) with elective = true.
// ===========================================================================================
constexpr Subject kAll[] = {
  // 1. godina
  {101, "Temelji liječničkog umijeća I", 1, kWinter, false},
  {102, "Fizika i biofizika", 1, kWinter, false},
  {103, "Medicinska biologija", 1, kWinter, false},
  {104, "Socijalna medicina", 1, kWinter, false},
  {105, "Uvod u medicinu i povijest medicine", 1, kWinter, false},
  {106, "Anatomija", 1, kBoth, false},
  {107, "Medicinska kemija i biokemija I", 1, kSummer, false},
  {108, "Prva pomoć", 1, kSummer, false},
  {109, "Medicinski engleski I", 1, kSummer, false},
  // 1. godina - izborni
  {110, "Anatomski aspekti fetalnog i postnatalnog razvoja lubanje", 1, kBoth, true},
  {111, "Anorganske tvari u biološkim procesima", 1, kBoth, true},
  {112, "Bioprognoza i zdravlje", 1, kBoth, true},
  {113, "Čovjek i okoliš", 1, kBoth, true},
  {114, "Društvene mreže u medicini", 1, kBoth, true},
  {115, "Gospodin Horvat ide doktoru", 1, kBoth, true},
  {116, "Kako primijeniti Hipokratovu zakletvu?", 1, kBoth, true},
  {117, "Liječnici i njihovi suradnici", 1, kBoth, true},
  {118, "Metode zdravstveno-odgojnog rada u praksi doktora medicine", 1, kBoth, true},
  {119, "Molekularna biologija u medicini", 1, kBoth, true},
  {120, "Palijativna skrb i rehabilitacija onkološkog bolesnika", 1, kBoth, true},
  {121, "Specifične značajke rada primarnog liječnika", 1, kBoth, true},
  {122, "Zaštita zdravlja mladih", 1, kBoth, true},
  {123, "Živčani signal u odabranim bolestima prijenosa signala", 1, kBoth, true},
  // 2. godina
  {201, "Temelji neuroznanosti", 2, kWinter, false},
  {202, "Medicinska kemija i biokemija II", 2, kWinter, false},
  {203, "Imunologija", 2, kWinter, false},
  {204, "Medicinska sociologija", 2, kWinter, false},
  {205, "Medicinski engleski II", 2, kWinter, false},
  {206, "Histologija i embriologija", 2, kBoth, false},
  {207, "Fiziologija", 2, kBoth, false},
  {208, "Temelji liječničkog umijeća II", 2, kSummer, false},
  // 2. godina - izborni
  {209, "Aditivi u hrani", 2, kBoth, true},
  {210, "Anatomija organa trbuha u ultrazvučnom prikazu", 2, kBoth, true},
  {211, "Elementi naracije o medicini i u medicini", 2, kBoth, true},
  {212, "Etiologija, prevencija i rano otkrivanje tumora", 2, kBoth, true},
  {213, "Funkcionalni aspekti pušenja", 2, kBoth, true},
  {214, "Kako očuvati kvalitetu života i radnu sposobnost liječnika", 2, kBoth, true},
  {215, "Kost", 2, kBoth, true},
  {216, "Mehanizmi boli", 2, kBoth, true},
  {217, "Neurovid i umjetna inteligencija", 2, kBoth, true},
  {218, "Planiranje obitelji", 2, kBoth, true},
  {219, "Povijest psihijatrije", 2, kBoth, true},
  {220, "Pravo na život", 2, kBoth, true},
  {221, "Prije nego što smo se rodili", 2, kBoth, true},
  {222, "Razvitak i plastičnost mozga", 2, kBoth, true},
  {223, "Reologija arterijskog krvotoka", 2, kBoth, true},
  {224, "Stres i mozak", 2, kBoth, true},
  {225, "Učenje i pamćenje", 2, kBoth, true},
  {226, "Umjetnost i medicina", 2, kBoth, true},
  {227, "Uvod u umjetnu inteligenciju u medicini", 2, kBoth, true},
  {228, "Vrijeme je život", 2, kBoth, true},
  {229, "Životne navike i okoliš", 2, kBoth, true},
  // 3. godina
  {301, "Patologija", 3, kBoth, false},
  {302, "Patofiziologija", 3, kBoth, false},
  {303, "Klinička propedeutika", 3, kWinter, false},
  {304, "Medicinski engleski III", 3, kWinter, false},
  {305, "Temelji liječničkog umijeća III", 3, kWinter, false},
  {306, "Farmakologija", 3, kSummer, false},
  {307, "Medicinska mikrobiologija i parazitologija", 3, kSummer, false},
  {308, "Psihološka medicina I i II", 3, kSummer, false},
  // 3. godina - izborni
  {309, "Dispepsija", 3, kBoth, true},
  {310, "Eksperimentalna farmakologija i patologija", 3, kBoth, true},
  {311, "Evolucijski i razvojni korijeni ljudskosti", 3, kBoth, true},
  {312, "Genom eukariota", 3, kBoth, true},
  {313, "Globalno zdravlje", 3, kBoth, true},
  {314, "Idemo na jetru", 3, kBoth, true},
  {315, "Interakcija lijekova i hrane", 3, kBoth, true},
  {316, "CROSS 21 (3.)", 3, kBoth, true},
  {317, "CROSS 22 (3.)", 3, kBoth, true},
  {318, "Moderni pristup liječenju šećerne bolesti tipa 2", 3, kBoth, true},
  {319, "Najčešći štetni učinci lijekova u internoj medicini", 3, kBoth, true},
  {320, "Od biozločina do bioterorizma", 3, kBoth, true},
  {321, "Psihoonkologija", 3, kBoth, true},
  {322, "Reakcija preosjetljivosti na lijekove", 3, kBoth, true},
  {323, "Simptomski orijentirani pristup neurološkom bolesniku", 3, kBoth, true},
  {324, "Spolno prenosive bolesti i infekcije", 3, kBoth, true},
  {325, "Suvremena dijagnostika i epidemiologija dječjih bolesti uzrokovanih virusima", 3, kBoth, true},
  {326, "Što kad srce stane", 3, kBoth, true},
  {327, "Uzročnici zoonoze", 3, kBoth, true},
  {328, "Važno je naći valjan dokaz", 3, kBoth, true},
  {329, "Zdravlje i kvaliteta života", 3, kBoth, true},
  {330, "Živčana stanica", 3, kBoth, true},
  // 4. godina
  {401, "Interna medicina", 4, kBoth, false},
  {402, "Neurologija", 4, kWinter, false},
  {403, "Neurokirurgija", 4, kWinter, false},
  {404, "Nuklearna medicina", 4, kWinter, false},
  {405, "Klinička biokemija", 4, kWinter, false},
  {406, "Medicinski engleski IV", 4, kWinter, false},
  {407, "Infektologija", 4, kSummer, false},
  {408, "Klinička mikrobiologija i parazitologija", 4, kSummer, false},
  {409, "Psihijatrija", 4, kSummer, false},
  {410, "Radiologija", 4, kSummer, false},
  {411, "Dermatovenerologija", 4, kSummer, false},
  {412, "Klinička onkologija", 4, kSummer, false},
  {413, "Medicinska fizika", 4, kSummer, false},
  {414, "Temelji liječničkog umijeća IV", 4, kSummer, false},
  // 4. godina - izborni
  {415, "Autoimune bolesti u svakodnevnoj kliničkoj praksi", 4, kBoth, true},
  {416, "Bolesti štitnjače", 4, kBoth, true},
  {417, "Dijagnostika i liječenje funkcijskih bolesti probavnog sustava", 4, kBoth, true},
  {418, "Dijete u krizi", 4, kBoth, true},
  {419, "Doktore, bole me zglobovi", 4, kBoth, true},
  {420, "Doping i antidoping", 4, kBoth, true},
  {421, "Elektrostimulacija srca u kliničkoj praksi", 4, kBoth, true},
  {422, "Farmakodinamski učinci i laboratorijska karakterizacija antibiotika", 4, kBoth, true},
  {423, "Farmakoekonomika", 4, kBoth, true},
  {424, "Farmakogenomika i individualizacija terapije", 4, kBoth, true},
  {425, "Gastrointestinalne infekcije", 4, kBoth, true},
  {426, "Gdje je granica između embriogeneze i kancerogeneze?", 4, kBoth, true},
  {427, "Geni i okoliš", 4, kBoth, true},
  {428, "Hemofilija i nasljedni poremećaji krvarenja", 4, kBoth, true},
  {429, "CROSS 21 (4.)", 4, kBoth, true},
  {430, "CROSS 22 (4.)", 4, kBoth, true},
  {431, "Komunikacija u medicini (4.)", 4, kBoth, true},
  {432, "Križobolja", 4, kBoth, true},
  {433, "Krv kao lijek", 4, kBoth, true},
  {434, "Mehanizam nastanka nuspojave lijekova", 4, kBoth, true},
  {435, "Mikrobiologija sepse i infekcija krvotoka", 4, kBoth, true},
  {436, "Multirezistentne bakterije", 4, kBoth, true},
  {437, "Nasljedne metaboličke bolesti (4.)", 4, kBoth, true},
  {438, "Nuklearna medicina u dijagnostici i terapiji zloćudnih bolesti", 4, kBoth, true},
  {439, "Ovisnosti", 4, kBoth, true},
  {440, "Primjena matičnih stanica u istraživanju i liječenju bolesti živčanog sustava", 4, kBoth, true},
  {441, "Transplantacija jetre", 4, kBoth, true},
  {442, "Transplantacijska reakcija", 4, kBoth, true},
  {443, "Tuberkuloza i netuberkulozne mikobakterije", 4, kBoth, true},
  {444, "Umjetna inteligencija u slikovnoj dijagnostici", 4, kBoth, true},
  {445, "Venska tromboza i trombofilija", 4, kBoth, true},
  {446, "Zaštita zdravlja obitelji", 4, kBoth, true},
  // 5. godina
  {501, "Pedijatrija", 5, kWinter, false},
  {502, "Kirurgija", 5, kWinter, false},
  {503, "Kirurgija dječje dobi", 5, kWinter, false},
  {504, "Anesteziologija i reanimatologija", 5, kWinter, false},
  {505, "Fizikalna i rehabilitacijska medicina", 5, kWinter, false},
  {506, "Urologija", 5, kWinter, false},
  {507, "Medicinski engleski V", 5, kWinter, false},
  {508, "Ginekologija i opstetricija", 5, kSummer, false},
  {509, "Otorinolaringologija i kirurgija glave i vrata", 5, kSummer, false},
  {510, "Ortopedija", 5, kSummer, false},
  {511, "Maksilofacijalna kirurgija sa stomatologijom", 5, kSummer, false},
  {512, "Medicinska informatika", 5, kSummer, false},
  {513, "Medicinska statistika", 5, kSummer, false},
  {514, "Uvod u znanstveni rad", 5, kSummer, false},
  {515, "Temelji liječničkog umijeća V", 5, kSummer, false},
  // 5. godina - izborni
  {516, "Bolesti srca i trudnoća", 5, kBoth, true},
  {517, "COVID-19", 5, kBoth, true},
  {518, "Dijagnostika i liječenje bolesti perifernog živčanog sustava", 5, kBoth, true},
  {519, "Farmakovigilancija", 5, kBoth, true},
  {520, "Ginekološke manifestacije poremećaja zgrušavanja", 5, kBoth, true},
  {521, "Implantabilni uređaji za rehabilitaciju sluha", 5, kBoth, true},
  {522, "CROSS 22 (5.)", 5, kBoth, true},
  {523, "Kako izraditi diplomski rad", 5, kBoth, true},
  {524, "Kako živjeti s epilepsijom?", 5, kBoth, true},
  {525, "Klinička optometrija", 5, kBoth, true},
  {526, "Komunikacija u medicini (5.)", 5, kBoth, true},
  {527, "Konzervativno liječenje prijeloma", 5, kBoth, true},
  {528, "Kronična bubrežna bolest i nadomještanje bubrežne funkcije dijalizom", 5, kBoth, true},
  {529, "Laboratorijska medicina, laboratorijska imunologija i transfuzijska medicina", 5, kBoth, true},
  {530, "Laparoskopska kirurgija", 5, kBoth, true},
  {531, "Minimalno invazivna kirurgija kralježnice", 5, kBoth, true},
  {532, "Nasljedne metaboličke bolesti (5.)", 5, kBoth, true},
  {533, "Onkološka i rekonstrukcijska kirurgija glave i vrata", 5, kBoth, true},
  {534, "Osnove ultrazvučne dijagnostike", 5, kBoth, true},
  {535, "Pismeno izražavanje u medicini", 5, kBoth, true},
  {536, "Psihijatrijski poremećaji u žena", 5, kBoth, true},
  {537, "Ratna kirurgija", 5, kBoth, true},
  {538, "Regionalna anestezija u liječenju boli", 5, kBoth, true},
  {539, "Rezistentna hipertenzija, dijabetes i kronična bubrežna bolest", 5, kBoth, true},
  {540, "Sportska kardiologija", 5, kBoth, true},
  {541, "Sportska medicina", 5, kBoth, true},
  {542, "Što trebamo znati o transfuziji", 5, kBoth, true},
  {543, "Transplantacija bubrega", 5, kBoth, true},
  {544, "Transplantacija tkiva i rekonstrukcijski postupci u plastičnoj kirurgiji", 5, kBoth, true},
  {545, "Trudnoća i oko", 5, kBoth, true},
  {546, "Ultrazvuk kao stetoskop", 5, kBoth, true},
  {547, "Zaštita od zračenja u zdravstvu", 5, kBoth, true},
  {548, "Živjeti dobro s demencijom", 5, kBoth, true},
  // 6. godina
  {601, "Obiteljska medicina", 6, kWinter, false},
  {602, "Zdravstvena ekologija", 6, kWinter, false},
  {603, "Oftalmologija", 6, kWinter, false},
  {604, "Epidemiologija", 6, kWinter, false},
  {605, "Medicinska genetika", 6, kWinter, false},
  {606, "Sudska medicina", 6, kWinter, false},
  {607, "Medicinska etika", 6, kWinter, false},
  {608, "Palijativna medicina", 6, kWinter, false},
  {609, "Temelji liječničkog umijeća VI", 6, kWinter, false},
  {610, "Medicina rada i sporta", 6, kWinter, false},
  {611, "Organizacija zdravstvene zaštite i zdravstvena ekonomika", 6, kSummer, false},
  {612, "Školska medicina", 6, kSummer, false},
  {613, "Kliničko prosuđivanje", 6, kSummer, false},
  {614, "Hitna medicina", 6, kSummer, false},
  {615, "Racionalna primjena lijekova", 6, kSummer, false},
  {616, "Medicinski engleski VI", 6, kSummer, false},
  {617, "Zdravlje u zajednici", 6, kSummer, false},
  {618, "Dežurstvo u hitnom prijemu", 6, kSummer, false},
  {619, "Diplomski rad", 6, kSummer, false},
  {620, "Završni ispit", 6, kSummer, false},
  {621, "Diplomski ispit", 6, kSummer, false},
};

constexpr size_t kAllCount = sizeof(kAll) / sizeof(kAll[0]);
static_assert(kAllCount <= kMaxSubjects, "Subjects::kMaxSubjects is too small for the subject list");

// Ids are what is saved in flash and in the session log, so a duplicate would merge two subjects' data,
// and 0 means "none" in the saved data. The build fails instead of silently mixing data up.
constexpr bool idsAreValid() {
  for (size_t i = 0; i < kAllCount; i++) {
    if (kAll[i].id == 0) return false;
    for (size_t j = i + 1; j < kAllCount; j++) {
      if (kAll[i].id == kAll[j].id) return false;
    }
  }
  return true;
}
constexpr size_t textLength(const char* text) {
  size_t n = 0;
  while (text[n]) n++;
  return n;
}
constexpr bool namesFit() {
  for (size_t i = 0; i < kAllCount; i++) {
    if (textLength(kAll[i].name) > kMaxNameBytes) return false;
  }
  return true;
}
static_assert(namesFit(), "subjects.cpp: a subject name is longer than Subjects::kMaxNameBytes");
static_assert(idsAreValid(), "subjects.cpp: every subject needs a unique id that is not 0");

}  // namespace

size_t count() { return kAllCount; }

const Subject& at(size_t index) { return kAll[index]; }

int indexOfId(uint16_t id) {
  for (size_t i = 0; i < kAllCount; i++) {
    if (kAll[i].id == id) return (int)i;
  }
  return -1;
}

}  // namespace Subjects
