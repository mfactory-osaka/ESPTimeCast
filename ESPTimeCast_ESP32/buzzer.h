#ifndef BUZZER_H
#define BUZZER_H

#include <string.h>
#include <ctype.h>
#include <math.h>

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
  bool eventRepeat[BUZZER_EVENT_COUNT];
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
const BuzzerStep PATTERN_ALARM[]  = { { 2048, 100 }, { 0, 25 }, { 2048, 100 }, { 0, 25 }, { 2048, 100 }, { 0, 25 }, { 2048, 100 }, { 0, 500 } };

const BuzzerPattern SOUND_PATTERNS[] = {
  { nullptr, 0, false },
  { PATTERN_BEEP, sizeof(PATTERN_BEEP) / sizeof(BuzzerStep), false },
  { PATTERN_CHIRP, sizeof(PATTERN_CHIRP) / sizeof(BuzzerStep), false },
  { PATTERN_ALARM, sizeof(PATTERN_ALARM) / sizeof(BuzzerStep), true }
};
const uint8_t SOUND_PATTERN_COUNT = sizeof(SOUND_PATTERNS) / sizeof(BuzzerPattern);

// -----------------------------------------------------------------------------
// RTTTL support. Custom tunes live in /tunes.txt on LittleFS (one per line,
// uploaded via /upload_tunes) — NOT in config.json — so config.json stays a
// fixed, small size no matter how many tunes exist. Only one line is ever
// read into RAM at a time, on demand, right before it plays.
// -----------------------------------------------------------------------------
#define BUZZER_RTTTL_MAX_STEPS 50    // cap parsed notes per tune
#define BUZZER_RTTTL_MAX_LEN   160   // cap raw line length
#define TUNES_MAX_COUNT        20    // cap number of uploaded tunes

int8_t rtttlNoteOffset(char c) {
  switch (tolower(c)) {
    case 'c': return -9;
    case 'd': return -7;
    case 'e': return -5;
    case 'f': return -4;
    case 'g': return -2;
    case 'a': return 0;
    case 'b': return 2;
    default:  return -100;
  }
}

uint16_t rtttlFreq(char note, bool sharp, uint8_t octave) {
  int8_t off = rtttlNoteOffset(note);
  if (off <= -100) return 0;
  if (sharp) off += 1;
  float semitones = off + (int(octave) - 4) * 12;
  return (uint16_t)round(440.0f * pow(2.0f, semitones / 12.0f));
}

uint8_t buzzerParseRtttl(const char *rtttl, BuzzerStep *out, uint8_t maxSteps) {
  if (!rtttl || !rtttl[0]) return 0;
  const char *first = strchr(rtttl, ':');
  if (!first) return 0;
  const char *second = strchr(first + 1, ':');

  const char *p, *notesStart;
  if (second) {
    p = first + 1;      // name:defaults:notes
    notesStart = second;
  } else {
    p = rtttl;           // defaults:notes (no name)
    notesStart = first;
  }

  uint8_t defDuration = 4, defOctave = 6;
  uint16_t bpm = 63;
  {
    char defaults[40];
    size_t len = min((size_t)(notesStart - p), sizeof(defaults) - 1);
    strncpy(defaults, p, len);
    defaults[len] = '\0';
    char *tok = strtok(defaults, ",");
    while (tok) {
      if      (tok[0] == 'd') defDuration = atoi(tok + 2);
      else if (tok[0] == 'o') defOctave   = atoi(tok + 2);
      else if (tok[0] == 'b') bpm         = atoi(tok + 2);
      tok = strtok(nullptr, ",");
    }
  }
  const float wholeNoteMs = 240000.0f / bpm;

  char notes[BUZZER_RTTTL_MAX_LEN];
  strncpy(notes, notesStart + 1, sizeof(notes) - 1);
  notes[sizeof(notes) - 1] = '\0';

  uint8_t count = 0;
  char *tok = strtok(notes, ",");
  while (tok && count < maxSteps) {
    while (*tok == ' ') tok++;
    uint8_t duration = 0;
    while (isdigit(*tok)) duration = duration * 10 + (*tok++ - '0');
    if (duration == 0) duration = defDuration;

    char note = *tok++;
    bool sharp = (*tok == '#');
    if (sharp) tok++;

    uint8_t octave = 0;
    while (isdigit(*tok)) octave = octave * 10 + (*tok++ - '0');
    if (octave == 0) octave = defOctave;

    bool dotted = (*tok == '.');
    float ms = wholeNoteMs / duration;
    if (dotted) ms *= 1.5f;

    out[count].freq       = rtttlFreq(note, sharp, octave);
    out[count].durationMs = (uint16_t)ms;
    count++;
    tok = strtok(nullptr, ",");
  }
  return count;
}

