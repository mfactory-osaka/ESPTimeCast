#ifndef BUZZER_H
#define BUZZER_H

// -----------------------------------------------------------------------------
// Buzzer: types, event table, sound patterns
// -----------------------------------------------------------------------------
enum BuzzerEventIndex {
  BUZZER_EVT_ALARM = 0,
  BUZZER_EVT_COUNTDOWN,
  BUZZER_EVT_TIMER,
  BUZZER_EVT_POMODORO_WORK,
  BUZZER_EVT_POMODORO_BREAK,
  BUZZER_EVT_STOPWATCH,
  BUZZER_EVT_BUTTON,
  BUZZER_EVENT_COUNT
};

const char* const buzzerEventNames[BUZZER_EVENT_COUNT] = {
  "alarm", "countdown", "timer", "pomodoro_work", "pomodoro_break", "stopwatch", "button"
};

inline int buzzerEventIndexFromName(const String &name) {
  for (int i = 0; i < BUZZER_EVENT_COUNT; i++) {
    if (name == buzzerEventNames[i]) return i;
  }
  return -1;
}

struct BuzzerConfig {
  bool enabled;
  uint8_t pin;            // 255 = not set
  uint8_t volume;         // 1-10
  bool eventEnabled[BUZZER_EVENT_COUNT];
  uint8_t eventSound[BUZZER_EVENT_COUNT];
  bool eventRepeat[BUZZER_EVENT_COUNT];   // <-- add this

};

struct BuzzerStep {
  uint16_t freq;   // 0 = silence
  uint16_t durationMs;
};

struct BuzzerPattern {
  const BuzzerStep* steps;
  uint8_t stepCount;
  bool repeat;
};

const BuzzerStep PATTERN_BEEP[]  = { { 2048, 200 }, { 0, 200 } };
const BuzzerStep PATTERN_CHIRP[] =  { { 1800, 80 }, { 2200, 80 }, { 2600, 80 }, { 3000, 120 }, { 0, 80 } };
const BuzzerStep PATTERN_ALARM[]  = { { 2048, 100 }, { 0, 25 }, { 2048, 100 }, { 0, 25 }, { 2048, 100 }, { 0, 25 }, { 2048, 100 }, { 0, 500 } };   // Beep 4 + LONG pause before repeating};

const BuzzerPattern SOUND_PATTERNS[] = {
  { nullptr, 0, false },                                                 // 0 = off / none
  { PATTERN_BEEP, sizeof(PATTERN_BEEP) / sizeof(BuzzerStep), false },    // 1 = beep
  { PATTERN_CHIRP, sizeof(PATTERN_CHIRP) / sizeof(BuzzerStep), false },  // 2 = chirp (notification)
  { PATTERN_ALARM, sizeof(PATTERN_ALARM) / sizeof(BuzzerStep), true }    // 3 = alarm (repeats)
};
const uint8_t SOUND_PATTERN_COUNT = sizeof(SOUND_PATTERNS) / sizeof(BuzzerPattern);

inline const BuzzerPattern* getSoundPattern(uint8_t id) {
  if (id >= SOUND_PATTERN_COUNT) id = 1;
  return &SOUND_PATTERNS[id];
}

#endif