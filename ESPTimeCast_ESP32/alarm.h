#ifndef ALARM_H
#define ALARM_H

struct AlarmConfig {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
  bool days[7];           // 0=Sun ... 6=Sat
  uint8_t snoozeMinutes;
  uint8_t brightness;
};

inline String formatAlarmSchedule(const AlarmConfig &cfg) {
  if (!cfg.enabled) return "NO ALARM SET";

  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", cfg.hour, cfg.minute);

  bool allDays = true, noDays = true;
  for (int i = 0; i < 7; i++) {
    if (!cfg.days[i]) allDays = false;
    if (cfg.days[i]) noDays = false;
  }

  const char* dayNames[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
  String result = "ALARM ";
  result += timeBuf;

  if (noDays) {
    result += " (NO DAYS SELECTED)";
  } else if (allDays) {
    result += " EVERY DAY";
  } else {
    result += " ";
    bool first = true;
    for (int i = 0; i < 7; i++) {
      if (cfg.days[i]) {
        if (!first) result += " ";
        result += dayNames[i];
        first = false;
      }
    }
  }
  return result;
}

#endif