// Built-in demo tunes (public domain), ids 4-6
const char* const BUZZER_BUILTIN_RTTTL[] = {
  "FurElise:d=8,o=5,b=120:e5,d#5,e5,d#5,e5,b4,d5,c5,a4,4p,c4,e4,a4,b4,4p,e4,g#4,b4,c5,4p,e4,e5,d#5,e5,d#5,e5,b4,d5,c5,a4,4p,c4,e4,a4,b4,4p,e4,c5,b4,a4,2",
  "Nokia:d=4,o=5,b=180:8e6,8d6,f#,g#,8c#6,8b,d,e,8b,8a,c#,e,2a,2p",
  "Saints:d=4,o=6,b=160:p,c,e,f,4g,p,c,e,f,4g,p,c,e,f,2g,2e,2c,2e,4d,p,e,e,d,c,4c,2",
  "OdeToJoy:d=4,o=5,b=140:e,e,f,g,g,f,e,d,c,c,d,e,e,d,d,2p,e,e,f,g,g,f,e,d,c,c,d,e,d,c,c,2p",
  "Tetris:d=4,o=5,b=160:e6,8b,8c6,8d6,16e6,16d6,8c6,8b,a,8a,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,2a,8p"
};
const uint8_t BUZZER_BUILTIN_RTTTL_COUNT = sizeof(BUZZER_BUILTIN_RTTTL) / sizeof(char*);


// One shared scratch buffer — only one tune plays at a time.
BuzzerStep    buzzerCustomScratch[BUZZER_RTTTL_MAX_STEPS];
BuzzerPattern buzzerCustomPattern = { buzzerCustomScratch, 0, false };

// Defined in the .ino (needs LittleFS)
extern bool readTuneLine(uint8_t index, char *out, size_t maxLen);
extern uint8_t tuneCount;

inline const BuzzerPattern* getSoundPattern(uint8_t id) {
  if (id >= 4 && id < 4 + BUZZER_BUILTIN_RTTTL_COUNT) {
    uint8_t n = buzzerParseRtttl(BUZZER_BUILTIN_RTTTL[id - 4], buzzerCustomScratch, BUZZER_RTTTL_MAX_STEPS);
    buzzerCustomPattern.stepCount = n;
    return &buzzerCustomPattern;
  }
  if (id >= 4 + BUZZER_BUILTIN_RTTTL_COUNT) {
    uint8_t tuneIndex = id - (4 + BUZZER_BUILTIN_RTTTL_COUNT);
    char line[BUZZER_RTTTL_MAX_LEN];
    if (readTuneLine(tuneIndex, line, sizeof(line))) {
      uint8_t n = buzzerParseRtttl(line, buzzerCustomScratch, BUZZER_RTTTL_MAX_STEPS);
      buzzerCustomPattern.stepCount = n;
      return &buzzerCustomPattern;
    }
    return &SOUND_PATTERNS[1];
  }
  if (id >= SOUND_PATTERN_COUNT) id = 1;
  return &SOUND_PATTERNS[id];
}

#endif