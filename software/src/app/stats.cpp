#include "app/stats.h"
#include "data/session_log.h"

namespace Stats {

namespace {

bool inRange(const SessionLog::Record& r, DayRange range) {
  uint32_t day = SessionLog::studyDay(r.startUnix);
  return day >= range.firstDay && day <= range.lastDay;
}

struct TotalContext {
  DayRange range;
  Totals totals;
};

struct PerDayContext {
  uint32_t firstDay;
  size_t days;
  Totals* out;
};

struct PerSubjectContext {
  DayRange range;
  SubjectTotal* out;
  size_t capacity;
  size_t count;
};

}  // namespace

uint32_t weekStart(uint32_t day) {
  uint32_t sinceMonday = (SessionLog::weekdayOfStudyDay(day) + 6) % 7;   // Sunday (0) -> 6, Monday (1) -> 0
  return day >= sinceMonday ? day - sinceMonday : 0;
}

Totals total(DayRange range) {
  TotalContext ctx = {range, {0, 0}};
  SessionLog::forEach([](const SessionLog::Record& r, void* c) {
    auto* ctx = static_cast<TotalContext*>(c);
    if (!inRange(r, ctx->range)) return;
    ctx->totals.seconds += r.seconds;
    ctx->totals.sessions++;
  }, &ctx);
  return ctx.totals;
}

void perDay(uint32_t firstDay, size_t days, Totals* out) {
  for (size_t i = 0; i < days; i++) out[i] = {0, 0};
  PerDayContext ctx = {firstDay, days, out};
  SessionLog::forEach([](const SessionLog::Record& r, void* c) {
    auto* ctx = static_cast<PerDayContext*>(c);
    uint32_t day = SessionLog::studyDay(r.startUnix);
    if (day < ctx->firstDay || day - ctx->firstDay >= ctx->days) return;
    Totals& t = ctx->out[day - ctx->firstDay];
    t.seconds += r.seconds;
    t.sessions++;
  }, &ctx);
}

size_t perSubject(DayRange range, SubjectTotal* out, size_t capacity) {
  PerSubjectContext ctx = {range, out, capacity, 0};
  SessionLog::forEach([](const SessionLog::Record& r, void* c) {
    auto* ctx = static_cast<PerSubjectContext*>(c);
    if (!inRange(r, ctx->range)) return;
    for (size_t i = 0; i < ctx->count; i++) {
      if (ctx->out[i].subjectId == r.subjectId) {
        ctx->out[i].seconds += r.seconds;
        ctx->out[i].sessions++;
        return;
      }
    }
    if (ctx->count < ctx->capacity) ctx->out[ctx->count++] = {r.subjectId, r.seconds, 1};
  }, &ctx);

  // Insertion sort: a handful of subjects. Most time first, ties by id so the order is stable.
  for (size_t i = 1; i < ctx.count; i++) {
    SubjectTotal item = out[i];
    size_t j = i;
    while (j > 0 && (out[j - 1].seconds < item.seconds ||
                     (out[j - 1].seconds == item.seconds && out[j - 1].subjectId > item.subjectId))) {
      out[j] = out[j - 1];
      j--;
    }
    out[j] = item;
  }
  return ctx.count;
}

}  // namespace Stats
