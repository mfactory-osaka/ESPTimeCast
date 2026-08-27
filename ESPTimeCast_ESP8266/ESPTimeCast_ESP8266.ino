/*
ESPTimeCast™

Copyright (c) 2026 M-Factory

This software is source-available for personal, non-commercial use only.
It is not open source.

See LICENSE.txt for full terms.
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <sntp.h>
#include <time.h>
#include <WiFiClientSecure.h>
#include <ESP8266mDNS.h>
#include "version.h"
#include "mfactoryfont.h"
#include "buzzer.h"
#include "tz_lookup.h"      // Timezone lookup, do not duplicate mapping here!
#include "days_lookup.h"    // Languages for the Days of the Week
#include "months_lookup.h"  // Languages for the Months of the Year
#include "index_html.h"     // Web UI
#include <EEPROM.h>
#include "alarm.h"

#define EEPROM_SIZE 16
#define EEPROM_ADDR 0
#define EEPROM_MAGIC 0xAB
#define PIN_EEPROM_SECTOR 1019  // fixed, safe for 4MB flash (0x3FB000)
EEPROMClass PinStorage(PIN_EEPROM_SECTOR);

// Fallback defaults (Wemos D1 Mini) — only used if EEPROM is blank
// (manual flash users who skipped the installer)
#define L_CLK 14
#define L_CS 13
#define L_DATA 15

struct PinConfig {
  uint8_t magic;  // byte 0
  uint8_t clk;    // byte 1
  uint8_t cs;     // byte 2
  uint8_t data;   // byte 3
};

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4


#ifdef ESP8266
WiFiEventHandler mConnectHandler;
WiFiEventHandler mDisConnectHandler;
WiFiEventHandler mGotIpHandler;
#endif

int CLK_PIN = L_CLK;
int CS_PIN = L_CS;
int DATA_PIN = L_DATA;
MD_Parola P = MD_Parola(HARDWARE_TYPE, L_DATA, L_CLK, L_CS, MAX_DEVICES);
AsyncWebServer server(80);

// --- Buzzer variables ---
BuzzerConfig buzzerConfig;
enum BuzzerState { BUZZER_IDLE,
                   BUZZER_PLAYING };
BuzzerState buzzerState = BUZZER_IDLE;
const BuzzerPattern *buzzerActivePattern = nullptr;
uint8_t buzzerCurrentStep = 0;
unsigned long buzzerStepStartTime = 0;
unsigned long buzzerEventStopAt = 0;
bool buzzerRepeating = false;
const unsigned long BUZZER_EVENT_TIMEOUT_MS = 5000;  // 5 seconds — everything else
const uint8_t defaultEventSound[BUZZER_EVENT_COUNT] = { 3, 3, 3, 3, 2, 1, 1 };
const bool defaultEventRepeat[BUZZER_EVENT_COUNT] = { true, true, true, false, false, false, false };
uint8_t buzzerSavedVolumeForPreview = 0;
bool buzzerVolumeOverridden = false;

// --- Alarm variables ---
#define MAX_ALARMS 4
AlarmConfig alarmConfigs[MAX_ALARMS];
bool alarmRinging = false;
int alarmRingingIndex = -1;
int alarmLastCheckedMinute = -1;
int alarmPreviousDisplayMode = 0;
bool alarmSavedDisplayOff = false;
int alarmSavedBrightness = 7;
bool alarmSavedRotationEnabled = true;
unsigned long alarmSnoozedUntil[MAX_ALARMS] = { 0, 0, 0, 0 };
unsigned long alarmStopAt = 0;
const unsigned long ALARM_TIMEOUT_MS = 15UL * 60 * 1000;
bool alarmTest = false;

// --- Global Scroll Speed Settings ---
const int GENERAL_SCROLL_SPEED = 85;  // Default: Adjust this for Weather Description and Countdown Label (e.g., 50 for faster, 200 for slower)
const int RSS_SCROLL_SPEED = 65;      // Faster than general scroll for RSS headlines
int IP_SCROLL_SPEED = 115;            // Default: Adjust this for the IP Address display (slower for readability)
int messageScrollSpeed = 85;          // default fallback

// Order for safe advance display mode
const uint8_t modeOrder[] = {
  0,  // CLOCK
  5,  // DATE
  1,  // WEATHER
  2,  // WEATHER DESCRIPTION
  3,  // COUNTDOWN
  4,  // BRIDGE
  6   // CUSTOM MESSAGE
};

const uint8_t MODE_COUNT = sizeof(modeOrder) / sizeof(modeOrder[0]);
uint8_t modeIndex = 0;

// --- Nightscout setting ---
const unsigned int NIGHTSCOUT_IDLE_THRESHOLD_MIN = 10;  // minutes before data is considered outdated
unsigned long lastNightscoutFetchTime = 0;
unsigned long nightscoutBackoffUntil = 0;                // millis() deadline for 404/429 backoff
const unsigned long NIGHTSCOUT_FETCH_INTERVAL = 150000;  // 2.5 minutes
int currentGlucose = -1;
String currentDirection = "?";
time_t lastGlucoseTime = 0;  // store timestamp from JSON
bool isNetworkBusy = false;
bool nightscoutMmol = false;
int nightscoutFailCount = 0;

// --- SNS (YouTube / Instagram) sniffing & settings ---
enum SnsType { SNS_NTP,
               SNS_NIGHTSCOUT,
               SNS_YOUTUBE,
               SNS_INSTAGRAM,
               SNS_RSS };

SnsType detectSnsType(const String &val) {
  if (val.indexOf("youtube.com") != -1 || val.indexOf("youtu.be") != -1) return SNS_YOUTUBE;
  if (val.indexOf("instagram.com") != -1) return SNS_INSTAGRAM;
  // Strip query string before extension-based checks so ?show_every=N doesn't fool endsWith
  String pathOnly = val;
  int qPos = pathOnly.indexOf('?');
  if (qPos != -1) pathOnly = pathOnly.substring(0, qPos);
  // RSS sniff: common feed path patterns, checked before the Nightscout https:// catch-all
  if (val.indexOf("feed") != -1 || val.indexOf("/rss") != -1 || val.indexOf("/atom") != -1 || pathOnly.endsWith(".rss") || pathOnly.endsWith(".atom") || pathOnly.endsWith(".xml")) return SNS_RSS;
  if (val.startsWith("https://")) return SNS_NIGHTSCOUT;
  return SNS_NTP;
}
// Strips a named query param from a URL. Handles all positions:
//   ?only=v  →  (empty)
//   ?p=v&rest  →  ?rest
//   ?first=x&p=v  →  ?first=x
//   ?first=x&p=v&rest  →  ?first=x&rest
String stripUrlParam(String url, const String &paramName) {
  String needle = paramName + "=";
  int idx = url.indexOf(needle);
  if (idx == -1) return url;
  int valEnd = url.indexOf('&', idx + needle.length());
  char before = (idx > 0) ? url.charAt(idx - 1) : 0;
  if (before == '?' || before == '&') idx--;  // step back to include the delimiter
  if (valEnd == -1) {
    url.remove(idx);  // last (or only) param: chop from here
  } else if (before == '?') {
    url.remove(idx + 1, valEnd - idx);  // ?param=v&rest  →  ?rest
  } else {
    url.remove(idx, valEnd - idx);  // &param=v&rest  →  &rest
  }
  return url;
}

// Returns the value of show_every=N from a URL, or 1 if not present.
int parseBridgeShowEvery(const String &url) {
  int idx = url.indexOf("show_every=");
  if (idx == -1) return 1;
  String valStr = url.substring(idx + 11);
  int end = valStr.indexOf('&');
  if (end != -1) valStr = valStr.substring(0, end);
  valStr.trim();
  int val = valStr.toInt();
  return (val >= 1) ? val : 1;
}

const unsigned long YOUTUBE_FETCH_INTERVAL = 2UL * 60 * 60 * 1000;    // 2 hours
const unsigned long INSTAGRAM_FETCH_INTERVAL = 2UL * 60 * 60 * 1000;  // 2 hours
const unsigned long RSS_FETCH_INTERVAL = 30UL * 60 * 1000;            // 30 minutes
unsigned long lastYoutubeFetchTime = 0;
unsigned long lastInstagramFetchTime = 0;
unsigned long lastRssFetchTime = 0;
const unsigned long INSTAGRAM_THROTTLE_RETRY_MIN_MS = 10000UL;
const unsigned long INSTAGRAM_THROTTLE_RETRY_JITTER_MS = 5000UL;
unsigned long instagramThrottleRetryAt = 0;
long youtubeSubscribers = -1;
long instagramFollowers = -1;
String rssTitle = "";
int BRIDGE_SHOW_EVERY = 1;  // Show Bridge mode every N rotations, overridable via show_every=N in URL
int bridgeRotationCount = 0;

// --- Device identity ---
const char *DEFAULT_HOSTNAME = "esptimecast";
const char *DEFAULT_AP_PASSWORD = "12345678";
const char *DEFAULT_AP_SSID = "ESPTimeCast";
String deviceHostname = DEFAULT_HOSTNAME;

// WiFi and configuration globals
char ssid[32] = "";
char password[64] = "";
char openWeatherApiKey[64] = "";
char openWeatherCity[64] = "";
char openWeatherCountry[64] = "";
char weatherUnits[12] = "metric";
char timeZone[64] = "";
char language[8] = "en";
unsigned long lastWifiConnectTime = 0;
String mainDesc = "";
String detailedDesc = "";
bool credentialsExist() {
  return (strlen(ssid) > 0);
}
bool isRebooting = false;

// Timing and display settings
unsigned long clockDuration = 10000;
unsigned long weatherDuration = 5000;
bool displayOff = false;
int brightness = 7;
int lastBrightnessBeforeOff = 7;  // remembers brightness to restore on display_on
bool flipDisplay = false;
bool twelveHourToggle = false;
bool showDayOfWeek = true;
bool showDate = false;
bool showHumidity = false;
bool colonBlinkEnabled = true;
char ntpServer1[64] = "pool.ntp.org";
char ntpServer2[256] = "time.nist.gov";
char customMessage[121] = "";
char lastPersistentMessage[128] = "";
int messageDisplaySeconds;
int messageScrollTimes;
unsigned long messageStartTime = 0;
int currentScrollCount = 0;
int currentDisplayCycleCount = 0;
bool pendingModeShiftOut = false;

// Dimming
bool dimmingEnabled = false;
bool displayOffByDimming = false;
bool displayOffByBrightness = false;
int dimStartHour = 18;  // 6pm default
int dimStartMinute = 0;
int dimEndHour = 8;  // 8am default
int dimEndMinute = 0;
int dimBrightness = 2;            // Dimming level (0-15)
bool autoDimmingEnabled = false;  // true if using sunrise/sunset
int sunriseHour = 6;
int sunriseMinute = 0;
int sunsetHour = 18;
int sunsetMinute = 0;
bool clockOnlyDuringDimming = false;
bool configDirty = false;
unsigned long lastBrightnessChange = 0;
const unsigned long saveDelay = 1200;  // 1.2 seconds
int startTotal, endTotal;
bool dimActive = false;

//Countdown Globals - NEW
bool countdownEnabled = false;
time_t countdownTargetTimestamp = 0;  // Unix timestamp
char countdownLabel[64] = "";         // Label for the countdown
bool isDramaticCountdown = true;      // Default to the dramatic countdown mode
int countdownSegment = 0;
unsigned long segmentStartTime = 0;

// Runtime Uptime Tracker
unsigned long bootMillis = 0;                      // Stores millis() at boot
unsigned long lastUptimeLog = 0;                   // Timer for hourly logging
const unsigned long uptimeLogInterval = 600000UL;  // 10 minutes in ms
unsigned long totalUptimeSeconds = 0;              // Persistent accumulated uptime in seconds

// Unified OTA Control Variables
bool isUpdating = false;         // When true, all background tasks (Weather, NTP, Scroll) stop
bool pendingRestart = false;     // Flag to trigger a safe reboot in the loop
unsigned long restartTimer = 0;  // Timer to give the WebServer time to send the final "OK"

// State management
bool weatherCycleStarted = false;
WiFiClient client;
const byte DNS_PORT = 53;
DNSServer dnsServer;
bool rotationEnabled = true;
inline uint32_t getLargestFreeBlock() {
#if defined(ESP8266)
  return ESP.getMaxFreeBlockSize();
#elif defined(ESP32)
  return ESP.getMaxAllocHeap();
#else
  return 0xFFFFFFFF;
#endif
}

String currentTemp = "";
String weatherDescription = "";
String weatherIcon = "";
bool showWeatherDescription = false;
bool weatherAvailable = false;
bool weatherFetched = false;
bool weatherFetchInitiated = false;
bool isAPMode = false;
char tempSymbol = '\006';
bool shouldFetchWeatherNow = false;

unsigned long lastSwitch = 0;
unsigned long lastColonBlink = 0;
int displayMode = 0;  // 0: Clock, 1: Weather, 2: Weather Description, 3: Countdown
int prevDisplayMode = -1;
bool clockScrollDone = false;
int currentHumidity = -1;
bool ntpSyncSuccessful = false;

// NTP Synchronization State Machine
enum NtpState {
  NTP_IDLE,
  NTP_SYNCING,
  NTP_SUCCESS,
  NTP_FAILED
};
NtpState ntpState = NTP_IDLE;
unsigned long ntpStartTime = 0;
const int ntpTimeout = 30000;  // 30 seconds
const int maxNtpRetries = 30;
int ntpRetryCount = 0;
unsigned long lastNtpStatusPrintTime = 0;
const unsigned long ntpStatusPrintInterval = 1000;  // Print status every 1 seconds (adjust as needed)

// Non-blocking IP display globals
bool showingIp = false;
int ipDisplayCount = 0;
const int ipDisplayMax = 1;  // As per working copy for how long IP shows
String pendingIpToShow = "";

// Countdown display state - NEW
bool countdownScrolling = false;
unsigned long countdownScrollEndTime = 0;
unsigned long countdownStaticStartTime = 0;  // For last-day static display

// --- Inmediate countdown finish ---
bool countdownFinished = false;                       // Tracks if the countdown has permanently finished
bool countdownShowFinishedMessage = false;            // Flag to indicate "TIMES UP" message is active
unsigned long countdownFinishedMessageStartTime = 0;  // Timer for the 10-second message duration
unsigned long lastFlashToggleTime = 0;                // For controlling the flashing speed
bool currentInvertState = false;                      // Current state of display inversion for flashing
static bool hourglassPlayed = false;

// Weather Description Mode handling
unsigned long descStartTime = 0;  // For static description
bool descScrolling = false;
int totalPixelWidth = 0;
const unsigned long descriptionDuration = 3000;    // 3s for short text
static unsigned long descScrollEndTime = 0;        // for post-scroll delay (re-used for scroll timing)
const unsigned long descriptionScrollPause = 300;  // 300ms pause after scroll

// Custom message globals
bool forceMessageRestart = false;
bool messageBigNumbers = false;
bool allowInterrupt = true;

// Custom font for days and months
bool useCustomFont = true;

// Timer
bool timerActive = false;
int timerSubState = 0;  // 0: Timer Clock, 1: Message
bool timerPaused = false;
bool timerFinished = false;
unsigned long timerRemainingAtPause = 0;
unsigned long timerOriginalDuration = 0;  // For RESTART command
unsigned long timerFinishStartTime = 0;
unsigned long timerEndTime = 0;
bool isStopwatch = false;

// Pomodoro
bool isPomodoroActive = false;
unsigned long pomodoroWorkMs = 0;                  // Work phase duration in ms
unsigned long pomodoroBreakMs = 0;                 // Break phase duration in ms
bool pomodoroInBreak = false;                      // false = work phase, true = break phase
int pomodoroSession = 1;                           // 1–4, resets after long break
unsigned long pomodoroLongBreakMs = 15 * 60000UL;  // default 15 min

int global_scrolltimes = 0;  // Persisted from HTTP request
int global_msgSeconds = 0;

// --- Donation / Encouragement Message ---
bool hideDonationMsg = false;    // true = user opted out (or is an existing customer)
bool donationFirstBoot = false;  // true only on a fresh install (no prior config.json)
time_t nextDonationTime = 0;     // Unix timestamp for next scheduled message

// --- Configurable Buttons (3x, stored in config.json) ---
struct BtnCfg {
  int pin = -1;
  String shortAct;
  String longAct;
};
BtnCfg btnCfg[4];
#define BTN_LONG_MS 800

// --- Forward declarations ---
void advanceDisplayMode(bool forced = false);
void previousDisplayMode(bool forced = false);
void goToMode(const String &target);
bool handlePomodoroCommand(String cmd);
bool handleAlarmCommand(String cmd);

// --- Safe WiFi credential and API getters ---
const char *getSafeSsid() {
  if (isAPMode && strlen(ssid) == 0) {
    return "";
  } else {
    return isAPMode ? "********" : ssid;
  }
}

const char *getSafePassword() {
  if (strlen(password) == 0) {  // No password set yet — return empty string for fresh install
    return "";
  } else {  // Password exists — mask it in the web UI
    return "********";
  }
}

const char *getSafeApiKey() {
  if (strlen(openWeatherApiKey) == 0) {
    return "";
  } else {
    return "********************************";  // Always masked, even in AP mode
  }
}

// Scroll flipped
textEffect_t getEffectiveScrollDirection(textEffect_t desiredDirection, bool isFlipped) {
  if (isFlipped) {
    // If the display is horizontally flipped, reverse the horizontal scroll direction
    if (desiredDirection == PA_SCROLL_LEFT) {
      return PA_SCROLL_RIGHT;
    } else if (desiredDirection == PA_SCROLL_RIGHT) {
      return PA_SCROLL_LEFT;
    }
  }
  return desiredDirection;
}

// -----------------------------------------------------------------------------
// Configuration Load & Save
// -----------------------------------------------------------------------------
void loadConfig() {
  Serial.println(F("[CONFIG] Loading configuration..."));

  // Check if config.json exists, if not, create default
  if (!LittleFS.exists("/config.json")) {
    Serial.println(F("[CONFIG] config.json not found, creating with defaults..."));
    JsonDocument doc;
    doc[F("ssid")] = "";
    doc[F("password")] = "";
    doc[F("openWeatherApiKey")] = "";
    doc[F("openWeatherCity")] = "";
    doc[F("openWeatherCountry")] = "";
    doc[F("weatherUnits")] = "metric";
    doc[F("clockDuration")] = 10000;
    doc[F("weatherDuration")] = 5000;
    doc[F("timeZone")] = "";
    doc[F("language")] = "en";
    doc[F("brightness")] = brightness;
    doc[F("lastBrightnessBeforeOff")] = brightness;
    doc[F("flipDisplay")] = flipDisplay;
    doc[F("twelveHourToggle")] = twelveHourToggle;
    doc[F("showDayOfWeek")] = showDayOfWeek;
    doc[F("showDate")] = false;
    doc[F("showHumidity")] = showHumidity;
    doc[F("colonBlinkEnabled")] = colonBlinkEnabled;
    doc[F("ntpServer1")] = ntpServer1;
    doc[F("ntpServer2")] = ntpServer2;
    doc[F("dimmingEnabled")] = dimmingEnabled;
    doc[F("dimStartHour")] = dimStartHour;
    doc[F("dimStartMinute")] = dimStartMinute;
    doc[F("dimEndHour")] = dimEndHour;
    doc[F("dimEndMinute")] = dimEndMinute;
    doc[F("dimBrightness")] = dimBrightness;
    doc[F("showWeatherDescription")] = showWeatherDescription;

    // --- Automatic dimming defaults ---
    doc[F("autoDimmingEnabled")] = autoDimmingEnabled;
    doc[F("sunriseHour")] = sunriseHour;
    doc[F("sunriseMinute")] = sunriseMinute;
    doc[F("sunsetHour")] = sunsetHour;
    doc[F("sunsetMinute")] = sunsetMinute;
    doc[F("clockOnlyDuringDimming")] = false;

    // Add countdown defaults when creating a new config.json
    JsonObject countdownObj = doc.createNestedObject("countdown");
    countdownObj["enabled"] = false;
    countdownObj["targetTimestamp"] = 0;
    countdownObj["label"] = "";
    countdownObj["isDramaticCountdown"] = true;

    // Fresh install: donation messages enabled, schedule for next day
    doc[F("hideDonationMsg")] = false;
    doc[F("nextDonationTime")] = 0;
    donationFirstBoot = true;
    Serial.println(F("[DONATION] Fresh install — messages enabled, will schedule for day 2."));

    File f = LittleFS.open("/config.json", "w");
    if (f) {
      serializeJson(doc, f);
      f.close();
      Serial.println(F("[CONFIG] Default config.json created."));
    } else {
      Serial.println(F("[ERROR] Failed to create default config.json"));
    }
  }

  Serial.println(F("[CONFIG] Attempting to open config.json for reading."));
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[ERROR] Failed to open config.json for reading. Cannot load config."));
    return;
  }

  JsonDocument doc;  // Size based on ArduinoJson Assistant + buffer
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print(F("[ERROR] JSON parse failed during load: "));
    Serial.println(error.f_str());
    return;
  }

  bool configChanged = false;

  if (doc.containsKey("hostname")) {
    deviceHostname = doc["hostname"].as<String>();
    Serial.print(F("[CONFIG] Loaded hostname: "));
    Serial.println(deviceHostname);
  }

  strlcpy(ssid, doc["ssid"] | "", sizeof(ssid));
  strlcpy(password, doc["password"] | "", sizeof(password));
  strlcpy(openWeatherApiKey, doc["openWeatherApiKey"] | "", sizeof(openWeatherApiKey));
  strlcpy(openWeatherCity, doc["openWeatherCity"] | "", sizeof(openWeatherCity));
  strlcpy(openWeatherCountry, doc["openWeatherCountry"] | "", sizeof(openWeatherCountry));
  strlcpy(weatherUnits, doc["weatherUnits"] | "metric", sizeof(weatherUnits));
  strlcpy(customMessage, doc["customMessage"] | "", sizeof(customMessage));
  strlcpy(lastPersistentMessage, customMessage, sizeof(lastPersistentMessage));
  clockDuration = doc["clockDuration"] | 10000;
  weatherDuration = doc["weatherDuration"] | 5000;
  strlcpy(timeZone, doc["timeZone"] | "Etc/UTC", sizeof(timeZone));
  if (doc.containsKey("language")) {
    strlcpy(language, doc["language"], sizeof(language));
  } else {
    strlcpy(language, "en", sizeof(language));
    Serial.println(F("[CONFIG] 'language' key not found in config.json, defaulting to 'en'."));
  }

  brightness = doc["brightness"] | 7;
  displayOff = doc["displayOff"] | false;
  lastBrightnessBeforeOff = doc["lastBrightnessBeforeOff"] | 7;
  flipDisplay = doc["flipDisplay"] | false;
  twelveHourToggle = doc["twelveHourToggle"] | false;
  showDayOfWeek = doc["showDayOfWeek"] | true;
  showDate = doc["showDate"] | false;
  showHumidity = doc["showHumidity"] | false;
  colonBlinkEnabled = doc.containsKey("colonBlinkEnabled") ? doc["colonBlinkEnabled"].as<bool>() : true;
  showWeatherDescription = doc["showWeatherDescription"] | false;

  // --- Dimming settings ---
  if (doc["dimmingEnabled"].is<bool>()) {
    dimmingEnabled = doc["dimmingEnabled"].as<bool>();
  } else {
    String de = doc["dimmingEnabled"].as<String>();
    dimmingEnabled = (de == "true" || de == "1" || de == "on");
  }

  dimStartHour = doc["dimStartHour"] | 18;
  dimStartMinute = doc["dimStartMinute"] | 0;
  dimEndHour = doc["dimEndHour"] | 8;
  dimEndMinute = doc["dimEndMinute"] | 0;

  // safely handle both numeric or string "Off" for dimBrightness
  if (doc["dimBrightness"].is<int>()) {
    dimBrightness = doc["dimBrightness"].as<int>();
  } else {
    String val = doc["dimBrightness"].as<String>();
    if (val.equalsIgnoreCase("off")) dimBrightness = -1;
    else dimBrightness = val.toInt();
  }

  // --- Automatic dimming ---
  if (doc.containsKey("autoDimmingEnabled")) {
    if (doc["autoDimmingEnabled"].is<bool>()) {
      autoDimmingEnabled = doc["autoDimmingEnabled"].as<bool>();
    } else {
      String val = doc["autoDimmingEnabled"].as<String>();
      autoDimmingEnabled = (val == "true" || val == "1" || val == "on");
    }
  } else {
    autoDimmingEnabled = false;  // default if key missing
  }

  sunriseHour = doc["sunriseHour"] | 6;
  sunriseMinute = doc["sunriseMinute"] | 0;
  sunsetHour = doc["sunsetHour"] | 18;
  sunsetMinute = doc["sunsetMinute"] | 0;

  strlcpy(ntpServer1, doc["ntpServer1"] | "pool.ntp.org", sizeof(ntpServer1));
  strlcpy(ntpServer2, doc["ntpServer2"] | "time.nist.gov", sizeof(ntpServer2));

  if (strcmp(weatherUnits, "imperial") == 0)
    tempSymbol = '\007';
  else
    tempSymbol = '\006';


  // --- COUNTDOWN CONFIG LOADING ---
  if (doc.containsKey("countdown")) {
    JsonObject countdownObj = doc["countdown"];

    countdownEnabled = countdownObj["enabled"] | false;
    countdownTargetTimestamp = countdownObj["targetTimestamp"] | 0;
    isDramaticCountdown = countdownObj["isDramaticCountdown"] | true;


    JsonVariant labelVariant = countdownObj["label"];
    if (labelVariant.isNull() || !labelVariant.is<const char *>()) {
      strcpy(countdownLabel, "");
    } else {
      const char *labelTemp = labelVariant.as<const char *>();
      size_t labelLen = strlen(labelTemp);
      if (labelLen >= sizeof(countdownLabel)) {
        Serial.println(F("[CONFIG] label from JSON too long, truncating."));
      }
      strlcpy(countdownLabel, labelTemp, sizeof(countdownLabel));
    }
    countdownFinished = false;
  } else {
    countdownEnabled = false;
    countdownTargetTimestamp = 0;
    strcpy(countdownLabel, "");
    isDramaticCountdown = true;
    Serial.println(F("[CONFIG] Countdown object not found, defaulting to disabled."));
    countdownFinished = false;
  }

  // --- CLOCK-ONLY-DURING-DIMMING LOADING ---
  if (doc.containsKey("clockOnlyDuringDimming")) {
    clockOnlyDuringDimming = doc["clockOnlyDuringDimming"].as<bool>();
  } else {
    clockOnlyDuringDimming = false;
    doc["clockOnlyDuringDimming"] = clockOnlyDuringDimming;
    configChanged = true;
    Serial.println(F("[CONFIG] Migrated: added clockOnlyDuringDimming default."));
  }

  // --- DONATION MESSAGE LOADING / MIGRATION ---
  if (doc.containsKey("hideDonationMsg")) {
    hideDonationMsg = doc["hideDonationMsg"].as<bool>();
    nextDonationTime = (time_t)(doc["nextDonationTime"] | 0);
  } else {
    // Existing customer upgrading — silence donation messages silently
    hideDonationMsg = true;
    nextDonationTime = 0;
    doc["hideDonationMsg"] = true;
    doc["nextDonationTime"] = 0;
    configChanged = true;
    Serial.println(F("[CONFIG] Migrated: existing user detected, hideDonationMsg set to true."));
  }

  // --- BUZZER CONFIG LOADING / MIGRATION ---
  if (doc.containsKey("buzzer")) {
    JsonObject bz = doc["buzzer"];
    buzzerConfig.enabled = bz["enabled"] | true;
    buzzerConfig.pin = bz["pin"] | 255;
    buzzerConfig.volume = constrain((int)(bz["volume"] | 7), 1, 10);

    JsonArray events = bz["events"];
    for (int i = 0; i < BUZZER_EVENT_COUNT; i++) {
      if (i < (int)events.size()) {
        buzzerConfig.eventEnabled[i] = events[i]["enabled"] | true;
        buzzerConfig.eventSound[i] = events[i]["sound"] | defaultEventSound[i];
        buzzerConfig.eventRepeat[i] = events[i]["repeat"] | defaultEventRepeat[i];
      } else {
        buzzerConfig.eventEnabled[i] = true;
        buzzerConfig.eventSound[i] = defaultEventSound[i];
        buzzerConfig.eventRepeat[i] = defaultEventRepeat[i];
      }
    }
  } else {
    buzzerConfig.enabled = false;  // off until user wires a buzzer and enables it
    buzzerConfig.pin = 255;
    buzzerConfig.volume = 7;
    for (int i = 0; i < BUZZER_EVENT_COUNT; i++) {
      buzzerConfig.eventEnabled[i] = true;
      buzzerConfig.eventSound[i] = defaultEventSound[i];
      buzzerConfig.eventRepeat[i] = defaultEventRepeat[i];
    }

    JsonObject bz = doc.createNestedObject("buzzer");
    bz["enabled"] = buzzerConfig.enabled;
    bz["pin"] = buzzerConfig.pin;
    bz["volume"] = buzzerConfig.volume;
    JsonArray events = bz.createNestedArray("events");
    for (int i = 0; i < BUZZER_EVENT_COUNT; i++) {
      JsonObject e = events.createNestedObject();
      e["name"] = buzzerEventNames[i];
      e["enabled"] = buzzerConfig.eventEnabled[i];
      e["sound"] = buzzerConfig.eventSound[i];
      e["repeat"] = buzzerConfig.eventRepeat[i];
    }
    configChanged = true;
    Serial.println(F("[CONFIG] Migrated: added buzzer defaults."));
  }

  // --- ALARM CONFIG LOADING / MIGRATION ---
  if (doc.containsKey("alarms")) {
    JsonArray alarms = doc["alarms"];
    for (int i = 0; i < MAX_ALARMS; i++) {
      if (i < (int)alarms.size()) {
        JsonObject al = alarms[i];
        alarmConfigs[i].enabled = al["enabled"] | false;
        alarmConfigs[i].hour = al["hour"] | 7;
        alarmConfigs[i].minute = al["minute"] | 0;
        JsonArray days = al["days"];
        for (int d = 0; d < 7; d++) {
          alarmConfigs[i].days[d] = (d < (int)days.size()) ? (bool)days[d] : true;
        }
        alarmConfigs[i].snoozeMinutes = al["snoozeMinutes"] | 15;
        alarmConfigs[i].brightness = al["brightness"] | 10;
        alarmConfigs[i].sound = al["sound"] | 3;
      } else {
        alarmConfigs[i].enabled = false;
        alarmConfigs[i].hour = 7;
        alarmConfigs[i].minute = 0;
        for (int d = 0; d < 7; d++) alarmConfigs[i].days[d] = true;
        alarmConfigs[i].snoozeMinutes = 15;
        alarmConfigs[i].brightness = 10;
        alarmConfigs[i].sound = 3;
      }
    }
  } else if (doc.containsKey("alarm")) {
    // Migrating from single-alarm config — becomes alarm slot 0
    JsonObject al = doc["alarm"];
    alarmConfigs[0].enabled = al["enabled"] | false;
    alarmConfigs[0].hour = al["hour"] | 7;
    alarmConfigs[0].minute = al["minute"] | 0;
    JsonArray days = al["days"];
    for (int d = 0; d < 7; d++) {
      alarmConfigs[0].days[d] = (d < (int)days.size()) ? (bool)days[d] : true;
    }
    alarmConfigs[0].snoozeMinutes = al["snoozeMinutes"] | 15;
    alarmConfigs[0].brightness = al["brightness"] | 10;
    alarmConfigs[0].sound = 3;
    for (int i = 1; i < MAX_ALARMS; i++) {
      alarmConfigs[i].enabled = false;
      alarmConfigs[i].hour = 7;
      alarmConfigs[i].minute = 0;
      for (int d = 0; d < 7; d++) alarmConfigs[i].days[d] = true;
      alarmConfigs[i].snoozeMinutes = 15;
      alarmConfigs[i].brightness = 10;
      alarmConfigs[i].sound = 3;
    }
    doc.remove("alarm");
    configChanged = true;
    Serial.println(F("[CONFIG] Migrated: single alarm -> alarm slot 0."));
  } else {
    for (int i = 0; i < MAX_ALARMS; i++) {
      alarmConfigs[i].enabled = false;
      alarmConfigs[i].hour = 7;
      alarmConfigs[i].minute = 0;
      for (int d = 0; d < 7; d++) alarmConfigs[i].days[d] = true;
      alarmConfigs[i].snoozeMinutes = 15;
      alarmConfigs[i].brightness = 10;
      alarmConfigs[i].sound = 3;
    }
    JsonArray alarms = doc.createNestedArray("alarms");
    for (int i = 0; i < MAX_ALARMS; i++) {
      JsonObject al = alarms.createNestedObject();
      al["enabled"] = alarmConfigs[i].enabled;
      al["hour"] = alarmConfigs[i].hour;
      al["minute"] = alarmConfigs[i].minute;
      JsonArray days = al.createNestedArray("days");
      for (int d = 0; d < 7; d++) days.add(alarmConfigs[i].days[d]);
      al["snoozeMinutes"] = alarmConfigs[i].snoozeMinutes;
      al["brightness"] = alarmConfigs[i].brightness;
      al["sound"] = alarmConfigs[i].sound;
    }
    configChanged = true;
    Serial.println(F("[CONFIG] Migrated: added alarm defaults (4 slots)."));
  }

  // --- Save migrated config if needed ---
  if (configChanged) {
    Serial.println(F("[CONFIG] Saving migrated config.json"));

    File f = LittleFS.open("/config.json", "w");
    if (f) {
      serializeJson(doc, f);
      f.close();
      Serial.println(F("[CONFIG] Migration saved successfully."));
    } else {
      Serial.println(F("[ERROR] Failed to save migrated config.json"));
    }
  }

  // --- BUTTON CONFIG LOADING ---
  if (doc.containsKey("buttons")) {
    JsonArray arr = doc["buttons"].as<JsonArray>();

    for (int i = 0; i < 4 && i < (int)arr.size(); i++) {
      btnCfg[i].pin = arr[i]["pin"] | -1;
      btnCfg[i].shortAct = arr[i]["shortAction"] | "";
      btnCfg[i].longAct = arr[i]["longAction"] | "";

      if (btnCfg[i].pin >= 0) {
        Serial.printf(PSTR("[BUTTON] Button %d → GPIO %d | short=%s | long=%s\n"),
                      i + 1,
                      btnCfg[i].pin,
                      btnCfg[i].shortAct.c_str(),
                      btnCfg[i].longAct.c_str());
      }
    }
  }

  Serial.println(F("[CONFIG] Configuration loaded."));
}


// -----------------------------------------------------------------------------
// Network Identity
// -----------------------------------------------------------------------------
void setupHostname() {
#if defined(ESP8266)
  WiFi.hostname(deviceHostname);
#elif defined(ESP32)
  WiFi.setHostname(deviceHostname.c_str());
#endif
}


// -----------------------------------------------------------------------------
// WiFi Setup
// -----------------------------------------------------------------------------
void connectWiFi() {
  Serial.println(F("[WIFI] Connecting to WiFi..."));

  if (!credentialsExist()) {
    Serial.println(F("[WIFI] No saved credentials. Starting AP mode directly."));
    WiFi.mode(WIFI_AP);
    WiFi.disconnect(true);
    delay(100);

    setupHostname();

    if (strlen(DEFAULT_AP_PASSWORD) < 8) {
      WiFi.softAP(DEFAULT_AP_SSID);
      Serial.println(F("[WIFI] AP Mode started (no password, too short)."));
    } else {
      WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
      Serial.println(F("[WIFI] AP Mode started."));
    }

    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    Serial.printf(PSTR("[HEAP] AP mode started: %u free, %u largest\n"), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
    Serial.print(F("[WIFI] AP IP address: "));
    Serial.println(WiFi.softAPIP());
    isAPMode = true;

    WiFiMode_t mode = WiFi.getMode();
    Serial.printf(PSTR("[WIFI] WiFi mode after setting AP: %s\n"),
                  mode == WIFI_OFF ? "OFF" : mode == WIFI_STA    ? "STA ONLY"
                                           : mode == WIFI_AP     ? "AP ONLY"
                                           : mode == WIFI_AP_STA ? "AP + STA (Error!)"
                                                                 : "UNKNOWN");

    Serial.println(F("[WIFI] AP Mode Started"));
    return;
  }

  // If credentials exist, attempt STA connection
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
#ifdef ESP8266
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
#endif
#ifdef ESP32
  WiFi.setSleep(false);
#endif
  setupHostname();
  WiFi.disconnect();  // Ensure a clean slate
  delay(100);         // The "Radio Breathing Room"
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  const unsigned long timeout = 30000;
  const int maxRetries = 3;
  int retryCount = 0;
  unsigned long animTimer = 0;
  int animFrame = 0;
  bool animating = true;

  while (animating) {
    unsigned long now = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(F("[WIFI] Connected: "));
      Serial.println(WiFi.localIP().toString());
      isAPMode = false;

      WiFiMode_t mode = WiFi.getMode();
      Serial.printf(PSTR("[WIFI] WiFi mode after STA connection: %s\n"),
                    mode == WIFI_OFF ? "OFF" : mode == WIFI_STA    ? "STA ONLY"
                                             : mode == WIFI_AP     ? "AP ONLY"
                                             : mode == WIFI_AP_STA ? "AP + STA (Error!)"
                                                                   : "UNKNOWN");

      // --- IP Display initiation ---
      if (LittleFS.exists("/update_success.txt")) {
        // Use (char) to force the raw font glyphs and avoid the newline bug
        pendingIpToShow = String((char)10) + (char)11 + (char)32 + (char)173 + String(FIRMWARE_VERSION);
        LittleFS.remove("/update_success.txt");
        IP_SCROLL_SPEED = 70;
      } else {
        pendingIpToShow = WiFi.localIP().toString();
        IP_SCROLL_SPEED = 115;
        // Replace all dots with your custom font code 184
        for (int i = 0; i < pendingIpToShow.length(); i++) {
          if (pendingIpToShow[i] == '.') {
            pendingIpToShow[i] = 184;
          }
        }
      }

      showingIp = true;
      ipDisplayCount = 0;
      P.displayClear();
      P.setCharSpacing(1);
      textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);
      P.displayScroll(pendingIpToShow.c_str(), PA_CENTER, actualScrollDirection, IP_SCROLL_SPEED);
      // --- END IP Display initiation ---

      animating = false;  // Exit the connection loop
      break;
    } else if (now - startAttemptTime >= timeout) {

      if (retryCount < maxRetries - 1) {
        retryCount++;
        Serial.printf(PSTR("[WIFI] Attempt failed. Retrying (%d/%d)...\n"), retryCount + 1, maxRetries);

        WiFi.disconnect();
        delay(500);
        WiFi.begin(ssid, password);
        startAttemptTime = millis();  // reset timeout timer
      } else {
        Serial.println(F("[WIFI] All attempts failed. Starting AP mode..."));

        WiFi.mode(WIFI_AP);
        WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
        Serial.print(F("[WIFI] AP IP address: "));
        Serial.println(WiFi.softAPIP());
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
        Serial.printf(PSTR("[HEAP] AP mode started: %u free, %u largest\n"), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
        isAPMode = true;

        WiFiMode_t mode = WiFi.getMode();
        Serial.printf(PSTR("[WIFI] WiFi mode after STA failure and setting AP: %s\n"),
                      mode == WIFI_OFF ? "OFF" : mode == WIFI_STA    ? "STA ONLY"
                                               : mode == WIFI_AP     ? "AP ONLY"
                                               : mode == WIFI_AP_STA ? "AP + STA (Error!)"
                                                                     : "UNKNOWN");

        animating = false;
        Serial.println(F("[WIFI] AP Mode Started"));
        break;
      }
    }
    if (now - animTimer > 750) {
      animTimer = now;
      P.setTextAlignment(PA_CENTER);
      switch (animFrame % 3) {
        case 0:
          {
            String s = "\003 ";
            s += char(169);
            P.print(s.c_str());
            break;
          }

        case 1:
          {
            String s = "\003 ";
            s += char(170);
            P.print(s.c_str());
            break;
          }

        case 2:
          {
            String s = "\003 ";
            s += char(171);
            P.print(s.c_str());
            break;
          }
      }
      animFrame++;
    }
    yield();
  }
}

// -----------------------------------------------------------------------------
// mDNS
// -----------------------------------------------------------------------------
void setupMDNS() {
  MDNS.end();

  bool mdnsStarted = MDNS.begin(deviceHostname.c_str());

  if (mdnsStarted) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf(PSTR("[WIFI] mDNS started: http://%s.local\n"), deviceHostname.c_str());
  } else {
    Serial.println(F("[WIFI] mDNS failed to start"));
  }
}

// -----------------------------------------------------------------------------
// Time / NTP Functions
// -----------------------------------------------------------------------------
void setupTime() {
  sntp_stop();
  if (!isAPMode) {
    Serial.println(F("[TIME] Starting NTP sync"));
  }

  configTime(0, 0, ntpServer1, ntpServer2);

  // Set the Time Zone
  setenv("TZ", ianaToPosix(timeZone), 1);
  tzset();

  // Initialize state flags (essential for your loop logic to handle retries)
  ntpState = NTP_SYNCING;
  ntpStartTime = millis();
  ntpRetryCount = 0;
  ntpSyncSuccessful = false;
}


// -----------------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------------
void printConfigToSerial() {
  Serial.println(F("========= Loaded Configuration ========="));
  Serial.print(F("WiFi SSID: "));
  Serial.println(ssid);
  Serial.print(F("WiFi Password: "));
  Serial.println(password);
  Serial.print(F("OpenWeather City: "));
  Serial.println(openWeatherCity);
  Serial.print(F("OpenWeather Country: "));
  Serial.println(openWeatherCountry);
  Serial.print(F("OpenWeather API Key: "));
  Serial.println(openWeatherApiKey);
  Serial.print(F("Temperature Unit: "));
  Serial.println(weatherUnits);
  Serial.print(F("Clock duration: "));
  Serial.println(clockDuration);
  Serial.print(F("Weather duration: "));
  Serial.println(weatherDuration);
  Serial.print(F("TimeZone (IANA): "));
  Serial.println(timeZone);
  Serial.print(F("Days of the Week/Weather description language: "));
  Serial.println(language);
  Serial.print(F("Brightness: "));
  Serial.println(brightness);
  Serial.print(F("Flip Display: "));
  Serial.println(flipDisplay ? "Yes" : "No");
  Serial.print(F("Show 12h Clock: "));
  Serial.println(twelveHourToggle ? "Yes" : "No");
  Serial.print(F("Show Day of the Week: "));
  Serial.println(showDayOfWeek ? "Yes" : "No");
  Serial.print(F("Show Date: "));
  Serial.println(showDate ? "Yes" : "No");
  Serial.print(F("Show Weather Description: "));
  Serial.println(showWeatherDescription ? "Yes" : "No");
  Serial.print(F("Show Humidity: "));
  Serial.println(showHumidity ? "Yes" : "No");
  Serial.print(F("Blinking colon: "));
  Serial.println(colonBlinkEnabled ? "Yes" : "No");
  Serial.print(F("NTP Server 1: "));
  Serial.println(ntpServer1);
  Serial.print(F("NTP Server 2: "));
  Serial.println(ntpServer2);
  Serial.print(F("Automatic Dimming: "));
  Serial.println(autoDimmingEnabled ? "Enabled" : "Disabled");
  Serial.print(F("Custom Dimming: "));
  Serial.println(dimmingEnabled ? "Enabled" : "Disabled");
  Serial.print(F("Clock only during dimming: "));
  Serial.println(clockOnlyDuringDimming ? "Yes" : "No");
  if (autoDimmingEnabled) {
    // --- Automatic (Sunrise/Sunset) dimming mode ---
    if ((sunriseHour == 6 && sunriseMinute == 0) && (sunsetHour == 18 && sunsetMinute == 0)) {
      Serial.println(F("Automatic Dimming Schedule: Sunrise/Sunset Data not available yet (waiting for weather update)"));
    } else {
      Serial.printf(PSTR("Automatic Dimming Schedule: Sunrise: %02d:%02d → Sunset: %02d:%02d\n"),
                    sunriseHour, sunriseMinute, sunsetHour, sunsetMinute);
      time_t now_time = time(nullptr);
      struct tm localTime;
      localtime_r(&now_time, &localTime);
      int curTotal = localTime.tm_hour * 60 + localTime.tm_min;
      int startTotal = sunsetHour * 60 + sunsetMinute;
      int endTotal = sunriseHour * 60 + sunriseMinute;
      bool autoActive = (startTotal < endTotal)
                          ? (curTotal >= startTotal && curTotal < endTotal)
                          : (curTotal >= startTotal || curTotal < endTotal);
      Serial.printf(PSTR("Current Auto-Dimming Status: %s\n"), autoActive ? "ACTIVE" : "Inactive");
      Serial.printf(PSTR("Dimming Brightness (night): %d\n"), dimBrightness);
    }
  } else {
    // --- Manual (Custom Schedule) dimming mode ---
    Serial.printf(PSTR("Custom Dimming Schedule: %02d:%02d → %02d:%02d\n"),
                  dimStartHour, dimStartMinute, dimEndHour, dimEndMinute);
    Serial.printf(PSTR("Dimming Brightness: %d\n"), dimBrightness);
  }
  Serial.print(F("Countdown Enabled: "));
  Serial.println(countdownEnabled ? "Yes" : "No");
  Serial.print(F("Countdown Target Timestamp: "));
  Serial.println(countdownTargetTimestamp);
  Serial.print(F("Countdown Label: "));
  Serial.println(countdownLabel);
  Serial.print(F("Dramatic Countdown Display: "));
  Serial.println(isDramaticCountdown ? "Yes" : "No");
  Serial.print(F("Custom Message: "));
  Serial.println(customMessage);
  Serial.print(F("Donation Messages: "));
  Serial.println(hideDonationMsg ? "Hidden (user opted out)" : "Enabled");
  Serial.print(F("Next Donation Message: "));
  if (hideDonationMsg || nextDonationTime == 0) {
    Serial.println(F("N/A"));
  } else {
    struct tm t;
    localtime_r(&nextDonationTime, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &t);
    Serial.println(buf);
  }
  Serial.print(F("Total Runtime: "));
  if (getTotalRuntimeSeconds() > 0) {
    Serial.println(formatTotalRuntime());
  } else {
    Serial.println(F("No runtime recorded yet."));
  }
  Serial.print(F("Hostname: "));
  Serial.println(deviceHostname);
  Serial.printf(PSTR("Matrix Pins: CLK=%d CS=%d DATA=%d\n"),
                CLK_PIN, CS_PIN, DATA_PIN);
  Serial.print(F("Physical Buttons:"));
  bool hasButtons = false;
  for (int i = 0; i < 4; i++) {
    if (btnCfg[i].pin >= 0) {
      hasButtons = true;
      Serial.printf(PSTR(" Button %d → GPIO %d | short=%s | long=%s\n"),
                    i + 1,
                    btnCfg[i].pin,
                    btnCfg[i].shortAct.c_str(),
                    btnCfg[i].longAct.c_str());
    }
  }
  if (!hasButtons) {
    Serial.println(F(" None configured"));
  }
  Serial.println(F("Buzzer:"));
  Serial.printf(PSTR("  Enabled=%s | Pin=%d | Volume=%d\n"),
                buzzerConfig.enabled ? "Yes" : "No",
                buzzerConfig.pin,
                buzzerConfig.volume);
  for (int i = 0; i < BUZZER_EVENT_COUNT; i++) {
    Serial.printf(PSTR("  %-14s → enabled=%s | sound=%d | repeat=%s\n"),
                  buzzerEventNames[i],
                  buzzerConfig.eventEnabled[i] ? "Yes" : "No",
                  buzzerConfig.eventSound[i],
                  buzzerConfig.eventRepeat[i] ? "Yes" : "No");
  }
  Serial.println(F("========================================"));
  Serial.println();
}

void replaceIconTokens(String &msg, int &totalPixelWidth) {
  struct IconMap {
    const char *token;
    const char *glyph;
    int pixelWidth;
  };

  static const IconMap icons[] = {
    { "[NOTEMP]", "\x01", 28 },
    { "[NONTP]", "\x02", 24 },
    { "[WIFI]", "\x03", 13 },
    { "[INFO]", "\x04", 26 },
    { "[AP]", "\x05", 9 },
    { "[C]", "\x06", 4 },
    { "[F]", "\x07", 4 },
    { "[TIMEISUP]", "\x08", 32 },
    { "[TIMEISUPINVERTED]", "\x09", 32 },
    { "[SUNNY]", "\x0C", 8 },
    { "[CLOUDY]", "\x0D", 8 },
    { "[NODATA]", "\x0F", 28 },
    { "[RAINY]", "\x10", 8 },
    { "[THUNDER]", "\x11", 8 },
    { "[SNOWY]", "\x12", 8 },
    { "[WINDY]", "\x13", 8 },
    { "[CLOCK]", "\x14", 8 },
    { "[ALARMICON]", "\x15", 9 },
    { "[UPDATE]", "\x16", 8 },
    { "[BATTERYEMPTY]", "\x17", 8 },
    { "[BATTERY33]", "\x18", 8 },
    { "[BATTERY66]", "\x19", 8 },
    { "[BATTERYFULL]", "\x1A", 8 },
    { "[BOLT]", "\x1B", 4 },
    { "[HOUSE]", "\x1C", 7 },
    { "[TEMP]", "\x1D", 8 },
    { "[MUSICNOTE]", "\x1E", 7 },
    { "[PLAY]", "\x1F", 4 },
    { "[SPACE]", "\x20", 1 },
    { "[PAUSE]", "\x7F", 5 },
    { "[EURO]", "\x80", 5 },
    { "[SPEAKER]", "\x81", 8 },
    { "[SPEAKEROFF]", "\x82", 8 },
    { "[RED]", "\x83", 6 },
    { "[UP]", "\x86", 3 },
    { "[DOWN]", "\x88", 3 },
    { "[RIGHT]", "\x8B", 8 },
    { "[LEFT]", "\x8D", 8 },
    { "[TALK]", "\x8E", 7 },
    { "[HEART]", "\x8F", 7 },
    { "[CHECK]", "\x90", 5 },
    { "[INSTA]", "\x9B", 8 },
    { "[TV]", "\x9C", 11 },
    { "[YOUTUBE]", "\x9D", 8 },
    { "[BELL]", "\x9E", 6 },
    { "[LOCK]", "\x9F", 7 },
    { "[PERSON]", "\xA0", 6 },
    { "[HOURGLASS]", "\xA1", 5 },
    { "[HOURGLASS25]", "\xA2", 5 },
    { "[HOURGLASS75]", "\xA3", 5 },
    { "[HOURGLASSFULL]", "\xA4", 5 },
    { "[CAR]", "\xBB", 9 },
    { "[MAIL]", "\xA6", 9 },
    { "[CO2]", "\xA7", 13 },
    { "[MOON]", "\xA8", 9 },
    { "[SIGNAL1]", "\xA9", 8 },
    { "[SIGNAL2]", "\xAA", 8 },
    { "[SIGNAL3]", "\xAB", 8 },
    { "[DEG]", "\xB0", 3 },
    { "[SUNDAYJP]", "\xB1", 5 },
    { "[MONDAYJP]", "\xB2", 6 },
    { "[TUESDAYJP]", "\xB3", 7 },
    { "[WEDNESDAYJP]", "\xB4", 7 },
    { "[THURSDAYJP]", "\xB5", 7 },
    { "[FRIDAYJP]", "\xB6", 7 },
    { "[SATURDAYJP]", "\xB7", 7 },
    { "[MIST]", "\xB9", 7 },
    { "[ONAIR]", "\xCF", 26 }
  };

  // 1. Replace all tokens with glyphs first
  for (const auto &icon : icons) {
    msg.replace(icon.token, icon.glyph);
  }

  // 2. Calculate pixel width of the resulting string
  totalPixelWidth = 0;

  for (int i = 0; i < (int)msg.length(); i++) {
    bool isIcon = false;
    int charWidth = 0;
    unsigned char c = (unsigned char)msg[i];

    // Check for icons
    for (const auto &icon : icons) {
      if (c == (unsigned char)icon.glyph[0]) {
        charWidth = icon.pixelWidth;
        isIcon = true;
        break;
      }
    }

    if (!isIcon) {
      switch (c) {
        // --- 1 Pixel Wide ---
        case 32:  // Space
        case '!':
        case '.':
        case ':':
        case '\'':  // Single quote
        case '|':
        case 73:   // Capital 'I'
        case 184:  // Custom Dot (IP display)
          charWidth = 1;
          break;

        // --- 2 Pixels Wide ---
        case 40:  // (
        case 41:  // )
        case 59:  // ;
        case 91:  // [
        case 93:  // ]
        case ',':
          charWidth = 2;
          break;

        // --- 3 Pixels Wide ---
        case 34:
        case '?':
        case '-':
        case '_':  // Underscore
        case '/':
          charWidth = 3;
          break;

        // --- 4 Pixels Wide ---
        case 176:  // Degree symbol (Standard °)
          charWidth = 4;
          break;

        // --- 5 Pixels Wide ---
        case '#':
        case '&':
        case '$':
        case 0xA5:  //¥
        case '@':
        case '+':  // Moved here: 5px
          charWidth = 5;
          break;

        // --- 6 Pixels Wide ---
        case '%':
        case '~':
          charWidth = 6;
          break;

        // --- Default (Caps & Numbers) ---
        default:
          charWidth = 3;
          break;
      }
    }

    totalPixelWidth += charWidth;

    // Add 1px gap between characters/icons (except the last one)
    if (i < (int)msg.length() - 1) {
      totalPixelWidth += 1;
    }
  }
}

void handleCustomMessageLogic(AsyncWebServerRequest *request) {
  if (isNetworkBusy) {
    Serial.println(F("[MESSAGE] Rejected: Network Busy"));
    AsyncWebServerResponse *busyResponse = request->beginResponse(503, "text/plain", "Network Busy");
    busyResponse->addHeader("Access-Control-Allow-Origin", "*");
    request->send(busyResponse);
    return;
  }

  if (request->hasArg("message")) {
    String msg = request->arg("message");
    msg.trim();

    bool isClearRequest = (msg.length() == 0);
    bool incomingAllowInterrupt = true;

    // Detect Source: Header or URL param
    bool isFromUI = (request->header("X-Source") == "UI") || (request->arg("source") == "UI");
    bool isFromAPI = !isFromUI;

    // 1. Interrupt (allowInterrupt / interrupt)
    if (request->hasArg("allowInterrupt")) {
      incomingAllowInterrupt = (request->arg("allowInterrupt") == "1");
    } else if (request->hasArg("interrupt")) {
      incomingAllowInterrupt = (request->arg("interrupt") == "1");
    }

    // 2. Seconds (seconds / duration)
    int rawSecs = request->hasArg("seconds") ? request->arg("seconds").toInt() : (request->hasArg("duration") ? request->arg("duration").toInt() : 0);
    int newMessageDisplaySeconds = constrain(rawSecs, 0, 3600);

    // 3. Scrolls (scrolltimes / scrolls / scroll_times)
    int rawScrolls = request->hasArg("scrolltimes") ? request->arg("scrolltimes").toInt() : (request->hasArg("scrolls") ? request->arg("scrolls").toInt() : (request->hasArg("scroll_times") ? request->arg("scroll_times").toInt() : 0));
    int newMessageScrollTimes = constrain(rawScrolls, 0, 100);

    // 4. Speed & Big Numbers
    bool newMessageBigNumbers = (request->arg("bignumbers") == "1");
    int rawSpeed = request->hasArg("speed") ? request->arg("speed").toInt() : GENERAL_SCROLL_SPEED;
    int localSpeed = constrain(rawSpeed, 10, 200);

    // PROTECTION: Clock-only dimming
    if (!isClearRequest && clockOnlyDuringDimming && dimActive) {
      Serial.printf(PSTR("[MESSAGE] Rejected (Dimming Mode): '%s'\n"), msg.c_str());
      request->send(409, "text/plain", "Clock-only dimming mode active");
      return;
    }

    // Handle Alarm Commands
    if (handleAlarmCommand(msg)) {
      if (!allowInterrupt) {
        request->send(409, "text/plain", "Protected message active");
      } else if (clockOnlyDuringDimming && dimActive) {
        request->send(409, "text/plain", "Clock-only dimming active");
      } else {
        Serial.println(F("[MESSAGE] Alarm command executed."));
        request->send(200, "text/plain", "Alarm Command Executed");
      }
      return;
    }

    // Handle Pomodoro Commands (checked before Timer)
    if (handlePomodoroCommand(msg)) {
      if (!allowInterrupt) {
        request->send(409, "text/plain", "Protected message active");
      } else if (clockOnlyDuringDimming && dimActive) {
        request->send(409, "text/plain", "Clock-only dimming active");
      } else {
        Serial.println(F("[MESSAGE] Pomodoro command executed."));
        request->send(200, "text/plain", "Pomodoro Command Executed");
      }
      return;
    }

    // Handle Timer Commands
    if (handleTimerCommand(msg)) {
      if (!allowInterrupt) {
        request->send(409, "text/plain", "Protected message active");
      } else if (clockOnlyDuringDimming && dimActive) {
        request->send(409, "text/plain", "Clock-only dimming active");
      } else {
        Serial.println(F("[MESSAGE] Timer command executed."));
        request->send(200, "text/plain", "Timer Command Executed");
      }
      return;
    }

    // PROTECTION: Alarm Active
    if (alarmRinging && !isClearRequest && incomingAllowInterrupt) {
      Serial.println(F("[MESSAGE] Rejected: Alarm is ringing."));
      request->send(409, "text/plain", "Alarm active - stop or snooze alarm first");
      return;
    }

    // PROTECTION: Timer Active
    if (timerActive && !isClearRequest && incomingAllowInterrupt) {
      Serial.println(F("[MESSAGE] Rejected: Timer is active."));
      request->send(409, "text/plain", "Timer active - use priority message");
      return;
    }

    // PROTECTION: Protected Message Running
    if (!isClearRequest && !allowInterrupt && incomingAllowInterrupt) {
      Serial.println(F("[MESSAGE] Rejected: Protected message running"));
      request->send(409, "text/plain", "Display busy");
      return;
    }

    String filtered = cleanTextForDisplay(msg);
    messageDisplaySeconds = newMessageDisplaySeconds;
    messageScrollTimes = newMessageScrollTimes;
    messageBigNumbers = newMessageBigNumbers;

    // --- LOG: Consolidated Intent (Before Saving/Execution) ---
    Serial.printf(
      "[MESSAGE] Source=%s | msg='%s' | seconds=%d | scrolls=%d | speed=%d | big=%d | allowInterrupt=%d\n",
      isFromUI ? "UI" : "API",
      filtered.c_str(),
      messageDisplaySeconds,
      messageScrollTimes,
      localSpeed,
      messageBigNumbers,
      incomingAllowInterrupt);

    if (timerActive && !isClearRequest) {
      displayMode = (messageScrollTimes == 0 && messageDisplaySeconds == 0) ? 6 : 7;
      lastSwitch = millis();
      forceMessageRestart = true;
    }

    // --- CLEAR MESSAGE ---
    if (isClearRequest) {
      allowInterrupt = true;
      forceMessageRestart = true;
      if (isFromUI) {
        customMessage[0] = '\0';
        lastPersistentMessage[0] = '\0';
        messageStartTime = 0;
        currentScrollCount = 0;
        messageDisplaySeconds = 0;
        messageScrollTimes = 0;
        displayMode = 0;
        prevDisplayMode = 6;
        lastSwitch = millis();
        clockScrollDone = false;
        saveCustomMessageToConfig("");
        Serial.println(F("[MESSAGE] Full Clear (UI) completed."));
        request->send(200, "text/plain", "CLEARED (UI)");
      } else {
        customMessage[0] = '\0';
        messageStartTime = 0;
        currentScrollCount = 0;
        messageDisplaySeconds = 0;
        messageScrollTimes = 0;

        if (strlen(lastPersistentMessage) > 0) {
          strlcpy(customMessage, lastPersistentMessage, sizeof(customMessage));
          messageScrollSpeed = GENERAL_SCROLL_SPEED;
          displayMode = 6;
          prevDisplayMode = 0;
          Serial.println(F("[MESSAGE] Temp message cleared. Persistent restored."));
          request->send(200, "text/plain", "CLEARED (API temporary, persistent restored)");
        } else {
          displayMode = 0;
          lastSwitch = millis();
          prevDisplayMode = 6;
          clockScrollDone = false;
          Serial.println(F("[MESSAGE] Temp message cleared. No persistent found."));
          request->send(200, "text/plain", "CLEARED (API temporary, no persistent)");
        }
      }
      return;
    }

    // --- STORE & ACTIVATE ---
    if (isFromAPI) {
      filtered.toCharArray(customMessage, sizeof(customMessage));
      messageScrollSpeed = localSpeed;
    } else {
      filtered.toCharArray(customMessage, sizeof(customMessage));
      strlcpy(lastPersistentMessage, customMessage, sizeof(lastPersistentMessage));
      messageScrollSpeed = GENERAL_SCROLL_SPEED;
      saveCustomMessageToConfig(lastPersistentMessage);
    }

    allowInterrupt = incomingAllowInterrupt;
    displayMode = 6;
    prevDisplayMode = 0;
    messageStartTime = millis();
    currentScrollCount = 0;
    clockScrollDone = false;
    forceMessageRestart = true;

    // --- FINAL RESPONSE ---
    String responseMsg = "OK (" + String(isFromUI ? "UI" : "API") + ")";
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", responseMsg);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  } else {
    Serial.println(F("[MESSAGE] Error: Missing message parameter"));
    request->send(400, "text/plain", "Missing message parameter");
  }
}

// -----------------------------------------------------------------------------
// Web Server and Captive Portal
// -----------------------------------------------------------------------------
void handleCaptivePortal(AsyncWebServerRequest *request);

String sanitizeValue(const char *key) {
  if (strcmp(key, "ssid") == 0) {
    return getSafeSsid();
  }
  if (strcmp(key, "password") == 0) {
    return getSafePassword();
  }
  if (strcmp(key, "openWeatherApiKey") == 0) {
    return getSafeApiKey();
  }
  if (strcmp(key, "mode") == 0) {
    return isAPMode ? "ap" : "sta";
  }

  return "";
}

struct StatusStreamState {
  int section = 0;
  String buf;
  size_t bufPos = 0;
  SnsType snsType;
  time_t nowTime;
};

static String statusSectionJson(int section, SnsType snsType, time_t nowTime) {
  switch (section) {
    case 0:
      {
        JsonDocument doc;
        doc["id"] = deviceHostname;
        doc["version"] = FIRMWARE_VERSION;
        doc["hardware"] = "MAX7219_FC16";
#if defined(ESP32)
        doc["board"] = "ESP32";
#elif defined(ESP8266)
        doc["board"] = "ESP8266";
#else
        doc["board"] = "unknown";
#endif
        String json;
        serializeJson(doc, json);
        return "{\"identity\":" + json;
      }
    case 1:
      {
        JsonDocument doc;
        doc["displayMode"] = displayMode;
        doc["displayBusy"] = (displayMode == 6 || displayMode == 7);
        doc["allowInterrupt"] = allowInterrupt;
        switch (displayMode) {
          case 0: doc["mode"] = "clock"; break;
          case 1: doc["mode"] = "weather"; break;
          case 2: doc["mode"] = "weather_desc"; break;
          case 3: doc["mode"] = "countdown"; break;
          case 4:
            if (snsType == SNS_YOUTUBE) doc["mode"] = "youtube";
            else if (snsType == SNS_INSTAGRAM) doc["mode"] = "instagram";
            else doc["mode"] = "nightscout";
            break;
          case 5: doc["mode"] = "date"; break;
          case 6: doc["mode"] = "message"; break;
          case 7: doc["mode"] = "timer"; break;
          case 8: doc["mode"] = "alarm"; break;
          default: doc["mode"] = "cycling"; break;
        }
        doc["message"] = (strlen(customMessage) > 0) ? customMessage : "";
        doc["displayOff"] = displayOff;
        doc["brightness"] = brightness;
        doc["lastBrightnessBeforeOff"] = lastBrightnessBeforeOff;
        String json;
        serializeJson(doc, json);
        return ",\"display\":" + json;
      }
    case 2:
      {
        JsonDocument doc;
        doc["device_runtime"] = formatTotalRuntime();
        doc["session_runtime"] = millis() / 1000;
        doc["wifi_signal"] = WiFi.RSSI();
        doc["mdns_url"] = String(deviceHostname) + ".local";
        doc["time_synced"] = ntpSyncSuccessful;
        struct tm timeinfo;
        localtime_r(&nowTime, &timeinfo);
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        doc["localTime"] = String(buffer);
        doc["epochTime"] = static_cast<uint32_t>(nowTime);
        String json;
        serializeJson(doc, json);
        return ",\"runtime\":" + json;
      }
    case 3:
      {
        JsonDocument doc;
        doc["enabled"] = countdownEnabled;
        doc["targetTimestamp"] = countdownTargetTimestamp;
        doc["label"] = String(countdownLabel);
        doc["isDramatic"] = isDramaticCountdown;
        long long remaining = static_cast<long long>(countdownTargetTimestamp) - static_cast<long long>(nowTime);
        doc["remaining"] = countdownEnabled ? (remaining > 0 ? remaining : 0) : 0;
        String json;
        serializeJson(doc, json);
        return ",\"countdown\":" + json;
      }
    case 4:
      {
        JsonDocument doc;
        if (weatherAvailable && weatherDescription.length() > 0) {
          doc["currentTemperature"] = String(currentTemp).toInt();
          doc["weatherDescription"] = weatherDescription;
          doc["icon"] = weatherIcon;
          doc["currentHumidity"] = currentHumidity;
        } else {
          doc["currentTemperature"] = JsonVariant();
          doc["weatherDescription"] = JsonVariant();
          doc["icon"] = JsonVariant();
          doc["currentHumidity"] = JsonVariant();
        }
        doc["sunriseHour"] = weatherAvailable ? sunriseHour : JsonVariant();
        doc["sunriseMinute"] = weatherAvailable ? sunriseMinute : JsonVariant();
        doc["sunsetHour"] = weatherAvailable ? sunsetHour : JsonVariant();
        doc["sunsetMinute"] = weatherAvailable ? sunsetMinute : JsonVariant();
        String json;
        serializeJson(doc, json);
        return ",\"weather\":" + json;
      }
    case 5:
      {
        JsonDocument doc;
        doc["active"] = (displayMode == 4);
        doc["glucose"] = (currentGlucose != -1) ? currentGlucose : JsonVariant();
        doc["trend"] = (currentDirection.length() > 0 && currentDirection != "?") ? currentDirection : JsonVariant();
        if (lastGlucoseTime > 0) {
          doc["lastReadingEpoch"] = lastGlucoseTime;
          int minutes = static_cast<int>(difftime(time(nullptr), lastGlucoseTime) / 60.0);
          doc["minutesSinceReading"] = (minutes > 0) ? minutes : 0;
          doc["isOutdated"] = (minutes > NIGHTSCOUT_IDLE_THRESHOLD_MIN);
        } else {
          doc["lastReadingEpoch"] = nullptr;
          doc["minutesSinceReading"] = nullptr;
          doc["isOutdated"] = true;
        }
        String json;
        serializeJson(doc, json);
        return ",\"nightscout\":" + json;
      }
    case 6:
      {
        JsonDocument doc;
        switch (snsType) {
          case SNS_YOUTUBE: doc["type"] = "youtube"; break;
          case SNS_INSTAGRAM: doc["type"] = "instagram"; break;
          case SNS_NIGHTSCOUT: doc["type"] = "nightscout"; break;
          case SNS_RSS: doc["type"] = "rss"; break;
          default: doc["type"] = "none"; break;
        }
        doc["youtubeSubscribers"] = (youtubeSubscribers >= 0) ? youtubeSubscribers : JsonVariant();
        doc["instagramFollowers"] = (instagramFollowers >= 0) ? instagramFollowers : JsonVariant();
        doc["rssTitle"] = (rssTitle.length() > 0) ? rssTitle : JsonVariant();
        String json;
        serializeJson(doc, json);
        return ",\"sns\":" + json;
      }
    case 7:
      {
        JsonDocument doc;
        doc["ssid"] = String(ssid);
        doc["openWeatherApiKey"] = (strlen(openWeatherApiKey) > 0) ? "***HIDDEN***" : "";
        doc["openWeatherCity"] = String(openWeatherCity);
        doc["weatherUnits"] = String(weatherUnits);
        doc["clockDuration"] = clockDuration;
        doc["weatherDuration"] = weatherDuration;
        doc["timeZone"] = String(timeZone);
        doc["language"] = String(language);
        doc["flipDisplay"] = flipDisplay;
        doc["twelveHourToggle"] = twelveHourToggle;
        doc["showDate"] = showDate;
        doc["showHumidity"] = showHumidity;
        doc["ntpServer1"] = String(ntpServer1);
        String nsUrl = String(ntpServer2);
        int tokenIdx = nsUrl.indexOf("token=");
        if (tokenIdx == -1) tokenIdx = nsUrl.indexOf("api_key=");
        if (tokenIdx != -1) {
          int keyStart = nsUrl.indexOf('=', tokenIdx) + 1;
          doc["ntpServer2"] = nsUrl.substring(0, keyStart) + "***HIDDEN***";
        } else {
          doc["ntpServer2"] = nsUrl;
        }
        String json;
        serializeJson(doc, json);
        return ",\"config\":" + json;
      }
    case 8:
      {
        JsonDocument doc;
        doc["dimmingEnabled"] = dimmingEnabled;
        doc["dimStartHour"] = dimStartHour;
        doc["dimStartMinute"] = dimStartMinute;
        doc["dimEndHour"] = dimEndHour;
        doc["dimEndMinute"] = dimEndMinute;
        doc["autoDimmingEnabled"] = autoDimmingEnabled;
        doc["clockOnlyDuringDimming"] = clockOnlyDuringDimming;
        String json;
        serializeJson(doc, json);
        return ",\"dimming\":" + json;
      }
    case 9:
      {
        JsonDocument doc;
        doc["hideDonationMsg"] = hideDonationMsg;
        if (!hideDonationMsg && nextDonationTime > 0) {
          struct tm t;
          localtime_r(&nextDonationTime, &t);
          char buf[32];
          strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &t);
          doc["nextDonationTime"] = String(buf);
        } else {
          doc["nextDonationTime"] = "N/A";
        }
        String json;
        serializeJson(doc, json);
        return ",\"donations\":" + json;
      }
    case 10:
      {
        JsonDocument doc;
        doc["clk"] = CLK_PIN;
        doc["cs"] = CS_PIN;
        doc["data"] = DATA_PIN;
        String json;
        serializeJson(doc, json);
        return ",\"pins\":" + json;
      }
    case 11:
      {
        JsonDocument doc;
        JsonArray buttons = doc.to<JsonArray>();
        for (int i = 0; i < 4; i++) {
          JsonObject b = buttons.createNestedObject();
          b["pin"] = btnCfg[i].pin;
          b["shortAction"] = btnCfg[i].shortAct;
          b["longAction"] = btnCfg[i].longAct;
        }
        String json;
        serializeJson(doc, json);
        return ",\"buttons\":" + json;
      }
    case 12:
      {
        JsonDocument doc;
        doc["enabled"] = buzzerConfig.enabled;
        doc["pin"] = buzzerConfig.pin;
        doc["volume"] = buzzerConfig.volume;
        doc["playing"] = (buzzerState == BUZZER_PLAYING);
        doc["repeating"] = buzzerRepeating;
        JsonArray events = doc.createNestedArray("events");
        for (int i = 0; i < BUZZER_EVENT_COUNT; i++) {
          JsonObject e = events.createNestedObject();
          e["name"] = buzzerEventNames[i];
          e["enabled"] = buzzerConfig.eventEnabled[i];
          e["sound"] = buzzerConfig.eventSound[i];
          e["repeat"] = buzzerConfig.eventRepeat[i];
        }
        String json;
        serializeJson(doc, json);
        return ",\"buzzer\":" + json;
      }
    case 13:
      {
        DynamicJsonDocument doc(768);
        JsonArray alarms = doc.createNestedArray("alarms");
        for (int i = 0; i < MAX_ALARMS; i++) {
          JsonObject al = alarms.createNestedObject();
          al["enabled"] = alarmConfigs[i].enabled;
          al["hour"] = alarmConfigs[i].hour;
          al["minute"] = alarmConfigs[i].minute;
          JsonArray days = al.createNestedArray("days");
          for (int d = 0; d < 7; d++) days.add(alarmConfigs[i].days[d]);
          al["snoozeMinutes"] = alarmConfigs[i].snoozeMinutes;
          al["brightness"] = alarmConfigs[i].brightness;
          al["sound"] = alarmConfigs[i].sound;
        }
        doc["ringing"] = alarmRinging;
        doc["ringingIndex"] = alarmRingingIndex;
        String json;
        serializeJson(doc, json);
        return ",\"alarm\":" + json;
      }
    case 14:
      {
        JsonDocument doc;

#if defined(ESP32)
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t maxAlloc = ESP.getMaxAllocHeap();

        doc["freeHeap"] = freeHeap;
        doc["minFreeHeap"] = ESP.getMinFreeHeap();
        doc["maxAllocHeap"] = maxAlloc;
        doc["heapFrag"] = (freeHeap > 0)
                            ? (100 - (maxAlloc * 100 / freeHeap))
                            : 0;

        esp_reset_reason_t reason = esp_reset_reason();
        doc["resetReason"] = (int)reason;

        const char *reasonText = "UNKNOWN";

        switch (reason) {
          case ESP_RST_UNKNOWN: reasonText = "UNKNOWN"; break;
          case ESP_RST_POWERON: reasonText = "POWERON"; break;
          case ESP_RST_EXT: reasonText = "EXTERNAL"; break;
          case ESP_RST_SW: reasonText = "SOFTWARE"; break;
          case ESP_RST_PANIC: reasonText = "PANIC"; break;
          case ESP_RST_INT_WDT: reasonText = "INT_WDT"; break;
          case ESP_RST_TASK_WDT: reasonText = "TASK_WDT"; break;
          case ESP_RST_WDT: reasonText = "WDT"; break;
          case ESP_RST_DEEPSLEEP: reasonText = "DEEPSLEEP"; break;
          case ESP_RST_BROWNOUT: reasonText = "BROWNOUT"; break;
          case ESP_RST_SDIO: reasonText = "SDIO"; break;
          default: reasonText = "OTHER"; break;
        }

        doc["resetReasonText"] = reasonText;

#elif defined(ESP8266)
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t maxBlock = ESP.getMaxFreeBlockSize();

        doc["freeHeap"] = freeHeap;
        doc["maxAllocHeap"] = maxBlock;
        doc["heapFrag"] = (freeHeap > 0)
                            ? (100 - (maxBlock * 100 / freeHeap))
                            : 0;

        doc["resetReason"] = ESP.getResetReason();
#endif

        doc["uptime"] = millis() / 1000;
        doc["sketchFree"] = ESP.getFreeSketchSpace();

        doc["ip"] = WiFi.localIP().toString();
        doc["wifiRssi"] = WiFi.RSSI();
        doc["wifiChannel"] = WiFi.channel();
        doc["wifiBssid"] = WiFi.BSSIDstr();

        String json;
        serializeJson(doc, json);

        return ",\"debug\":" + json;
      }
    case 15:
      return "}";
    default:
      return "";
  }
}

void setupWebServer() {
  Serial.println(F("[WEBSERVER] Setting up web server..."));

  // 1. Global CORS headers (Required for Chrome Extension)
  // These headers allow the browser to verify the security policy for all routes.
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, X-Source");

  // Root handler with BOTH CORS and Cache-Prevention
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
    // Anti-Caching Headers: Ensures the browser always fetches the latest UI
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    // CORS Header (Manual insurance for strict browser scrutiny)
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(204);
  });

  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[WEBSERVER] Request: /config.json"));
    struct ConfigStreamState {
      File f;
      String out;
    };

    AsyncWebServerResponse *response = request->beginChunkedResponse(
      "application/json",
      [state = std::make_shared<ConfigStreamState>()](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t {
        if (index == 0) {
          state->f = LittleFS.open("/config.json", "r");
          state->out = String();
        }
        if (!state->f) return 0;

        while (state->out.length() < maxLen && state->f.available()) {
          char c = state->f.read();
          if (c == '"') {
            String possibleKey = "\"";
            while (state->f.available()) {
              char k = state->f.read();
              possibleKey += k;
              if (k == '"') break;
            }
            if (possibleKey == "\"ssid\"" || possibleKey == "\"password\"" || possibleKey == "\"openWeatherApiKey\"") {
              state->out += possibleKey;
              while (state->f.available()) {
                char x = state->f.read();
                state->out += x;
                if (x == '"') break;
              }
              while (state->f.available()) {
                char x = state->f.read();
                if (x == '"') break;
              }
              String cleanKey = possibleKey.substring(1, possibleKey.length() - 1);
              state->out += sanitizeValue(cleanKey.c_str());
              state->out += "\"";
              continue;
            }
            state->out += possibleKey;
            continue;
          }
          state->out += c;
        }

        if (state->out.length() == 0) {
          if (state->f) state->f.close();
          return 0;
        }

        size_t sendLen = min(maxLen, state->out.length());
        memcpy(buffer, state->out.c_str(), sendLen);
        state->out.remove(0, sendLen);
        return sendLen;
      });

    response->addHeader("Connection", "close");
    request->send(response);
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (getLargestFreeBlock() < 4000) {
      Serial.printf(PSTR("[SAVE] Aborting: heap too fragmented (%u largest)\n"), getLargestFreeBlock());
      request->send(503, "application/json", "{\"error\":\"Device busy, please try again in a moment.\"}");
      return;
    }
    Serial.println(F("[WEBSERVER] Request: /save"));
    JsonDocument doc;

    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      Serial.println(F("[WEBSERVER] Existing config.json found, loading for update..."));
      DeserializationError err = deserializeJson(doc, configFile);
      configFile.close();
      if (err) {
        Serial.print(F("[WEBSERVER] Error parsing existing config.json: "));
        Serial.println(err.f_str());
      }
    } else {
      Serial.println(F("[WEBSERVER] config.json not found, starting with empty doc for save."));
    }

    for (int i = 0; i < request->params(); i++) {
      const AsyncWebParameter *p = request->getParam(i);
      String n = p->name();
      String v = p->value();

      if (n == "brightness") doc[n] = v.toInt();
      else if (n == "clockDuration") doc[n] = v.toInt();
      else if (n == "weatherDuration") doc[n] = v.toInt();
      else if (n == "flipDisplay") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "twelveHourToggle") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "showDayOfWeek") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "showDate") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "showHumidity") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "colonBlinkEnabled") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "dimStartHour") doc[n] = v.toInt();
      else if (n == "dimStartMinute") doc[n] = v.toInt();
      else if (n == "dimEndHour") doc[n] = v.toInt();
      else if (n == "dimEndMinute") doc[n] = v.toInt();
      else if (n == "dimBrightness") {
        if (v == "Off" || v == "off") doc[n] = -1;
        else doc[n] = v.toInt();
      } else if (n == "showWeatherDescription") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "dimmingEnabled") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "clockOnlyDuringDimming") {
        doc[n] = (v == "true" || v == "on" || v == "1");
      } else if (n == "weatherUnits") doc[n] = v;
      else if (n == "hostname") doc[n] = v;
      else if (n == "password") {
        if (v != "********") {
          doc[n] = v;  // saves new password OR empty string (open network)
        } else {
          Serial.println(F("[SAVE] Password unchanged (masked)."));
        }
      } else if (n == "ssid") {
        if (v != "********" && v.length() > 0) {
          doc[n] = v;
        } else {
          Serial.println(F("[SAVE] SSID unchanged."));
        }
      } else if (n == "openWeatherApiKey") {
        if (v != "********************************") {  // ignore mask only
          doc[n] = v;                                   // save new key (even if empty)
          Serial.print(F("[SAVE] API key updated: "));
          Serial.println(v.length() == 0 ? "(empty)" : v);
        } else {
          Serial.println(F("[SAVE] API key unchanged (mask ignored)."));
        }
      } else {
        doc[n] = v;
      }
    }

    bool newCountdownEnabled = (request->hasParam("countdownEnabled", true) && (request->getParam("countdownEnabled", true)->value() == "true" || request->getParam("countdownEnabled", true)->value() == "on" || request->getParam("countdownEnabled", true)->value() == "1"));
    String countdownDateStr = request->hasParam("countdownDate", true) ? request->getParam("countdownDate", true)->value() : "";
    String countdownTimeStr = request->hasParam("countdownTime", true) ? request->getParam("countdownTime", true)->value() : "";
    String countdownLabelStr = request->hasParam("countdownLabel", true) ? request->getParam("countdownLabel", true)->value() : "";
    bool newIsDramaticCountdown = (request->hasParam("isDramaticCountdown", true) && (request->getParam("isDramaticCountdown", true)->value() == "true" || request->getParam("isDramaticCountdown", true)->value() == "on" || request->getParam("isDramaticCountdown", true)->value() == "1"));

    time_t newTargetTimestamp = 0;
    if (newCountdownEnabled && countdownDateStr.length() > 0 && countdownTimeStr.length() > 0) {
      int year = countdownDateStr.substring(0, 4).toInt();
      int month = countdownDateStr.substring(5, 7).toInt();
      int day = countdownDateStr.substring(8, 10).toInt();
      int hour = countdownTimeStr.substring(0, 2).toInt();
      int minute = countdownTimeStr.substring(3, 5).toInt();

      struct tm tm;
      tm.tm_year = year - 1900;
      tm.tm_mon = month - 1;
      tm.tm_mday = day;
      tm.tm_hour = hour;
      tm.tm_min = minute;
      tm.tm_sec = 0;
      tm.tm_isdst = -1;

      newTargetTimestamp = mktime(&tm);
      if (newTargetTimestamp == (time_t)-1) {
        Serial.println(F("[SAVE] Error converting countdown date/time to timestamp."));
        newTargetTimestamp = 0;
      } else {
        Serial.printf(PSTR("[SAVE] Converted countdown target: %s -> %lu\n"), countdownDateStr.c_str(), newTargetTimestamp);
      }
    }

    JsonObject countdownObj = doc.createNestedObject("countdown");
    countdownObj["enabled"] = newCountdownEnabled;
    countdownObj["targetTimestamp"] = newTargetTimestamp;
    countdownObj["label"] = countdownLabelStr;
    countdownObj["isDramaticCountdown"] = newIsDramaticCountdown;

#if defined(ESP8266)
    FSInfo fs_info;
    LittleFS.info(fs_info);
    Serial.printf(PSTR("[SAVE] LittleFS total bytes: %u, used bytes: %u\n"), fs_info.totalBytes, fs_info.usedBytes);
#elif defined(ESP32)
      size_t total = LittleFS.totalBytes();
      size_t used = LittleFS.usedBytes();
      Serial.printf(PSTR("[SAVE] LittleFS total bytes: %u, used bytes: %u\n"), (unsigned)total, (unsigned)used);
#endif

    if (LittleFS.exists("/config.json")) {
      Serial.println(F("[SAVE] Renaming /config.json to /config.bak"));
      LittleFS.rename("/config.json", "/config.bak");
    }
    File f = LittleFS.open("/config.json", "w");
    if (!f) {
      Serial.println(F("[SAVE] ERROR: Failed to open /config.json for writing!"));
      JsonDocument errorDoc;
      errorDoc[F("error")] = "Failed to write config file.";
      String response;
      serializeJson(errorDoc, response);
      request->send(500, "application/json", response);
      return;
    }

    size_t bytesWritten = serializeJson(doc, f);
    Serial.printf(PSTR("[SAVE] Bytes written to /config.json: %u\n"), bytesWritten);
    f.close();
    Serial.println(F("[SAVE] /config.json file closed."));

    if (bytesWritten == 0) {
      Serial.println(F("[SAVE] ERROR: 0 bytes written to config.json!"));
      JsonDocument errorDoc;
      errorDoc[F("error")] = "Write failed: 0 bytes written.";
      String response;
      serializeJson(errorDoc, response);
      request->send(500, "application/json", response);
      return;
    }

    Serial.println(F("[SAVE] Config write verified (byte count check)."));

    JsonDocument okDoc;
    if (doc.containsKey("hostname")) {
      deviceHostname = doc["hostname"].as<String>();
    }
    strlcpy(customMessage, doc["customMessage"] | "", sizeof(customMessage));
    okDoc[F("message")] = "Saved successfully. Rebooting...";
    String response;
    serializeJson(okDoc, response);
    request->send(200, "application/json", response);
    Serial.println(F("[WEBSERVER] Sending success response and scheduling reboot..."));

    request->onDisconnect([]() {
      Serial.println(F("[WEBSERVER] Client disconnected, rebooting ESP..."));
      saveUptime();
      isRebooting = true;
      delay(100);  // ensure file is written
      ESP.restart();
    });
  });

  server.on("/restore", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[WEBSERVER] Request: /restore"));
    if (LittleFS.exists("/config.bak")) {
      File src = LittleFS.open("/config.bak", "r");
      if (!src) {
        Serial.println(F("[WEBSERVER] Failed to open /config.bak"));
        JsonDocument errorDoc;
        errorDoc[F("error")] = "Failed to open backup file.";
        String response;
        serializeJson(errorDoc, response);
        request->send(500, "application/json", response);
        return;
      }
      File dst = LittleFS.open("/config.json", "w");
      if (!dst) {
        src.close();
        Serial.println(F("[WEBSERVER] Failed to open /config.json for writing"));
        JsonDocument errorDoc;
        errorDoc[F("error")] = "Failed to open config for writing.";
        String response;
        serializeJson(errorDoc, response);
        request->send(500, "application/json", response);
        return;
      }

      while (src.available()) {
        dst.write(src.read());
      }
      src.close();
      dst.close();

      JsonDocument okDoc;
      okDoc[F("message")] = "✅ Backup restored! Device will now reboot.";
      String response;
      serializeJson(okDoc, response);
      request->send(200, "application/json", response);
      request->onDisconnect([]() {
        Serial.println(F("[WEBSERVER] Rebooting after restore..."));
        saveUptime();
        delay(100);  // ensure file is written
        ESP.restart();
      });

    } else {
      Serial.println(F("[WEBSERVER] No backup found"));
      JsonDocument errorDoc;
      errorDoc[F("error")] = "No backup found.";
      String response;
      serializeJson(errorDoc, response);
      request->send(404, "application/json", response);
    }
  });

  server.on("/ap_status", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.print(F("[WEBSERVER] Request: /ap_status = "));
    Serial.println(isAPMode);
    String json = "{\"isAP\": ";
    json += (isAPMode) ? "true" : "false";
    json += "}";
    request->send(200, "application/json", json);
  });

  auto setHandler = [](AsyncWebServerRequest *request) {
    String action = request->url().substring(5);  // strips "/set_" → e.g. "brightness"
    String value = "";
    if (request->hasParam("value", true)) {
      value = request->getParam("value", true)->value();
    } else if (request->params() > 0) {
      value = request->getParam(0)->value();
    }
    executeAction(action, value);
    request->send(200, "application/json", "{\"ok\":true}");
  };

  server.on("/set_brightness", HTTP_POST, setHandler);
  server.on("/set_flip", HTTP_POST, setHandler);
  server.on("/set_twelvehour", HTTP_POST, setHandler);
  server.on("/set_dayofweek", HTTP_POST, setHandler);
  server.on("/set_showdate", HTTP_POST, setHandler);
  server.on("/set_humidity", HTTP_POST, setHandler);
  server.on("/set_colon_blink", HTTP_POST, setHandler);
  server.on("/set_language", HTTP_POST, setHandler);
  server.on("/set_weatherdesc", HTTP_POST, setHandler);
  server.on("/set_units", HTTP_POST, setHandler);
  server.on("/set_countdown_enabled", HTTP_POST, setHandler);
  server.on("/set_dramatic_countdown", HTTP_POST, setHandler);
  server.on("/set_clock_only_dimming", HTTP_POST, setHandler);

  // --- Custom Message Endpoint ---
  server.on("/set_custom_message", HTTP_ANY, [](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      AsyncWebServerResponse *response = request->beginResponse(200);
      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      response->addHeader("Access-Control-Allow-Headers", "Content-Type, X-Source");
      request->send(response);
      return;
    }
    handleCustomMessageLogic(request);
  });

  server.on("/set_hide_donation", HTTP_POST, [](AsyncWebServerRequest *request) {
    String value = "";
    if (request->hasParam("value", true)) {
      value = request->getParam("value", true)->value();
    } else if (request->params() > 0) {
      value = request->getParam(0)->value();
    }
    hideDonationMsg = (value == "1" || value == "true" || value == "on");
    saveConfigRuntime();
    Serial.printf(PSTR("[DONATION] hideDonationMsg set to %s\n"), hideDonationMsg ? "true" : "false");
    request->send(200, "application/json", "{\"ok\":true}");
  });


  // --- Physical Buttons: get config ---
  server.on("/get_buttons", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray used = doc.createNestedArray("usedPins");
    used.add(CLK_PIN);
    used.add(CS_PIN);
    used.add(DATA_PIN);
    if (buzzerConfig.pin != 255) used.add(buzzerConfig.pin);
    JsonArray btns = doc.createNestedArray("buttons");
    for (int i = 0; i < 4; i++) {
      JsonObject b = btns.createNestedObject();
      b["pin"] = btnCfg[i].pin;
      b["shortAction"] = btnCfg[i].shortAct;
      b["longAction"] = btnCfg[i].longAct;
    }
    String response;
    serializeJson(doc, response);
    AsyncWebServerResponse *res = request->beginResponse(200, "application/json", response);
    res->addHeader("Connection", "close");
    request->send(res);
  });

  // --- Physical Buttons: save config (config.json, no reboot) ---
  server.on("/save_buttons", HTTP_POST, [](AsyncWebServerRequest *request) {
    int forbidden[] = { CLK_PIN, CS_PIN, DATA_PIN };
    int newPins[4];
    String newShort[4], newLong[4];

    for (int i = 0; i < 4; i++) {
      String idx = String(i + 1);
      int pin = request->hasParam("btn" + idx + "_pin", true)
                  ? request->getParam("btn" + idx + "_pin", true)->value().toInt()
                  : -1;
      for (int f : forbidden) {
        if (pin == f) {
          pin = -1;
          break;
        }
      }
      newPins[i] = pin;
      newShort[i] = request->hasParam("btn" + idx + "_short", true)
                      ? request->getParam("btn" + idx + "_short", true)->value()
                      : "";
      newLong[i] = request->hasParam("btn" + idx + "_long", true)
                     ? request->getParam("btn" + idx + "_long", true)->value()
                     : "";
    }
    // Reject duplicate pins between buttons
    for (int i = 0; i < 4; i++)
      for (int j = i + 1; j < 4; j++)
        if (newPins[i] >= 0 && newPins[i] == newPins[j]) newPins[j] = -1;

    // Load existing config.json and patch the buttons key
    JsonDocument doc;
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      deserializeJson(doc, configFile);
      configFile.close();
    }

    doc.remove("buttons");
    JsonArray arr = doc.createNestedArray("buttons");
    for (int i = 0; i < 4; i++) {
      JsonObject b = arr.createNestedObject();
      b["pin"] = newPins[i];
      b["shortAction"] = newShort[i];
      b["longAction"] = newLong[i];
      // Update in-memory state immediately
      btnCfg[i].pin = newPins[i];
      btnCfg[i].shortAct = newShort[i];
      btnCfg[i].longAct = newLong[i];
    }

    if (LittleFS.exists("/config.json")) LittleFS.rename("/config.json", "/config.bak");
    File f = LittleFS.open("/config.json", "w");
    if (f) {
      serializeJson(doc, f);
      f.close();
    }

    setupButtons();  // apply new pinMode() without reboot
    buzzerHwSetup();
    Serial.println(F("[BUTTON] Config saved to config.json."));
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/get_buzzer", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray used = doc.createNestedArray("usedPins");
    used.add(CLK_PIN);
    used.add(CS_PIN);
    used.add(DATA_PIN);
    for (int i = 0; i < 4; i++) {
      if (btnCfg[i].pin >= 0) used.add(btnCfg[i].pin);
    }
    doc["enabled"] = buzzerConfig.enabled;
    doc["pin"] = buzzerConfig.pin;
    doc["volume"] = buzzerConfig.volume;
    JsonArray events = doc.createNestedArray("events");
    for (int i = 0; i < BUZZER_EVENT_COUNT; i++) {
      JsonObject e = events.createNestedObject();
      e["name"] = buzzerEventNames[i];
      e["enabled"] = buzzerConfig.eventEnabled[i];
      e["sound"] = buzzerConfig.eventSound[i];
      e["repeat"] = buzzerConfig.eventRepeat[i];
    }
    String response;
    serializeJson(doc, response);
    AsyncWebServerResponse *res = request->beginResponse(200, "application/json", response);
    res->addHeader("Connection", "close");
    request->send(res);
  });

  server.on("/save_buzzer", HTTP_POST, [](AsyncWebServerRequest *request) {
    int forbidden[8];
    int fCount = 0;
    forbidden[fCount++] = CLK_PIN;
    forbidden[fCount++] = CS_PIN;
    forbidden[fCount++] = DATA_PIN;
    for (int i = 0; i < 4; i++) {
      if (btnCfg[i].pin >= 0) forbidden[fCount++] = btnCfg[i].pin;
    }

    int pin = request->hasParam("pin", true) ? request->getParam("pin", true)->value().toInt() : 255;
    for (int i = 0; i < fCount; i++) {
      if (pin == forbidden[i]) {
        pin = 255;
        break;
      }
    }
    buzzerConfig.pin = (uint8_t)pin;

    if (request->hasParam("enabled", true)) {
      buzzerConfig.enabled = request->getParam("enabled", true)->value() == "1";
    }
    if (request->hasParam("volume", true)) {
      buzzerConfig.volume = constrain(request->getParam("volume", true)->value().toInt(), 1, 10);
    }
    for (int i = 0; i < BUZZER_EVENT_COUNT; i++) {
      String base = "evt" + String(i) + "_";
      if (request->hasParam(base + "enabled", true)) {
        buzzerConfig.eventEnabled[i] = request->getParam(base + "enabled", true)->value() == "1";
      }
      if (request->hasParam(base + "sound", true)) {
        buzzerConfig.eventSound[i] = request->getParam(base + "sound", true)->value().toInt();
      }
      if (request->hasParam(base + "repeat", true)) {
        buzzerConfig.eventRepeat[i] = request->getParam(base + "repeat", true)->value() == "1";
      }
    }

    saveConfigRuntime();
    buzzerHwSetup();  // re-init pin without reboot
    Serial.println(F("[BUZZER] Config saved to config.json."));
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/get_alarm", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    JsonArray alarms = doc.createNestedArray("alarms");
    for (int i = 0; i < MAX_ALARMS; i++) {
      JsonObject al = alarms.createNestedObject();
      al["enabled"] = alarmConfigs[i].enabled;
      al["hour"] = alarmConfigs[i].hour;
      al["minute"] = alarmConfigs[i].minute;
      JsonArray days = al.createNestedArray("days");
      for (int d = 0; d < 7; d++) days.add(alarmConfigs[i].days[d]);
      al["snoozeMinutes"] = alarmConfigs[i].snoozeMinutes;
      al["brightness"] = alarmConfigs[i].brightness;
      al["sound"] = alarmConfigs[i].sound;
    }
    doc["ringing"] = alarmRinging;
    doc["ringingIndex"] = alarmRingingIndex;

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->addHeader("Connection", "close");
    serializeJson(doc, *response);
    request->send(response);
  });

  server.on("/save_alarm", HTTP_POST, [](AsyncWebServerRequest *request) {
    for (int i = 0; i < MAX_ALARMS; i++) {
      String p = "alarm" + String(i) + "_";
      if (request->hasParam(p + "enabled", true)) {
        alarmConfigs[i].enabled = request->getParam(p + "enabled", true)->value() == "1";
      }
      if (request->hasParam(p + "hour", true)) {
        alarmConfigs[i].hour = constrain(request->getParam(p + "hour", true)->value().toInt(), 0, 23);
      }
      if (request->hasParam(p + "minute", true)) {
        alarmConfigs[i].minute = constrain(request->getParam(p + "minute", true)->value().toInt(), 0, 59);
      }
      for (int d = 0; d < 7; d++) {
        String dayKey = p + "day" + String(d);
        if (request->hasParam(dayKey, true)) {
          alarmConfigs[i].days[d] = request->getParam(dayKey, true)->value() == "1";
        }
      }
      if (request->hasParam(p + "snoozeMinutes", true)) {
        alarmConfigs[i].snoozeMinutes = constrain(request->getParam(p + "snoozeMinutes", true)->value().toInt(), 1, 60);
      }
      if (request->hasParam(p + "brightness", true)) {
        alarmConfigs[i].brightness = constrain(request->getParam(p + "brightness", true)->value().toInt(), 0, 15);
      }
      if (request->hasParam(p + "sound", true)) {
        int s = request->getParam(p + "sound", true)->value().toInt();
        if (s >= 1 && s <= 3) alarmConfigs[i].sound = (uint8_t)s;
      }
    }

    saveConfigRuntime();
    Serial.println(F("[ALARM] Config saved."));
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/save_weather", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (getLargestFreeBlock() < 4000) {
      request->send(503, "application/json", "{\"error\":\"Device busy, please try again in a moment.\"}");
      return;
    }
    if (request->hasParam("weatherDuration", true)) {
      weatherDuration = (unsigned long)request->getParam("weatherDuration", true)->value().toInt();
    }
    if (request->hasParam("openWeatherApiKey", true)) {
      String v = request->getParam("openWeatherApiKey", true)->value();
      if (v != "********************************") {  // ignore mask, same convention as /save
        strlcpy(openWeatherApiKey, v.c_str(), sizeof(openWeatherApiKey));
      }
    }
    if (request->hasParam("openWeatherCity", true)) {
      strlcpy(openWeatherCity, request->getParam("openWeatherCity", true)->value().c_str(), sizeof(openWeatherCity));
    }
    if (request->hasParam("openWeatherCountry", true)) {
      strlcpy(openWeatherCountry, request->getParam("openWeatherCountry", true)->value().c_str(), sizeof(openWeatherCountry));
    }
    if (request->hasParam("weatherUnits", true)) {
      strlcpy(weatherUnits, request->getParam("weatherUnits", true)->value().c_str(), sizeof(weatherUnits));
      tempSymbol = (strcmp(weatherUnits, "imperial") == 0) ? '\007' : '\006';
    }
    if (request->hasParam("showHumidity", true)) {
      String v = request->getParam("showHumidity", true)->value();
      showHumidity = (v == "true" || v == "on" || v == "1");
    }
    if (request->hasParam("showWeatherDescription", true)) {
      String v = request->getParam("showWeatherDescription", true)->value();
      showWeatherDescription = (v == "true" || v == "on" || v == "1");
    }

    shouldFetchWeatherNow = true;
    saveConfigRuntime();
    Serial.println(F("[WEATHER] Config saved."));
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/save_timedate", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (getLargestFreeBlock() < 4000) {
      request->send(503, "application/json", "{\"error\":\"Device busy, please try again in a moment.\"}");
      return;
    }
    if (request->hasParam("timeZone", true)) {
      strlcpy(timeZone, request->getParam("timeZone", true)->value().c_str(), sizeof(timeZone));
    }
    if (request->hasParam("clockDuration", true)) {
      clockDuration = (unsigned long)request->getParam("clockDuration", true)->value().toInt();
    }
    if (request->hasParam("ntpServer1", true)) {
      strlcpy(ntpServer1, request->getParam("ntpServer1", true)->value().c_str(), sizeof(ntpServer1));
    }
    if (request->hasParam("ntpServer2", true)) {
      strlcpy(ntpServer2, request->getParam("ntpServer2", true)->value().c_str(), sizeof(ntpServer2));
    }
    if (request->hasParam("showDayOfWeek", true)) {
      String v = request->getParam("showDayOfWeek", true)->value();
      showDayOfWeek = (v == "true" || v == "on" || v == "1");
    }
    if (request->hasParam("colonBlinkEnabled", true)) {
      String v = request->getParam("colonBlinkEnabled", true)->value();
      colonBlinkEnabled = (v == "true" || v == "on" || v == "1");
    }
    if (request->hasParam("showDate", true)) {
      String v = request->getParam("showDate", true)->value();
      showDate = (v == "true" || v == "on" || v == "1");
    }
    if (request->hasParam("twelveHourToggle", true)) {
      String v = request->getParam("twelveHourToggle", true)->value();
      twelveHourToggle = (v == "true" || v == "on" || v == "1");
    }

    saveConfigRuntime();
    setupTime();  // re-applies TZ and re-kicks NTP sync against (possibly new) servers — no reboot needed
    Serial.println(F("[TIMEDATE] Config saved."));
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/save_display", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (getLargestFreeBlock() < 4000) {
      request->send(503, "application/json", "{\"error\":\"Device busy, please try again in a moment.\"}");
      return;
    }
    if (request->hasParam("brightness", true)) {
      handleBrightnessChange(request->getParam("brightness", true)->value().toInt(), false);
    }
    if (request->hasParam("flipDisplay", true)) {
      String v = request->getParam("flipDisplay", true)->value();
      flipDisplay = (v == "true" || v == "on" || v == "1");
      P.setZoneEffect(0, flipDisplay, PA_FLIP_UD);
      P.setZoneEffect(0, flipDisplay, PA_FLIP_LR);
    }

    bool autoDimmingChecked = request->hasParam("autoDimmingEnabled", true) && request->getParam("autoDimmingEnabled", true)->value() == "true";
    bool customDimmingChecked = request->hasParam("dimmingEnabled", true) && request->getParam("dimmingEnabled", true)->value() == "true";
    if (autoDimmingChecked && customDimmingChecked) {
      autoDimmingEnabled = true;
      dimmingEnabled = false;
    } else {
      autoDimmingEnabled = autoDimmingChecked;
      dimmingEnabled = customDimmingChecked;
    }

    if (request->hasParam("dimStartHour", true)) {
      dimStartHour = constrain(request->getParam("dimStartHour", true)->value().toInt(), 0, 23);
    }
    if (request->hasParam("dimStartMinute", true)) {
      dimStartMinute = constrain(request->getParam("dimStartMinute", true)->value().toInt(), 0, 59);
    }
    if (request->hasParam("dimEndHour", true)) {
      dimEndHour = constrain(request->getParam("dimEndHour", true)->value().toInt(), 0, 23);
    }
    if (request->hasParam("dimEndMinute", true)) {
      dimEndMinute = constrain(request->getParam("dimEndMinute", true)->value().toInt(), 0, 59);
    }
    if (request->hasParam("dimBrightness", true)) {
      String v = request->getParam("dimBrightness", true)->value();
      dimBrightness = (v == "Off" || v == "off") ? -1 : v.toInt();
    }
    if (request->hasParam("clockOnlyDuringDimming", true)) {
      String v = request->getParam("clockOnlyDuringDimming", true)->value();
      clockOnlyDuringDimming = (v == "true" || v == "on" || v == "1");
    }

    saveConfigRuntime();
    Serial.println(F("[DISPLAY] Config saved."));
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/save_countdown", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (getLargestFreeBlock() < 4000) {
      request->send(503, "application/json", "{\"error\":\"Device busy, please try again in a moment.\"}");
      return;
    }
    bool newCountdownEnabled = request->hasParam("countdownEnabled", true) && (request->getParam("countdownEnabled", true)->value() == "true" || request->getParam("countdownEnabled", true)->value() == "on" || request->getParam("countdownEnabled", true)->value() == "1");
    String countdownDateStr = request->hasParam("countdownDate", true) ? request->getParam("countdownDate", true)->value() : "";
    String countdownTimeStr = request->hasParam("countdownTime", true) ? request->getParam("countdownTime", true)->value() : "";
    String countdownLabelStr = request->hasParam("countdownLabel", true) ? request->getParam("countdownLabel", true)->value() : "";
    bool newIsDramaticCountdown = request->hasParam("isDramaticCountdown", true) && (request->getParam("isDramaticCountdown", true)->value() == "true" || request->getParam("isDramaticCountdown", true)->value() == "on" || request->getParam("isDramaticCountdown", true)->value() == "1");

    // Same date/time -> epoch conversion as /save, kept identical on purpose
    time_t newTargetTimestamp = 0;
    if (newCountdownEnabled && countdownDateStr.length() > 0 && countdownTimeStr.length() > 0) {
      int year = countdownDateStr.substring(0, 4).toInt();
      int month = countdownDateStr.substring(5, 7).toInt();
      int day = countdownDateStr.substring(8, 10).toInt();
      int hour = countdownTimeStr.substring(0, 2).toInt();
      int minute = countdownTimeStr.substring(3, 5).toInt();

      struct tm tm;
      tm.tm_year = year - 1900;
      tm.tm_mon = month - 1;
      tm.tm_mday = day;
      tm.tm_hour = hour;
      tm.tm_min = minute;
      tm.tm_sec = 0;
      tm.tm_isdst = -1;

      newTargetTimestamp = mktime(&tm);
      if (newTargetTimestamp == (time_t)-1) {
        Serial.println(F("[COUNTDOWN] Error converting date/time to timestamp."));
        newTargetTimestamp = 0;
      }
    }

    countdownEnabled = newCountdownEnabled;
    countdownTargetTimestamp = newTargetTimestamp;
    strlcpy(countdownLabel, countdownLabelStr.c_str(), sizeof(countdownLabel));
    isDramaticCountdown = newIsDramaticCountdown;
    countdownFinished = false;  // clear any stale "finished" state on a fresh target

    saveCountdownConfig(countdownEnabled, countdownTargetTimestamp, countdownLabel);
    Serial.println(F("[COUNTDOWN] Config saved."));
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      AsyncWebServerResponse *response = request->beginResponse(200);
      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      response->addHeader("Access-Control-Allow-Headers", "Content-Type, X-Source");
      request->send(response);
      return;
    }
    handleCaptivePortal(request);
  });

  server.on(
    "/action", HTTP_ANY, [](AsyncWebServerRequest *request) {
      if (request->method() == HTTP_OPTIONS) {
        AsyncWebServerResponse *response = request->beginResponse(200);
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type, X-Source");
        request->send(response);
        return;
      }

      // Capture 'message' from either URL (GET) or Body (POST)
      if (request->hasArg("message")) {
        handleCustomMessageLogic(request);
      } else {
        // Handle other actions (brightness, etc.)
        if (request->params() > 0) {
          executeAction(request->getParam(0)->name(), request->getParam(0)->value());
          request->send(200, "text/plain", "OK");
        } else {
          request->send(400, "text/plain", "No parameters found");
        }
      }
    },
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // This empty body handler is CRITICAL for AsyncWebServer
      // to parse 'application/x-www-form-urlencoded' POST data!
    });

  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int scanStatus = WiFi.scanComplete();

    // -2 means scan not triggered, -1 means scan in progress
    if (scanStatus < -1 || scanStatus == WIFI_SCAN_FAILED) {
      // Start the asynchronous scan
      WiFi.scanNetworks(true);
      request->send(202, "application/json", "{\"status\":\"processing\"}");
    } else if (scanStatus == -1) {
      // Scan is currently running
      request->send(202, "application/json", "{\"status\":\"processing\"}");
    } else {
      // Scan finished (scanStatus >= 0)
      String json = "[";
      for (int i = 0; i < scanStatus; ++i) {
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i));
        json += "}";
        if (i < scanStatus - 1) json += ",";
      }
      json += "]";

      // Clean up scan results from memory
      WiFi.scanDelete();
      request->send(200, "application/json", json);
    }
  });

  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest *request) {
    String ip;

    if (WiFi.getMode() == WIFI_AP) {
      ip = WiFi.softAPIP().toString();  // usually 192.168.4.1
    } else if (WiFi.isConnected()) {
      ip = WiFi.localIP().toString();
    } else {
      ip = "—";
    }

    request->send(200, "text/plain", ip);
  });

  server.on("/uptime", HTTP_GET, [](AsyncWebServerRequest *request) {
    // 1. Get Total Lifetime (from LittleFS)
    unsigned long totalSeconds = 0;
    if (LittleFS.exists("/uptime.dat")) {
      File f = LittleFS.open("/uptime.dat", "r");
      if (f) {
        totalSeconds = f.readString().toInt();
        f.close();
      }
    }

    // 2. Calculate Session Uptime (Time since boot)
    unsigned long sessionSeconds = millis() / 1000;

    // 3. Build the combined JSON
    String json = "{";
    json += "\"hostname\":\"" + deviceHostname + "\",";
    json += "\"total_seconds\":" + String(totalSeconds) + ",";
    json += "\"total_formatted\":\"" + formatUptime(totalSeconds) + "\",";
    json += "\"session_seconds\":" + String(sessionSeconds) + ",";
    json += "\"session_formatted\":\"" + formatUptime(sessionSeconds) + "\",";
    json += "\"version\":\"" FIRMWARE_VERSION "\"";
    json += "}";

    request->send(200, "application/json", json);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    auto state = std::make_shared<StatusStreamState>();
    state->snsType = detectSnsType(String(ntpServer2));
    state->nowTime = time(nullptr);
    state->buf = statusSectionJson(0, state->snsType, state->nowTime);
    state->bufPos = 0;
    state->section = 1;

    AsyncWebServerResponse *response = request->beginChunkedResponse(
      "application/json",
      [state](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        while (state->bufPos >= state->buf.length()) {
          if (state->section > 15) {
            return 0;  // signals end of response
          }
          state->buf = statusSectionJson(state->section, state->snsType, state->nowTime);
          state->bufPos = 0;
          state->section++;
        }
        size_t remaining = state->buf.length() - state->bufPos;
        size_t toCopy = (remaining < maxLen) ? remaining : maxLen;
        memcpy(buffer, state->buf.c_str() + state->bufPos, toCopy);
        state->bufPos += toCopy;
        return toCopy;
      });

    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    response->addHeader("Connection", "close");
    request->send(response);
  });

  server.on("/export", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[WEBSERVER] Request: /export"));

    AsyncWebServerResponse *response = request->beginChunkedResponse(
      "application/json",
      [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        static File f;
        static String out;
        static bool sanitize = false;

        if (index == 0) {
          if (LittleFS.exists("/config.json")) {
            f = LittleFS.open("/config.json", "r");
            Serial.println(F("[EXPORT] Using /config.json"));
          } else if (LittleFS.exists("/config.bak")) {
            f = LittleFS.open("/config.bak", "r");
            Serial.println(F("[EXPORT] Using /config.bak"));
          } else {
            return 0;
          }
          out = String();
        }

        if (!f) return 0;

        while (out.length() < maxLen && f.available()) {
          char c = f.read();
          // Detect protected keys
          if (c == '"') {
            String possibleKey = "\"";
            while (f.available()) {
              char k = f.read();
              possibleKey += k;
              if (k == '"')
                break;
            }
            if (possibleKey == "\"ssid\"" || possibleKey == "\"password\"" || possibleKey == "\"openWeatherApiKey\"") {
              out += possibleKey;

              // Copy :"
              while (f.available()) {
                char x = f.read();
                out += x;
                if (x == '"')
                  break;
              }

              // Skip original value
              while (f.available()) {
                char x = f.read();
                if (x == '"')
                  break;
              }

              String cleanKey = possibleKey.substring(1, possibleKey.length() - 1);

              if (!isAPMode && (cleanKey == "ssid" || cleanKey == "password" || cleanKey == "openWeatherApiKey")) {
                if (cleanKey == "ssid")
                  out += "********";
                else if (cleanKey == "password")
                  out += "********";
                else
                  out += "********************************";
              } else if (cleanKey == "mode") {
                out += isAPMode ? "ap" : "sta";
              } else {
                out += "";
              }
              out += "\"";
              continue;
            }
            out += possibleKey;
            continue;
          }
          out += c;
        }

        if (out.length() == 0) {
          if (f) {
            f.close();
          }
          f = File();
          out = String();
          return 0;
        }
        size_t len = min(maxLen, out.length());
        memcpy(buffer, out.c_str(), len);
        out.remove(0, len);
        return len;
      });

    response->addHeader(
      "Content-Disposition",
      "attachment; filename=\"config.json\"");
    response->addHeader("Connection", "close");
    request->send(response);
  });

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
      <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1" />
          <style>
            html{
                background: linear-gradient(135deg, #081f56 0%, #110f2e 50%, #441a65 100%);
                height: 100%;
            }

            body {
                border: solid 1px rgba(255, 255, 255, 0.12);
                transition: opacity 0.6s cubic-bezier(.4, 0, .2, 1);
                max-width: 300px;
                margin: 4rem auto;
                background: rgba(255, 255, 255, 0.04);
                border-radius: 24px;
                text-align: center;
                font-family: Roboto, system-ui;
                /* margin: 0; */
                padding: 2rem 1rem;
                color: #ffffff;
                background-repeat: no-repeat, repeat, repeat;
                line-height: 1.5;
                -webkit-font-smoothing: antialiased;
                -moz-osx-font-smoothing: grayscale;
                box-shadow: 0 10px 36px 0 rgba(40, 170, 255, 0.11), 0 2px 8px 0 rgba(44, 70, 110, 0.08);
              }
            
            h3 {
              margin-top: 0;
              }

            input::file-selector-button {
              background: #0ea5e9;
              color: white;
              padding: 0.9rem 1.8rem;
              font-size: 1rem;
              font-weight: 600;
              border: none;
              border-radius: 999px;
              cursor: pointer;
              text-align: center;
              transition: background 0.25s, transform 0.15s 
              ease-in-out;
              margin-right: 0.5rem;
            }
          </style>
      </head>
      <body>
        <h3>Upload config.json</h3>
        <form method="POST" action="/upload" enctype="multipart/form-data">
          <input type="file" name="file" accept=".json" id="fileInput" onchange="this.form.submit()">
        </form>
      </body>
    </html>
  )rawliteral";
    request->send(200, "text/html", html);
  });

  server.on(
    "/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
      String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
        <head>
          <meta charset="UTF-8" />
          <meta name="viewport" content="width=device-width, initial-scale=1" />
          <title>Upload Successful</title>
          <meta http-equiv="refresh" content="1; url=/" />
          <style>
            html{
                background: linear-gradient(135deg, #081f56 0%, #110f2e 50%, #441a65 100%);
                height: 100%;
            }

            body {
                border: solid 1px rgba(255, 255, 255, 0.12);
                transition: opacity 0.6s cubic-bezier(.4, 0, .2, 1);
                max-width: 300px;
                margin: 4rem auto;
                background: rgba(255, 255, 255, 0.04);
                border-radius: 24px;
                text-align: center;
                font-family: Roboto, system-ui;
                /* margin: 0; */
                padding: 2rem 1rem;
                color: #ffffff;
                background-repeat: no-repeat, repeat, repeat;
                line-height: 1.5;
                -webkit-font-smoothing: antialiased;
                -moz-osx-font-smoothing: grayscale;
                box-shadow: 0 10px 36px 0 rgba(40, 170, 255, 0.11), 0 2px 8px 0 rgba(44, 70, 110, 0.08);
              }
            
            h3 {
              margin-top: 0;
              }

            input::file-selector-button {
              background: #0ea5e9;
              color: white;
              padding: 0.9rem 1.8rem;
              font-size: 1rem;
              font-weight: 600;
              border: none;
              border-radius: 999px;
              cursor: pointer;
              text-align: center;
              transition: background 0.25s, transform 0.15s 
              ease-in-out;
              margin-right: 0.5rem;
            }
          </style>
        </head>
        <body>
          <h3>File uploaded successfully!</h3>
          <p>Returning to main page...</p>
        </body>
      </html>
    )rawliteral";
      request->send(200, "text/html", html);
      // Restart after short delay to let browser handle redirect
      request->onDisconnect([]() {
        delay(500);  // ensure response is sent
        ESP.restart();
      });
    },
    [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
      static File f;
      if (index == 0) {
        f = LittleFS.open("/config.json", "w");  // start new file
      }
      if (f) f.write(data, len);  // write chunk
      if (final) f.close();       // finish file
    });

  server.on("/get_version", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"board\":\"" + String(BOARD_TYPE) + "\"";
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/perform_update", HTTP_GET, [](AsyncWebServerRequest *request) {
    isUpdating = true;

    // Immediate UI Feedback
    P.displayClear();
    P.setTextAlignment(PA_CENTER);
    delay(100);
    P.print((char)172);  // Show your download/update icon

    request->send(200, "application/json", "{\"status\":\"ready\"}");
  });

  server.on(
    "/upload_ota", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (!Update.hasError()) {
        request->send(200, "text/plain", "OK");
        // Set flags to reboot in the main loop
        pendingRestart = true;
        restartTimer = millis();
      } else {
        request->send(200, "text/plain", "FAIL");
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      // This runs for every chunk of data received
      if (!index) {
        Serial.printf(PSTR("[OTA] Start: %s\n"), filename.c_str());

#ifdef ESP8266
        Update.runAsync(true);  // Critical: Prevent __yield panic on ESP8266
#endif

        // Calculate max available space for the firmware
        uint32_t maxSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSpace, U_FLASH)) {
          Update.printError(Serial);
        }
      }

      if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
      }

      if (final) {
        if (Update.end(true)) {
          Serial.printf(PSTR("[OTA] Finished: %u bytes\n"), index + len);
          // CREATE THE "SUCCESS" FLAG
          File f = LittleFS.open("/update_success.txt", "w");
          if (f) {
            f.print("1");
            f.close();
          }
        } else {
          Update.printError(Serial);
        }
      }
    });

  server.on("/factory_reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    // If not in AP mode, block and return a 403 response
    if (!isAPMode) {
      request->send(403, "text/plain", "Factory reset only allowed in AP mode.");
      Serial.println(F("[RESET] Factory reset attempt blocked (not in AP mode)."));
      return;
    }
    const char *FACTORY_RESET_HTML = R"rawliteral(
      <!DOCTYPE html>
      <html>
        <head>
          <meta charset="UTF-8" />
          <meta name="viewport" content="width=device-width, initial-scale=1" />
          <title>Resetting Device</title>
          <style>
            html{
                background: linear-gradient(135deg, #081f56 0%, #110f2e 50%, #441a65 100%);
                height: 100%;
            }

            body {
                border: solid 1px rgba(255, 255, 255, 0.12);
                transition: opacity 0.6s cubic-bezier(.4, 0, .2, 1);
                max-width: 300px;
                margin: 4rem auto;
                background: rgba(255, 255, 255, 0.04);
                border-radius: 24px;
                text-align: center;
                font-family: Roboto, system-ui;
                /* margin: 0; */
                padding: 2rem 1rem;
                color: #ffffff;
                background-repeat: no-repeat, repeat, repeat;
                line-height: 1.5;
                -webkit-font-smoothing: antialiased;
                -moz-osx-font-smoothing: grayscale;
                box-shadow: 0 10px 36px 0 rgba(40, 170, 255, 0.11), 0 2px 8px 0 rgba(44, 70, 110, 0.08);
              }
            h3 { margin-top: 0; color: #ff9999; }
            p { font-size: 1.1em; }
            .warning { font-size: 1.2em; font-weight: bold; color: #fff; margin-top: 15px; }
          </style>
        </head>
        <body>
          <h3>Factory Reset Initiated</h3>
          <p>All saved configuration and Wi-Fi credentials are now being erased.</p>
          <hr style="margin: 15px 0; border: 0; border-top: 1px solid rgba(255,255,255,0.2);">
          <p class="warning"><span style="color: yellow;">⚠️</span> ACTION REQUIRED</p>
          <p>
            The device is rebooting and will be temporarily offline for about <strong>45 seconds</strong>.
            <br><br>
            <strong>Your browser will disconnect automatically.</strong>
          </p>
          <p>
            <strong>Next steps:</strong>
            <br>1. Wait about 45 seconds for the reboot to finish.<br>
            2. Reconnect your PC or phone to the Wi-Fi network: <strong>ESPTimeCast</strong>.<br>
            3. Open your browser and go to <strong>192.168.4.1</strong> to continue setup.
          </p>
        </body>
      </html>
    )rawliteral";
    request->send(200, "text/html", FACTORY_RESET_HTML);
    Serial.println(F("[RESET] Factory reset requested, initiating cleanup..."));

    // Use onDisconnect() to ensure the HTTP response is fully sent before the disruptive actions
    request->onDisconnect([]() {
      // Small delay to ensure the response buffer is flushed before file ops
      delay(500);

      // --- Remove configuration and uptime files ---
      const char *filesToRemove[] = { "/config.json", "/uptime.dat", "/index.html" };
      for (auto &file : filesToRemove) {
        if (LittleFS.exists(file)) {
          if (LittleFS.remove(file)) {
            Serial.printf(PSTR("[RESET] Deleted %s\n"), file);
          } else {
            Serial.printf(PSTR("[RESET] ERROR deleting %s\n"), file);
          }
        } else {
          Serial.printf(PSTR("[RESET] %s not found, skipping delete.\n"), file);
        }
      }

// --- Clear Wi-Fi credentials ---
#if defined(ESP8266)
      WiFi.disconnect(true);  // true = wipe credentials
#elif defined(ESP32)
        WiFi.disconnect(true, true);  // (erase=true, wifioff=true)
#endif

      Serial.println(F("[RESET] Factory defaults restored. Rebooting..."));
      delay(500);
      ESP.restart();
    });
  });

  server.begin();
  Serial.println(F("[WEBSERVER] Web server started"));
}

void handleCaptivePortal(AsyncWebServerRequest *request) {
  String uri = request->url();

  // Never interfere with real UI or API
  if (
    uri == "/" || uri == "/index.html" || uri.startsWith("/config") || uri.startsWith("/hostname") || uri.startsWith("/ip") || uri.endsWith(".json") || uri.endsWith(".js") || uri.endsWith(".css") || uri.endsWith(".png") || uri.endsWith(".ico")) {
    return;  // let normal handlers serve it
  }

  // Known captive portal probes → redirect
  if (
    uri == "/generate_204" || uri == "/gen_204" || uri == "/fwlink" || uri == "/hotspot-detect.html" || uri == "/ncsi.txt" || uri == "/cp/success.txt" || uri == "/library/test/success.html") {
    if (isAPMode) {
      IPAddress apIP = WiFi.softAPIP();
      String redirectUrl = "http://" + apIP.toString() + "/";
      AsyncWebServerResponse *response = request->beginResponse(302);
      response->addHeader("Location", redirectUrl);
      response->addHeader("Connection", "close");
      request->send(response);
      return;
    }
  }

  // Unknown URLs in AP mode → redirect (helps odd OSes like /chat)
  if (isAPMode) {
    IPAddress apIP = WiFi.softAPIP();
    String redirectUrl = "http://" + apIP.toString() + "/";
    Serial.printf(PSTR("[WEBSERVER] Captive fallback redirect: %s\n"), uri.c_str());
    AsyncWebServerResponse *response = request->beginResponse(302);
    response->addHeader("Location", redirectUrl);
    response->addHeader("Connection", "close");
    request->send(response);
    return;
  }

  // STA mode fallback
  request->send(404, "text/plain", "Not found");
}

String cleanTextForDisplay(String str) {
  // Serbian Cyrillic → Latin
  str.replace("а", "a");
  str.replace("б", "b");
  str.replace("в", "v");
  str.replace("г", "g");
  str.replace("д", "d");
  str.replace("ђ", "dj");
  str.replace("е", "e");
  str.replace("ё", "e");  // Russian
  str.replace("ж", "z");
  str.replace("з", "z");
  str.replace("и", "i");
  str.replace("й", "j");  // Russian
  str.replace("ј", "j");  // Serbian
  str.replace("к", "k");
  str.replace("л", "l");
  str.replace("љ", "lj");
  str.replace("м", "m");
  str.replace("н", "n");
  str.replace("њ", "nj");
  str.replace("о", "o");
  str.replace("п", "p");
  str.replace("р", "r");
  str.replace("с", "s");
  str.replace("т", "t");
  str.replace("ћ", "c");
  str.replace("у", "u");
  str.replace("ф", "f");
  str.replace("х", "h");
  str.replace("ц", "c");
  str.replace("ч", "c");
  str.replace("џ", "dz");
  str.replace("ш", "s");
  str.replace("щ", "sh");  // Russian
  str.replace("ы", "y");   // Russian
  str.replace("э", "e");   // Russian
  str.replace("ю", "yu");  // Russian
  str.replace("я", "ya");  // Russian

  // Latin diacritics → ASCII
  str.replace("å", "a");
  str.replace("ä", "a");
  str.replace("à", "a");
  str.replace("á", "a");
  str.replace("â", "a");
  str.replace("ã", "a");
  str.replace("ā", "a");
  str.replace("ă", "a");
  str.replace("ą", "a");

  str.replace("æ", "ae");

  str.replace("ç", "c");
  str.replace("č", "c");
  str.replace("ć", "c");

  str.replace("ď", "d");

  str.replace("é", "e");
  str.replace("è", "e");
  str.replace("ê", "e");
  str.replace("ë", "e");
  str.replace("ē", "e");
  str.replace("ė", "e");
  str.replace("ę", "e");

  str.replace("ğ", "g");
  str.replace("ģ", "g");

  str.replace("ĥ", "h");

  str.replace("í", "i");
  str.replace("ì", "i");
  str.replace("î", "i");
  str.replace("ï", "i");
  str.replace("ī", "i");
  str.replace("į", "i");

  str.replace("ĵ", "j");

  str.replace("ķ", "k");

  str.replace("ľ", "l");
  str.replace("ł", "l");

  str.replace("ñ", "n");
  str.replace("ń", "n");
  str.replace("ņ", "n");

  str.replace("ó", "o");
  str.replace("ò", "o");
  str.replace("ô", "o");
  str.replace("ö", "o");
  str.replace("õ", "o");
  str.replace("ø", "o");
  str.replace("ō", "o");
  str.replace("ő", "o");

  str.replace("œ", "oe");

  str.replace("ŕ", "r");

  str.replace("ś", "s");
  str.replace("š", "s");
  str.replace("ș", "s");
  str.replace("ŝ", "s");

  str.replace("ß", "ss");

  str.replace("ť", "t");
  str.replace("ț", "t");

  str.replace("ú", "u");
  str.replace("ù", "u");
  str.replace("û", "u");
  str.replace("ü", "u");
  str.replace("ū", "u");
  str.replace("ů", "u");
  str.replace("ű", "u");

  str.replace("ŵ", "w");

  str.replace("ý", "y");
  str.replace("ÿ", "y");
  str.replace("ŷ", "y");

  str.replace("ž", "z");
  str.replace("ź", "z");
  str.replace("ż", "z");

  str.toUpperCase();

  String result = "";
  for (unsigned int i = 0; i < str.length(); i++) {
    unsigned char c = (unsigned char)str.charAt(i);  // Use unsigned for safety

    // MASTER FILTER: Expanded for Modern Smart Home Notifications
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || strchr(" !.?:,;'\"-_+%/[]()#&¥$ ;|°@^~*=<>\t\n\r\\{}", c)) {
      result += (char)c;
    }
  }
  return result;  // Return the cleaned string
}

bool isNumber(const char *str) {
  for (int i = 0; str[i]; i++) {
    if (!isdigit(str[i]) && str[i] != '.' && str[i] != '-') return false;
  }
  return true;
}

bool isFiveDigitZip(const char *str) {
  if (strlen(str) != 5) return false;
  for (int i = 0; i < 5; i++) {
    if (!isdigit(str[i])) return false;
  }
  return true;
}


// -----------------------------------------------------------------------------
// Weather Fetching and API settings
// -----------------------------------------------------------------------------
String buildWeatherURL() {
#if defined(ESP8266) || defined(CONFIG_IDF_TARGET_ESP32S2)
  String base = "http://api.openweathermap.org/data/2.5/weather?";
#else
  String base = "https://api.openweathermap.org/data/2.5/weather?";
#endif

  float lat = atof(openWeatherCity);
  float lon = atof(openWeatherCountry);

  bool latValid = isNumber(openWeatherCity) && isNumber(openWeatherCountry) && lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;

  // Create encoded copies
  String cityEncoded = String(openWeatherCity);
  String countryEncoded = String(openWeatherCountry);
  cityEncoded.replace(" ", "%20");
  countryEncoded.replace(" ", "%20");

  if (latValid) {
    base += "lat=" + String(lat, 8) + "&lon=" + String(lon, 8);
  } else if (isFiveDigitZip(openWeatherCity) && String(openWeatherCountry).equalsIgnoreCase("US")) {
    base += "zip=" + String(openWeatherCity) + "," + String(openWeatherCountry);
  } else {
    base += "q=" + cityEncoded + "," + countryEncoded;
  }

  base += "&appid=" + String(openWeatherApiKey);
  base += "&units=" + String(weatherUnits);

  String langForAPI = String(language);
  if (langForAPI == "eo" || langForAPI == "ga" || langForAPI == "sw" || langForAPI == "ja") {
    langForAPI = "en";
  }
  base += "&lang=" + langForAPI;

  return base;
}


void fetchWeather() {
  if (millis() - lastWifiConnectTime < 5000) {
    Serial.println(F("[WEATHER] Skipped: Network just reconnected. Letting it stabilize..."));
    return;  // Stop execution if connection is less than 5 seconds old
  }

  Serial.println(F("[WEATHER] Fetching weather data..."));
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[WEATHER] Skipped: WiFi not connected"));
    weatherAvailable = false;
    weatherFetched = false;
    return;
  }
  if (!openWeatherApiKey || strlen(openWeatherApiKey) != 32) {
    Serial.println(F("[WEATHER] Skipped: Invalid API key (must be exactly 32 characters)"));
    weatherAvailable = false;
    weatherFetched = false;
    return;
  }
  if (!(strlen(openWeatherCity) > 0 && strlen(openWeatherCountry) > 0)) {
    Serial.println(F("[WEATHER] Skipped: City or Country is empty."));
    weatherAvailable = false;
    return;
  }

  Serial.println(F("[WEATHER] Connecting to OpenWeatherMap..."));
  String url = buildWeatherURL();
  Serial.print(F("[WEATHER] URL: "));  // Use F() with Serial.print
  Serial.println(url);

  HTTPClient http;  // Create an HTTPClient object

#if defined(ESP8266) || defined(CONFIG_IDF_TARGET_ESP32S2)
  // ===== ESP8266 → HTTP =====
  WiFiClient client;
  client.stop();
  yield();

  http.begin(client, url);
#else
  // ===== ESP32 → HTTPS =====
  WiFiClientSecure client;
  client.stop();
  client.setInsecure();  // no cert validation
  yield();

  http.begin(client, url);
#endif

  http.setTimeout(10000);  // Sets both connection and stream timeout to 10 seconds

  Serial.println(F("[WEATHER] Sending GET request..."));
  int httpCode = http.GET();  // Send the GET request

  if (httpCode == HTTP_CODE_OK) {  // Check if HTTP response code is 200 (OK)
    Serial.println(F("[WEATHER] HTTP 200 OK. Reading payload..."));

    String payload = http.getString();
    Serial.println(F("[WEATHER] Response received."));
    Serial.println(F("[WEATHER] Payload: ") + payload);

    JsonDocument doc;  // Adjust size as needed, use ArduinoJson Assistant
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("[WEATHER] JSON parse error: "));
      Serial.println(error.f_str());
      weatherAvailable = false;
      return;
    }

    if (doc.containsKey(F("main")) && doc[F("main")].containsKey(F("temp"))) {
      float temp = doc[F("main")][F("temp")];
      currentTemp = String((int)round(temp)) + char(176);
      Serial.printf(PSTR("[WEATHER] Temp: %d°\n"), (int)round(temp));
      weatherAvailable = true;
    } else {
      Serial.println(F("[WEATHER] Temperature not found in JSON payload"));
      weatherAvailable = false;
      return;
    }

    if (doc.containsKey(F("main")) && doc[F("main")].containsKey(F("humidity"))) {
      currentHumidity = doc[F("main")][F("humidity")];
      Serial.printf(PSTR("[WEATHER] Humidity: %d%%\n"), currentHumidity);
    } else {
      currentHumidity = -1;
    }

    if (doc.containsKey(F("weather")) && doc[F("weather")].is<JsonArray>()) {
      JsonObject weatherObj = doc[F("weather")][0];
      if (weatherObj.containsKey(F("main"))) {
        mainDesc = weatherObj[F("main")].as<String>();
      }
      if (weatherObj.containsKey(F("description"))) {
        detailedDesc = weatherObj[F("description")].as<String>();
      }
      if (weatherObj.containsKey(F("icon"))) {
        weatherIcon = getWeatherIconChar(weatherObj[F("icon")].as<String>());
      }
    } else {
      Serial.println(F("[WEATHER] Weather description not found in JSON payload"));
    }
    weatherDescription = String(weatherIcon) + " " + cleanTextForDisplay(detailedDesc);
    Serial.printf(PSTR("[WEATHER] Description used: %s\n"), weatherDescription.c_str());

    // -----------------------------------------
    // Sunrise/Sunset for Auto Dimming (local time)
    // -----------------------------------------
    if (doc.containsKey(F("sys"))) {
      JsonObject sys = doc[F("sys")];
      if (sys.containsKey(F("sunrise")) && sys.containsKey(F("sunset"))) {
        // OWM gives UTC timestamps
        time_t sunriseUtc = sys[F("sunrise")].as<time_t>();
        time_t sunsetUtc = sys[F("sunset")].as<time_t>();

        // Get local timezone offset (in seconds)
        long tzOffset = 0;
        struct tm local_tm;
        time_t now = time(nullptr);
        if (localtime_r(&now, &local_tm)) {
          tzOffset = mktime(&local_tm) - now;
        }

        // Convert UTC → local
        time_t sunriseLocal = sunriseUtc + tzOffset;
        time_t sunsetLocal = sunsetUtc + tzOffset;

        // Break into hour/minute
        struct tm tmSunrise, tmSunset;
        localtime_r(&sunriseLocal, &tmSunrise);
        localtime_r(&sunsetLocal, &tmSunset);

        sunriseHour = tmSunrise.tm_hour;
        sunriseMinute = tmSunrise.tm_min;
        sunsetHour = tmSunset.tm_hour;
        sunsetMinute = tmSunset.tm_min;

        Serial.printf(PSTR("[WEATHER] Adjusted Sunrise/Sunset (local): %02d:%02d | %02d:%02d\n"),
                      sunriseHour, sunriseMinute, sunsetHour, sunsetMinute);
      } else {
        Serial.println(F("[WEATHER] Sunrise/Sunset not found in JSON."));
      }
    } else {
      Serial.println(F("[WEATHER] 'sys' object not found in JSON payload."));
    }

    weatherFetched = true;

    // -----------------------------------------
    // Save updated sunrise/sunset to config.json
    // -----------------------------------------
    if (autoDimmingEnabled && sunriseHour >= 0 && sunsetHour >= 0) {
      File configFile = LittleFS.open("/config.json", "r");
      JsonDocument doc;

      if (configFile) {
        DeserializationError error = deserializeJson(doc, configFile);
        configFile.close();

        if (!error) {
          // Check if ANY value has changed
          bool valuesChanged =
            (doc["sunriseHour"].as<int>() != sunriseHour || doc["sunriseMinute"].as<int>() != sunriseMinute || doc["sunsetHour"].as<int>() != sunsetHour || doc["sunsetMinute"].as<int>() != sunsetMinute);

          if (valuesChanged) {  // Only write if a change occurred
            doc["sunriseHour"] = sunriseHour;
            doc["sunriseMinute"] = sunriseMinute;
            doc["sunsetHour"] = sunsetHour;
            doc["sunsetMinute"] = sunsetMinute;

            File f = LittleFS.open("/config.json", "w");
            if (f) {
              serializeJson(doc, f);
              f.close();
              Serial.println(F("[WEATHER] SAVED NEW sunrise/sunset to config.json (Values changed)"));
            } else {
              Serial.println(F("[WEATHER] Failed to write updated sunrise/sunset to config.json"));
            }
          } else {
            Serial.println(F("[WEATHER] Sunrise/Sunset unchanged, skipping config save."));
          }
          // --- END MODIFIED COMPARISON LOGIC ---

        } else {
          Serial.println(F("[WEATHER] JSON parse error when saving updated sunrise/sunset"));
        }
      }
    }

  } else {
    Serial.printf(PSTR("[WEATHER] HTTP GET failed, error code: %d, reason: %s\n"),
                  httpCode, http.errorToString(httpCode).c_str());
    weatherAvailable = false;
    weatherFetched = false;
  }

  http.end();
}

void fetchNightscout() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (ESP.getFreeHeap() < 10000 || isNetworkBusy) return;

#ifdef ESP8266
  // --- URL encode helper ---
  auto urlEncode = [](String str) -> String {
    String encoded = "";
    for (int i = 0; i < str.length(); i++) {
      char c = str.charAt(i);
      if (isalnum(c)) {
        encoded += c;
      } else {
        char code1 = (c & 0xf) + '0';
        if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
        char c2 = (c >> 4) & 0xf;
        char code0 = c2 + '0';
        if (c2 > 9) code0 = c2 - 10 + 'A';
        encoded += '%';
        encoded += code0;
        encoded += code1;
      }
    }
    return encoded;
  };

  // --- Detect and strip mmol=1 and show_every=N ---
  String rawUrl = String(ntpServer2);
  nightscoutMmol = (rawUrl.indexOf("mmol=1") != -1);
  if (nightscoutMmol) rawUrl = stripUrlParam(rawUrl, "mmol");
  rawUrl = stripUrlParam(rawUrl, "show_every");

  String bridgeUrl = "http://esptimecast.com/nightscout-bridge.php?url=";
  bridgeUrl += urlEncode(rawUrl);

  isNetworkBusy = true;
  Serial.println(F("[NIGHTSCOUT] Fetching via PHP bridge"));

  WiFiClient client;
  HTTPClient http;
  http.addHeader("User-Agent", "Mozilla/5.0");
  if (http.begin(client, bridgeUrl)) {
    http.setTimeout(4000);
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      payload.trim();
      Serial.printf(PSTR("[NIGHTSCOUT] Payload: %s\n"), payload.c_str());

      int space1 = payload.indexOf(' ');
      int space2 = payload.lastIndexOf(' ');
      if (space1 != -1 && space2 != -1 && space1 != space2) {
        int parsedGlucose = payload.substring(0, space1).toInt();
        if (parsedGlucose > 0) {
          currentGlucose = parsedGlucose;
          currentDirection = payload.substring(space1 + 1, space2);
          long long date = (long long)payload.substring(space2 + 1).toFloat();
          if (date > 0) lastGlucoseTime = date;  // already in seconds from PHP
          Serial.printf(PSTR("[NIGHTSCOUT] Fetched: %d (%s) %s\n"),
                        currentGlucose,
                        nightscoutMmol ? "will display as mmol" : "mg/dL",
                        currentDirection.c_str());
          lastNightscoutFetchTime = millis();
          nightscoutFailCount = 0;
        } else {
          Serial.println(F("[NIGHTSCOUT] Bridge returned error response, will retry"));
          lastNightscoutFetchTime = millis() - NIGHTSCOUT_FETCH_INTERVAL + 60000;
        }
      } else {
        Serial.println(F("[NIGHTSCOUT] Failed to parse payload"));
        lastNightscoutFetchTime = millis() - NIGHTSCOUT_FETCH_INTERVAL + 60000;
      }
    } else if (httpCode == 404) {
      Serial.println(F("[NIGHTSCOUT] Bridge: invalid URL (404). Pausing fetch for 24h."));
      nightscoutBackoffUntil = millis() + 86400000UL;
    } else if (httpCode == 429) {
      Serial.println(F("[NIGHTSCOUT] Bridge: rate limited (429). Backing off 30 min."));
      nightscoutBackoffUntil = millis() + 1800000UL;
    } else {
      nightscoutFailCount++;
      Serial.printf(PSTR("[NIGHTSCOUT] HTTP failed (%d): %s\n"), nightscoutFailCount, http.errorToString(httpCode).c_str());
      if (nightscoutFailCount >= 3) {
        unsigned long backoff = (nightscoutFailCount >= 6) ? 1800000UL : 300000UL;
        Serial.printf(PSTR("[NIGHTSCOUT] %d consecutive failures, backing off %lu min.\n"),
                      nightscoutFailCount, backoff / 60000UL);
        nightscoutBackoffUntil = millis() + backoff;
      }
    }
    http.end();
  }

#else
  // --- ESP32: direct HTTPS + JSON ---
  String rawUrl = String(ntpServer2);
  nightscoutMmol = (rawUrl.indexOf("mmol=1") != -1);
  if (nightscoutMmol) rawUrl = stripUrlParam(rawUrl, "mmol");
  rawUrl = stripUrlParam(rawUrl, "show_every");

  if (rawUrl.indexOf("count=") == -1)
    rawUrl += (rawUrl.indexOf('?') == -1) ? "?count=1" : "&count=1";

  isNetworkBusy = true;
  Serial.printf(PSTR("[NIGHTSCOUT] Fetching%s direct\n"), nightscoutMmol ? " (mmol)" : " (mg/dL)");

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  https.begin(client, rawUrl);
  https.setTimeout(8000);
  int httpCode = https.GET();
  if (httpCode == HTTP_CODE_OK) {
    WiFiClient *stream = https.getStreamPtr();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, *stream);
    if (!error && doc.is<JsonArray>() && doc.size() > 0) {
      JsonObject firstReading = doc[0].as<JsonObject>();
      currentGlucose = firstReading["sgv"] | firstReading["glucose"] | -1;
      currentDirection = firstReading["direction"] | "?";
      long long dateMs = firstReading["date"] | 0LL;
      if (dateMs > 0) lastGlucoseTime = dateMs / 1000;  // JSON returns ms, convert to seconds
      Serial.printf(PSTR("[NIGHTSCOUT] Fetched: %d (%s) %s\n"),
                    currentGlucose,
                    nightscoutMmol ? "will display as mmol" : "mg/dL",
                    currentDirection.c_str());
      lastNightscoutFetchTime = millis();
      nightscoutFailCount = 0;
    } else {
      Serial.println(F("[NIGHTSCOUT] Failed to parse JSON"));
      lastNightscoutFetchTime = millis() - NIGHTSCOUT_FETCH_INTERVAL + 60000;
    }
  } else if (httpCode == 404) {
    Serial.println(F("[NIGHTSCOUT] Bridge: invalid URL (404). Pausing fetch for 24h."));
    nightscoutBackoffUntil = millis() + 86400000UL;
    nightscoutFailCount = 0;
  } else if (httpCode == 429) {
    Serial.println(F("[NIGHTSCOUT] Bridge: rate limited (429). Backing off 30 min."));
    nightscoutBackoffUntil = millis() + 1800000UL;
    nightscoutFailCount = 0;
  } else {
    nightscoutFailCount++;
    Serial.printf(PSTR("[NIGHTSCOUT] HTTPS failed (%d): %s\n"), nightscoutFailCount, https.errorToString(httpCode).c_str());
    if (nightscoutFailCount >= 3) {
      unsigned long backoff = (nightscoutFailCount >= 6) ? 1800000UL : 300000UL;
      Serial.printf(PSTR("[NIGHTSCOUT] %d consecutive failures, backing off %lu min.\n"),
                    nightscoutFailCount, backoff / 60000UL);
      nightscoutBackoffUntil = millis() + backoff;
    }
  }
  https.end();
  client.stop();
#endif

  delay(100);
  isNetworkBusy = false;
}


// -----------------------------
// Load uptime from LittleFS
// -----------------------------
void loadUptime() {
  if (LittleFS.exists("/uptime.dat")) {
    File f = LittleFS.open("/uptime.dat", "r");
    if (f) {
      totalUptimeSeconds = f.parseInt();
      f.close();
      bootMillis = millis();
      Serial.printf(PSTR("[UPTIME] Loaded accumulated uptime: %lu seconds (%.2f hours)\n"),
                    totalUptimeSeconds, totalUptimeSeconds / 3600.0);
    } else {
      Serial.println(F("[UPTIME] Failed to open /uptime.dat for reading."));
      totalUptimeSeconds = 0;
      bootMillis = millis();
    }
  } else {
    Serial.println(F("[UPTIME] No previous uptime file found. Starting from 0."));
    totalUptimeSeconds = 0;
    bootMillis = millis();
  }
}

// -----------------------------
// Save uptime to LittleFS
// -----------------------------
void saveUptime() {
  // Use getTotalRuntimeSeconds() to include current session
  totalUptimeSeconds = getTotalRuntimeSeconds();
  bootMillis = millis();  // reset session start

  File f = LittleFS.open("/uptime.dat", "w");
  if (f) {
    f.print(totalUptimeSeconds);
    f.close();
    Serial.printf(PSTR("[UPTIME] Saved accumulated uptime: %s\n"), formatTotalRuntime().c_str());
  } else {
    Serial.println(F("[UPTIME] Failed to write /uptime.dat"));
  }
}

void setAlarmSchedule(int index, int hour, int minute, const bool days[7]) {
  if (index < 0 || index >= MAX_ALARMS) return;
  alarmConfigs[index].hour = hour;
  alarmConfigs[index].minute = minute;
  for (int i = 0; i < 7; i++) alarmConfigs[index].days[i] = days[i];
  alarmConfigs[index].enabled = true;
  configDirty = true;
  lastBrightnessChange = millis();
}

void setAlarmSchedule(int hour, int minute, const bool days[7]) {
  setAlarmSchedule(0, hour, minute, days);
}

// -----------------------------
// Get total uptime including current session
// -----------------------------
unsigned long getTotalRuntimeSeconds() {
  return totalUptimeSeconds + (millis() - bootMillis) / 1000;
}


// -----------------------------
// Format total uptime as HH:MM:SS
// -----------------------------
String formatTotalRuntime() {
  unsigned long secs = getTotalRuntimeSeconds();
  unsigned int h = secs / 3600;
  unsigned int m = (secs % 3600) / 60;
  unsigned int s = secs % 60;
  char buf[16];
  sprintf(buf, "%02u:%02u:%02u", h, m, s);
  return String(buf);
}


void saveCustomMessageToConfig(const char *msg) {
  Serial.println(F("[CONFIG] Updating customMessage in config.json..."));

  JsonDocument doc;

  // Load existing config.json (if present)
  File configFile = LittleFS.open("/config.json", "r");
  if (configFile) {
    DeserializationError err = deserializeJson(doc, configFile);
    configFile.close();
    if (err) {
      Serial.print(F("[CONFIG] Error reading existing config: "));
      Serial.println(err.f_str());
    }
  }

  // Update only customMessage
  doc["customMessage"] = msg;

  // Safely write back to config.json
  if (LittleFS.exists("/config.json")) {
    LittleFS.rename("/config.json", "/config.bak");
  }

  File f = LittleFS.open("/config.json", "w");
  if (!f) {
    Serial.println(F("[CONFIG] ERROR: Failed to open /config.json for writing"));
    return;
  }

  size_t bytesWritten = serializeJson(doc, f);
  f.close();
  Serial.printf(PSTR("[CONFIG] Saved customMessage='%s' (%u bytes written)\n"), msg, bytesWritten);
}

// Returns formatted uptime (for web UI or logs)
String formatUptime(unsigned long seconds) {
  unsigned long days = seconds / 86400;
  unsigned long hours = (seconds % 86400) / 3600;
  unsigned long minutes = (seconds % 3600) / 60;
  unsigned long secs = seconds % 60;

  char buf[64];
  if (days > 0)
    sprintf(buf, "%lud %02lu:%02lu:%02lu", days, hours, minutes, secs);
  else
    sprintf(buf, "%02lu:%02lu:%02lu", hours, minutes, secs);
  return String(buf);
}

// Weather Icon Mapping
char getWeatherIconChar(const String &iconCode) {

  if (iconCode.startsWith("01")) {                    // clear sky
    return iconCode.endsWith("n") ? '\xA8' : '\x0C';  // Moon : Sun
  }

  if (iconCode.startsWith("02")) return '\x0D';  // few clouds
  if (iconCode.startsWith("03")) return '\x0D';  // scattered clouds
  if (iconCode.startsWith("04")) return '\x0D';  // broken clouds

  if (iconCode.startsWith("09")) return '\x10';  // shower rain
  if (iconCode.startsWith("10")) return '\x10';  // rain

  if (iconCode.startsWith("11")) return '\x11';  // thunderstorm
  if (iconCode.startsWith("13")) return '\x12';  // snow
  if (iconCode.startsWith("50")) return '\xB9';  // mist

  return '\x0D';  // fallback = cloud
}

// -----------------------------------------------------------------------------
// Pomodoro Command Handler
// Syntax: [POMODORO W-B]  where W = work minutes (1-60), B = break minutes (1-60)
//         [POMODORO STOP]    — stop and return to clock
//         [POMODORO RESTART] — restart from the beginning of the work phase
// -----------------------------------------------------------------------------
bool handlePomodoroCommand(String cmd) {
  cmd.toUpperCase();
  if (cmd.indexOf("[POMODORO") == -1 && cmd.indexOf("[POM]") == -1 && cmd.indexOf("[POM ") == -1) return false;
  // Prevent starting or restarting Pomodoro while Clock-only Dimming is active
  if (clockOnlyDuringDimming && dimActive) {
    bool isStop = cmd.indexOf("[POM STOP]") != -1 || cmd.indexOf("[POMODORO STOP]") != -1;
    bool isPause = cmd.indexOf("[POM PAUSE]") != -1 || cmd.indexOf("[POMODORO PAUSE]") != -1;
    bool isResume = cmd.indexOf("[POM RESUME]") != -1 || cmd.indexOf("[POMODORO RESUME]") != -1;
    if (!isStop && !isPause && !isResume) {
      Serial.println(F("[POMODORO] Ignored: Clock-only Dimming is active."));
      return true;
    }
  }
  // Prevent starting/restarting Pomodoro while a protected message is active
  if (!allowInterrupt) {
    bool isStop = cmd.indexOf("[POM STOP]") != -1 || cmd.indexOf("[POMODORO STOP]") != -1;
    bool isPause = cmd.indexOf("[POM PAUSE]") != -1 || cmd.indexOf("[POMODORO PAUSE]") != -1;
    bool isResume = cmd.indexOf("[POM RESUME]") != -1 || cmd.indexOf("[POMODORO RESUME]") != -1;
    if (!isStop && !isPause && !isResume) {
      Serial.println(F("[POMODORO] Ignored: Protected message is active."));
      return true;
    }
  }

  // --- STOP ---
  if (cmd.indexOf("[POM STOP]") != -1 || cmd.indexOf("[POMODORO STOP]") != -1) {
    isPomodoroActive = false;
    pomodoroInBreak = false;
    timerActive = false;
    timerFinished = false;
    timerPaused = false;
    isStopwatch = false;
    displayMode = 0;
    prevDisplayMode = 6;
    clockScrollDone = false;
    forceMessageRestart = true;
    lastSwitch = millis();
    Serial.println(F("[POMODORO] Stopped. Returning to Clock."));
    return true;
  }

  // --- RESTART — restart from the beginning of the work phase ---
  if (cmd.indexOf("[POM RESTART]") != -1 || cmd.indexOf("[POMODORO RESTART]") != -1) {
    if (isPomodoroActive && pomodoroWorkMs > 0) {
      pomodoroInBreak = false;
      isStopwatch = false;
      timerOriginalDuration = pomodoroWorkMs;
      timerEndTime = millis() + pomodoroWorkMs;
      timerActive = true;
      timerPaused = false;
      timerFinished = false;
      timerSubState = 0;
      displayMode = 7;
      allowInterrupt = true;
      lastSwitch = millis();
      forceMessageRestart = true;
      Serial.println(F("[POMODORO] Restarted — beginning work phase."));
      return true;
    }
    return false;
  }

  // --- PAUSE ---
  if (cmd.indexOf("[POM PAUSE]") != -1 || cmd.indexOf("[POMODORO PAUSE]") != -1) {
    Serial.println(F("[POMODORO] Pause forwarded to timer."));
    return handleTimerCommand("[TIMER PAUSE]");
  }

  // --- RESUME ---
  if (cmd.indexOf("[POM RESUME]") != -1 || cmd.indexOf("[POMODORO RESUME]") != -1) {
    Serial.println(F("[POMODORO] Resume forwarded to timer."));
    return handleTimerCommand("[TIMER RESUME]");
  }

  // Shortcuts: [POM] or bare [POMODORO] (no params) default to 25-5-15
  String trimmed = cmd;
  trimmed.trim();
  if (cmd.indexOf("[POM]") != -1 || (cmd.indexOf("[POMODORO]") != -1 && cmd.indexOf("[POMODORO ") == -1)) {
    pomodoroWorkMs = 25 * 60000UL;
    pomodoroBreakMs = 5 * 60000UL;
    pomodoroLongBreakMs = 15 * 60000UL;
    pomodoroSession = 1;
    isPomodoroActive = true;
    pomodoroInBreak = false;
    isStopwatch = false;
    timerOriginalDuration = pomodoroWorkMs;
    timerEndTime = millis() + pomodoroWorkMs;
    timerActive = true;
    timerPaused = false;
    timerFinished = false;
    timerSubState = 0;
    displayMode = 7;
    allowInterrupt = true;
    lastSwitch = millis();
    forceMessageRestart = true;
    Serial.println(F("[POMODORO] Default 25-5-15 started."));
    return true;
  }

  // --- [POMODORO W-B] — parse work and break minutes ---
  int pomStart = cmd.indexOf("[POMODORO ") + 10;
  int pomEnd = cmd.indexOf("]", pomStart);
  if (pomEnd == -1) return false;
  String params = cmd.substring(pomStart, pomEnd);
  params.trim();

  int dash1 = params.indexOf('-');
  if (dash1 == -1) return false;
  int dash2 = params.indexOf('-', dash1 + 1);

  int workMin = params.substring(0, dash1).toInt();
  int breakMin = params.substring(dash1 + 1, dash2 == -1 ? params.length() : dash2).toInt();
  int longMin = (dash2 != -1) ? params.substring(dash2 + 1).toInt() : 15;

  if (workMin < 1) workMin = 1;
  if (workMin > 60) workMin = 60;
  if (breakMin < 1) breakMin = 1;
  if (breakMin > 60) breakMin = 60;
  if (longMin < 1) longMin = 1;
  if (longMin > 60) longMin = 60;

  pomodoroWorkMs = (unsigned long)workMin * 60000UL;
  pomodoroBreakMs = (unsigned long)breakMin * 60000UL;
  pomodoroLongBreakMs = (unsigned long)longMin * 60000UL;
  pomodoroSession = 1;
  isPomodoroActive = true;
  pomodoroInBreak = false;

  // Start the work-phase countdown
  isStopwatch = false;
  timerOriginalDuration = pomodoroWorkMs;
  timerEndTime = millis() + pomodoroWorkMs;
  timerActive = true;
  timerPaused = false;
  timerFinished = false;
  timerSubState = 0;
  displayMode = 7;
  allowInterrupt = true;
  lastSwitch = millis();
  forceMessageRestart = true;

  Serial.printf(PSTR("[POMODORO] Started: %d min work / %d min break\n"), workMin, breakMin);
  return true;
}

// -----------------------------------------------------------------------------
// Alarm Command Handler
// Syntax: [ALARM]         — scroll current schedule as text
//         [ALARM STOP]    — dismiss a ringing alarm
//         [ALARM SNOOZE]  — snooze a ringing alarm
//         [ALARM TEST]    — fire the alarm now, for testing
// -----------------------------------------------------------------------------
bool handleAlarmCommand(String cmd) {
  cmd.toUpperCase();  // Reassign the uppercase result
  if (cmd.indexOf("[ALARM") == -1) return false;

  // Detect which slot: [ALARM ...] = 0, [ALARM2 ...] = 1, [ALARM3 ...] = 2, [ALARM4 ...] = 3
  int index = 0;
  int prefixLen = 6;  // length of "[ALARM"
  if (cmd.length() > 6 && isDigit(cmd.charAt(6))) {
    index = cmd.charAt(6) - '0' - 1;
    prefixLen = 7;
    if (index < 0 || index >= MAX_ALARMS) return false;
  }

  String rest = cmd.substring(prefixLen);
  // REMOVED: rest.trim(); -- keep spaces so startsWith(" ENABLE]") works properly

  if (rest.startsWith("] STOP]") || rest.startsWith(" STOP]")) {
    if (alarmRinging) {
      buzzerStop();
      showAlarmNotification("ALARM OFF");
    }
    return true;
  }

  if (rest.startsWith(" SNOOZE]")) {
    if (alarmRinging) snoozeAlarm();
    return true;
  }

  if (rest.startsWith(" ENABLE]")) {
    alarmConfigs[index].enabled = true;
    configDirty = true;
    lastBrightnessChange = millis();
    Serial.printf(PSTR("[ALARM] Slot %d enabled via command.\n"), index);
    return true;
  }

  if (rest.startsWith(" DISABLE]")) {
    alarmConfigs[index].enabled = false;
    configDirty = true;
    lastBrightnessChange = millis();
    Serial.printf(PSTR("[ALARM] Slot %d disabled via command.\n"), index);
    return true;
  }

  if (clockOnlyDuringDimming && dimActive) {
    Serial.println(F("[ALARM] Ignored: Clock-only Dimming is active."));
    return true;
  }
  if (!allowInterrupt) {
    Serial.println(F("[ALARM] Ignored: Protected message is active."));
    return true;
  }

  if (rest.startsWith(" TEST]")) {
    alarmTest = true;
    fireAlarm(index, -1, -1);
    Serial.printf(PSTR("[ALARM] Slot %d test fired via command.\n"), index);
    return true;
  }

  if (rest.startsWith("]")) {
    if (alarmRinging) {
      Serial.println(F("[ALARM] Ignored: an alarm is currently ringing."));
      return true;
    }
    String schedule = formatAlarmSchedule(alarmConfigs[index]);
    strncpy(customMessage, schedule.c_str(), sizeof(customMessage) - 1);
    customMessage[sizeof(customMessage) - 1] = '\0';
    messageBigNumbers = false;
    messageDisplaySeconds = 0;
    messageScrollTimes = 1;
    messageScrollSpeed = RSS_SCROLL_SPEED;
    prevDisplayMode = displayMode;
    displayMode = 6;
    forceMessageRestart = true;
    Serial.println("[ALARM] Schedule requested: " + schedule);
    return true;
  }

  if (rest.startsWith(" SET ")) {
    int start = 5;
    int end = rest.indexOf("]", start);
    if (end == -1) return false;
    String params = rest.substring(start, end);
    params.trim();

    String tokens[6];
    int tokenCount = 0;
    int pos = 0;
    while (tokenCount < 6 && pos < (int)params.length()) {
      int sp = params.indexOf(' ', pos);
      if (sp == -1) {
        tokens[tokenCount++] = params.substring(pos);
        break;
      }
      tokens[tokenCount++] = params.substring(pos, sp);
      pos = sp + 1;
    }

    int colonIdx = tokens[0].indexOf(':');
    if (colonIdx == -1) return false;
    int h = tokens[0].substring(0, colonIdx).toInt();
    int m = tokens[0].substring(colonIdx + 1).toInt();
    if (h < 0 || h > 23 || m < 0 || m > 59) return false;

    bool days[7];
    if (tokenCount < 2 || tokens[1].length() == 0) {
      for (int i = 0; i < 7; i++) days[i] = true;
    } else {
      for (int i = 0; i < 7; i++) days[i] = false;
      for (unsigned int i = 0; i < tokens[1].length(); i++) {
        int d = tokens[1].charAt(i) - '0';
        if (d >= 0 && d <= 6) days[d] = true;
      }
    }

    setAlarmSchedule(index, h, m, days);

    if (tokenCount >= 3) {
      int soundId = tokens[2].toInt();
      if (soundId >= 1 && soundId <= 3) alarmConfigs[index].sound = (uint8_t)soundId;
    }
    if (tokenCount >= 4) {
      int bri = tokens[3].toInt();
      if (bri >= 0 && bri <= 15) alarmConfigs[index].brightness = (uint8_t)bri;
    }
    if (tokenCount >= 5) {
      int snooze = tokens[4].toInt();
      if (snooze >= 1 && snooze <= 60) alarmConfigs[index].snoozeMinutes = (uint8_t)snooze;
    }
    if (tokenCount >= 6) {
      int vol = tokens[5].toInt();
      if (vol >= 1 && vol <= 10) buzzerConfig.volume = (uint8_t)vol;
    }
    configDirty = true;
    lastBrightnessChange = millis();

    Serial.printf(PSTR("[ALARM] Slot %d set via command: %02d:%02d\n"), index, h, m);
    return true;
  }

  return false;
}

// Timer Helper
bool handleTimerCommand(String cmd) {
  cmd.toUpperCase();
  if (cmd.indexOf("[STOPWATCH]") != -1) cmd = "[TIMER STOPWATCH]";
  if (cmd.indexOf("[STOPWATCH PAUSE]") != -1) cmd = "[TIMER PAUSE]";
  if (cmd.indexOf("[STOPWATCH RESUME]") != -1) cmd = "[TIMER RESUME]";
  if (cmd.indexOf("[STOPWATCH STOP]") != -1) cmd = "[TIMER STOP]";
  if (cmd.indexOf("[STOPWATCH CLEAR]") != -1) cmd = "[TIMER CLEAR]";
  if (cmd.indexOf("[STOPWATCH RESTART]") != -1) cmd = "[TIMER STOPWATCH]";
  if (cmd.indexOf("[STOPWATCH RESET]") != -1) cmd = "[TIMER RESET]";
  if (cmd.indexOf("[TIMER") == -1) return false;

  int start = cmd.indexOf("[TIMER") + 6;
  int end = cmd.indexOf("]", start);
  String payload = cmd.substring(start, end);
  payload.trim();

  // Prevent starting or resuming timers while Clock-only Dimming is active
  if (clockOnlyDuringDimming && dimActive) {
    if (payload != "STOP" && payload != "CANCEL" && payload != "CLEAR" && payload != "PAUSE" && payload != "RESET") {
      Serial.println(F("[TIMER] Ignored: Clock-only Dimming is active."));
      return true;
    }
  }

  // Prevent starting or resuming timers while a protected message is active
  if (!allowInterrupt) {
    if (payload != "STOP" && payload != "CANCEL" && payload != "CLEAR" && payload != "PAUSE" && payload != "RESET") {
      Serial.println(F("[TIMER] Ignored: Protected message is active."));
      return true;
    }
  }

  if (payload == "STOP" || payload == "CANCEL") {
    if (isStopwatch) {
      // Stopwatch: STOP = PAUSE (keeps the time frozen; use CLEAR to reset)
      if (!timerPaused) {
        timerPaused = true;
        timerRemainingAtPause = millis() - timerEndTime;
      }
      Serial.println(F("[STOPWATCH] Paused. Use CLEAR to reset to zero."));
    } else {
      // Countdown timer / Pomodoro: STOP = fully stop and return to Clock
      timerActive = false;
      timerFinished = false;
      timerPaused = false;
      isStopwatch = false;
      buzzerStop();
      displayMode = 0;
      prevDisplayMode = 6;
      clockScrollDone = false;
      forceMessageRestart = true;
      lastSwitch = millis();
      Serial.println(F("[TIMER] Stopped. Returning to Clock."));
    }
    return true;
  }

  if (payload == "RESET") {
    if (isStopwatch) {
      timerPaused = true;
      timerRemainingAtPause = 0;
      timerFinished = false;
      timerEndTime = millis();  // zero elapsed time
      displayMode = 7;
      allowInterrupt = true;
      lastSwitch = millis();
      forceMessageRestart = true;
      Serial.println(F("[STOPWATCH] Reset."));
      return true;
    }
    return false;
  }

  if (payload == "CLEAR") {
    bool wasStopwatch = isStopwatch;
    timerActive = false;
    timerPaused = false;
    timerFinished = false;
    isStopwatch = false;
    displayMode = 0;
    prevDisplayMode = 6;
    clockScrollDone = false;
    forceMessageRestart = true;
    lastSwitch = millis();
    Serial.println(wasStopwatch ? F("[STOPWATCH] Cleared. Returning to Clock.") : F("[TIMER] Cleared. Returning to Clock."));
    return true;
  }

  if (payload == "PAUSE") {
    if (timerActive && !timerPaused && !timerFinished) {
      timerPaused = true;
      timerRemainingAtPause = isStopwatch ? (millis() - timerEndTime) : (timerEndTime - millis());
    }
    return true;
  }

  if (payload == "RESUME" || payload == "START") {
    if (timerActive && timerPaused) {
      timerEndTime = isStopwatch ? millis() - timerRemainingAtPause   // rewind start time
                                 : millis() + timerRemainingAtPause;  // existing countdown logic
      timerPaused = false;
    }
    return true;
  }

  if (payload == "RESTART") {
    if (timerOriginalDuration > 0) {
      isStopwatch = false;
      timerEndTime = millis() + timerOriginalDuration;
      timerActive = true;
      timerPaused = false;
      timerFinished = false;
      displayMode = 7;
      allowInterrupt = true;
      timerSubState = 0;
      lastSwitch = millis();
      forceMessageRestart = true;
      return true;
    }
    return false;
  }

  if (payload == "STOPWATCH") {

    // Already running -> Pause
    if (timerActive && isStopwatch && !timerPaused) {
      timerPaused = true;
      timerRemainingAtPause = millis() - timerEndTime;
      buzzerFireEvent(BUZZER_EVT_STOPWATCH);
      return true;
    }

    // Already paused -> Resume
    if (timerActive && isStopwatch && timerPaused) {
      timerEndTime = millis() - timerRemainingAtPause;
      timerPaused = false;
      buzzerFireEvent(BUZZER_EVT_STOPWATCH);
      return true;
    }

    // Not active -> Start a new stopwatch
    isStopwatch = true;
    isPomodoroActive = false;
    pomodoroInBreak = false;
    timerActive = true;
    timerPaused = false;
    timerFinished = false;
    timerEndTime = millis();
    timerSubState = 0;
    displayMode = 7;
    allowInterrupt = true;
    lastSwitch = millis();
    forceMessageRestart = true;
    buzzerFireEvent(BUZZER_EVT_STOPWATCH);
    return true;
  }

  long totalMs = 0;
  String val = "";
  for (unsigned int i = 0; i < payload.length(); i++) {
    char c = payload.charAt(i);
    if (isDigit(c)) val += c;
    else {
      long num = val.toInt();
      if (c == 'H') totalMs += num * 3600000;
      else if (c == 'M') totalMs += num * 60000;
      else if (c == 'S') totalMs += num * 1000;
      val = "";
    }
  }
  if (val.length() > 0 && totalMs == 0) totalMs = val.toInt() * 60000;

  if (totalMs > 86400000) totalMs = 86400000;  // 24h Cap

  if (totalMs > 0) {
    isStopwatch = false;
    timerOriginalDuration = totalMs;
    timerEndTime = millis() + totalMs;
    isPomodoroActive = false;
    pomodoroInBreak = false;
    timerActive = true;
    timerPaused = false;
    timerFinished = false;
    timerSubState = 0;
    displayMode = 7;
    allowInterrupt = true;
    lastSwitch = millis();
    forceMessageRestart = true;
    return true;
  }
  return false;
}

//Actions handler
void executeAction(const String &action, const String &value) {
  if (value.length() > 0) {
    Serial.print(F("[ACTION] "));
    Serial.print(action);
    Serial.print(F(" = "));
    Serial.println(value);
  } else {
    Serial.print(F("[ACTION] "));
    Serial.println(action);
  }
  String v = value;
  v.trim();
  v.toLowerCase();
  bool hasValue = v.length() > 0;
  bool boolVal = (v == "1" || v == "true" || v == "on");

  // Display actions
  if (action == "next_mode") {
    if (alarmRinging || displayMode == 8) {
      Serial.println(F("[ACTION] next_mode ignored: Alarm is active."));
      return;
    }
    if (displayMode == 0 && prevDisplayMode == 0) {
      Serial.println(F("[ACTION] next_mode ignored: already in CLOCK."));
      return;
    }
    advanceDisplayMode(true);
    if (displayMode == 0 && prevDisplayMode != 1 && prevDisplayMode != 5 && (prevDisplayMode != 6 || totalPixelWidth >= 27)) {
      pendingModeShiftOut = true;
    }
  } else if (action == "prev_mode") {
    previousDisplayMode(true);

  } else if (action == "brightness") {
    handleBrightnessChange(value.toInt(), false);

  } else if (action == "brightness_up") {
    handleBrightnessChange(displayOff ? lastBrightnessBeforeOff : constrain(brightness + 1, 0, 15), false);

  } else if (action == "brightness_down") {
    if (!displayOff) { handleBrightnessChange(constrain(brightness - 1, 0, 15), false); }

  } else if (action == "display_off") {
    handleBrightnessChange(-1, false);

  } else if (action == "display_on") {
    handleBrightnessChange(lastBrightnessBeforeOff, false);

  } else if (action == "flip" || action == "flip_display") {
    flipDisplay = hasValue ? boolVal : !flipDisplay;
    P.setZoneEffect(0, flipDisplay, PA_FLIP_UD);
    P.setZoneEffect(0, flipDisplay, PA_FLIP_LR);

  } else if (action == "twelvehour" || action == "twelve_hour") {
    twelveHourToggle = hasValue ? boolVal : !twelveHourToggle;
    if (!hasValue && displayMode != 0) { goToMode("0"); }

  } else if (action == "dayofweek" || action == "show_dayofweek") {
    showDayOfWeek = hasValue ? boolVal : !showDayOfWeek;
    if (!hasValue && displayMode != 0) { goToMode("0"); }

  } else if (action == "showdate" || action == "show_date") {
    bool newVal = hasValue ? boolVal : !showDate;
    if (showDate && !newVal && displayMode == 5) { advanceDisplayMode(true); }
    showDate = newVal;
    if (!hasValue && showDate) { goToMode("5"); }

  } else if (action == "colon_blink" || action == "animated_seconds") {
    colonBlinkEnabled = hasValue ? boolVal : !colonBlinkEnabled;
    if (!hasValue && displayMode != 0) { goToMode("0"); }

  } else if (action == "humidity" || action == "show_humidity") {
    showHumidity = hasValue ? boolVal : !showHumidity;
    if (!hasValue) { goToMode("1"); }  // show the change on weather

  } else if (action == "weatherdesc" || action == "show_weather_desc") {
    bool newVal = hasValue ? boolVal : !showWeatherDescription;
    if (showWeatherDescription && !newVal && displayMode == 2) { advanceDisplayMode(true); }
    showWeatherDescription = newVal;
    if (!hasValue && showWeatherDescription) { goToMode("2"); }  // only jump when turning ON

  } else if (action == "units" || action == "imperial") {
    bool isImperial = (action == "imperial") ? (!hasValue ? true : boolVal) : (hasValue ? boolVal : strcmp(weatherUnits, "imperial") != 0);
    if (isImperial) {
      strcpy(weatherUnits, "imperial");
      tempSymbol = '\007';
    } else {
      strcpy(weatherUnits, "metric");
      tempSymbol = '\006';
    }
    shouldFetchWeatherNow = true;
    if (!hasValue) { goToMode("1"); }  // show the change on weather

  } else if (action == "metric") {
    strcpy(weatherUnits, "metric");
    tempSymbol = '\006';
    shouldFetchWeatherNow = true;
    if (!hasValue) { goToMode("1"); }  // show the change on weather

  } else if (action == "countdown_enabled" || action == "countdown") {
    bool newVal = hasValue ? boolVal : !countdownEnabled;
    if (countdownEnabled && !newVal && displayMode == 3) { advanceDisplayMode(true); }
    countdownEnabled = newVal;

  } else if (action == "dramatic_countdown") {
    bool newVal = hasValue ? boolVal : !isDramaticCountdown;
    if (isDramaticCountdown != newVal) {
      isDramaticCountdown = newVal;
      saveCountdownConfig(countdownEnabled, countdownTargetTimestamp, countdownLabel);
    }

  } else if (action == "clock_only_dimming") {
    clockOnlyDuringDimming = hasValue ? boolVal : !clockOnlyDuringDimming;
    configDirty = true;

  } else if (action == "go_to_mode") {
    goToMode(value);

  } else if (action == "enable_rotation") {
    rotationEnabled = hasValue ? boolVal : !rotationEnabled;
    if (rotationEnabled && displayMode != 6) advanceDisplayMode();

  }

  // Buzzer actions
  else if (action == "buzzer_enable") {
    buzzerConfig.enabled = hasValue ? boolVal : !buzzerConfig.enabled;
    configDirty = true;
    lastBrightnessChange = millis();

  } else if (action == "buzzer_volume") {
    buzzerConfig.volume = constrain(value.toInt(), 1, 10);
    configDirty = true;
    lastBrightnessChange = millis();

  } else if (action == "play_sound") {
    int sep1 = value.indexOf(':');
    uint8_t soundId;
    int previewVolume = -1;
    bool repeat = false;

    if (sep1 != -1) {
      soundId = (uint8_t)value.substring(0, sep1).toInt();
      int sep2 = value.indexOf(':', sep1 + 1);
      if (sep2 == -1) {
        previewVolume = value.substring(sep1 + 1).toInt();
      } else {
        previewVolume = value.substring(sep1 + 1, sep2).toInt();
        repeat = value.substring(sep2 + 1).toInt() != 0;
      }
    } else {
      soundId = hasValue ? (uint8_t)value.toInt() : 1;
    }

    if (previewVolume >= 1 && previewVolume <= 10) {
      if (!buzzerVolumeOverridden) {
        buzzerSavedVolumeForPreview = buzzerConfig.volume;
      }
      buzzerConfig.volume = (uint8_t)previewVolume;
      buzzerVolumeOverridden = true;
    }

    buzzerTrigger(getSoundPattern(soundId), true, true);
    buzzerRepeating = repeat;
    buzzerEventStopAt = 0;

  } else if (action == "buzzer_stop") {
    buzzerStop();

  } else if (action == "buzzer_event") {
    int sep1 = value.indexOf(':');
    if (sep1 > 0) {
      String evtName = value.substring(0, sep1);
      int sep2 = value.indexOf(':', sep1 + 1);
      int soundId = (sep2 == -1) ? value.substring(sep1 + 1).toInt() : value.substring(sep1 + 1, sep2).toInt();
      int idx = buzzerEventIndexFromName(evtName);
      if (idx >= 0) {
        buzzerConfig.eventEnabled[idx] = (soundId > 0);
        buzzerConfig.eventSound[idx] = (uint8_t)max(soundId, 0);
        if (sep2 != -1) {
          buzzerConfig.eventRepeat[idx] = value.substring(sep2 + 1).toInt() != 0;
        }
        configDirty = true;
        lastBrightnessChange = millis();
        Serial.printf(PSTR("[BUZZER] Event '%s' -> sound %d, repeat=%s (enabled=%s)\n"),
                      evtName.c_str(), soundId,
                      buzzerConfig.eventRepeat[idx] ? "true" : "false",
                      buzzerConfig.eventEnabled[idx] ? "true" : "false");
      } else {
        Serial.print(F("[BUZZER] Unknown event name: "));
        Serial.println(evtName);
      }
    }
  }

  // Alarm actions
  else if (action.startsWith("alarm") && (action.indexOf("_") != -1)) {
    int underscorePos = action.indexOf("_");
    String prefix = action.substring(0, underscorePos);
    String sub = action.substring(underscorePos + 1);

    int index = 0;
    if (prefix.length() > 5) {
      index = prefix.substring(5).toInt() - 1;
      if (index < 0 || index >= MAX_ALARMS) return;
    }

    if (sub == "stop") {
      if (alarmRinging) {
        buzzerStop();
        showAlarmNotification("ALARM OFF");
      }

    } else if (sub == "snooze") {
      if (alarmRinging) snoozeAlarm();

    } else if (sub == "enable") {
      alarmConfigs[index].enabled = hasValue ? boolVal : !alarmConfigs[index].enabled;
      configDirty = true;
      lastBrightnessChange = millis();

    } else if (sub == "test") {
      int previewBrightness = -1;
      int previewSound = -1;
      if (hasValue) {
        int sep = value.indexOf(':');
        if (sep == -1) {
          previewBrightness = value.toInt();
        } else {
          previewBrightness = value.substring(0, sep).toInt();
          previewSound = value.substring(sep + 1).toInt();
        }
      }
      alarmTest = true;
      fireAlarm(index, previewBrightness, previewSound);
    } else if (sub == "set") {
      int c1 = value.indexOf(':');
      int c2 = value.indexOf(':', c1 + 1);
      int c3 = (c2 == -1) ? -1 : value.indexOf(':', c2 + 1);
      int c4 = (c3 == -1) ? -1 : value.indexOf(':', c3 + 1);
      int c5 = (c4 == -1) ? -1 : value.indexOf(':', c4 + 1);
      int c6 = (c5 == -1) ? -1 : value.indexOf(':', c5 + 1);

      int h = value.substring(0, c1).toInt();
      int m;
      String daysPart = "";
      int soundId = -1;
      int bri = -1;
      int snooze = -1;
      int vol = -1;

      if (c2 == -1) {
        m = value.substring(c1 + 1).toInt();
      } else if (c3 == -1) {
        m = value.substring(c1 + 1, c2).toInt();
        daysPart = value.substring(c2 + 1);
      } else if (c4 == -1) {
        m = value.substring(c1 + 1, c2).toInt();
        daysPart = value.substring(c2 + 1, c3);
        soundId = value.substring(c3 + 1).toInt();
      } else if (c5 == -1) {
        m = value.substring(c1 + 1, c2).toInt();
        daysPart = value.substring(c2 + 1, c3);
        soundId = value.substring(c3 + 1, c4).toInt();
        bri = value.substring(c4 + 1).toInt();
      } else if (c6 == -1) {
        m = value.substring(c1 + 1, c2).toInt();
        daysPart = value.substring(c2 + 1, c3);
        soundId = value.substring(c3 + 1, c4).toInt();
        bri = value.substring(c4 + 1, c5).toInt();
        snooze = value.substring(c5 + 1).toInt();
      } else {
        m = value.substring(c1 + 1, c2).toInt();
        daysPart = value.substring(c2 + 1, c3);
        soundId = value.substring(c3 + 1, c4).toInt();
        bri = value.substring(c4 + 1, c5).toInt();
        snooze = value.substring(c5 + 1, c6).toInt();
        vol = value.substring(c6 + 1).toInt();
      }

      bool days[7];
      if (daysPart.length() == 0) {
        for (int i = 0; i < 7; i++) days[i] = true;
      } else {
        for (int i = 0; i < 7; i++) days[i] = false;
        for (unsigned int i = 0; i < daysPart.length(); i++) {
          int d = daysPart.charAt(i) - '0';
          if (d >= 0 && d <= 6) days[d] = true;
        }
      }

      setAlarmSchedule(index, h, m, days);

      if (soundId >= 1 && soundId <= 3) alarmConfigs[index].sound = (uint8_t)soundId;
      if (bri >= 0 && bri <= 15) alarmConfigs[index].brightness = (uint8_t)bri;
      if (snooze >= 1 && snooze <= 60) alarmConfigs[index].snoozeMinutes = (uint8_t)snooze;
      if (vol >= 1 && vol <= 10) buzzerConfig.volume = (uint8_t)vol;
      configDirty = true;
      lastBrightnessChange = millis();

      Serial.printf(PSTR("[ALARM] Slot %d set via action: %02d:%02d\n"), index, h, m);
    }
  }

  // Timer commands
  else if (action == "timer_stop" || action == "timer_cancel") {
    handleTimerCommand("[TIMER STOP]");
  } else if (action == "timer_pause") {
    handleTimerCommand("[TIMER PAUSE]");
  } else if (action == "timer_resume" || action == "timer_start") {
    handleTimerCommand("[TIMER RESUME]");
  } else if (action == "timer_restart") {
    if (!(clockOnlyDuringDimming && dimActive)) handleTimerCommand("[TIMER RESTART]");
  } else if (action == "timer") {
    if (!(clockOnlyDuringDimming && dimActive)) handleTimerCommand("[TIMER " + value + "]");
  } else if (action == "stopwatch" || action == "stopwatch_start") {
    if (!(clockOnlyDuringDimming && dimActive))
      handleTimerCommand("[STOPWATCH]");
  } else if (action == "stopwatch_pause") {
    handleTimerCommand("[STOPWATCH PAUSE]");
  } else if (action == "stopwatch_resume") {
    handleTimerCommand("[STOPWATCH RESUME]");
  } else if (action == "stopwatch_stop" || action == "stopwatch_cancel") {
    handleTimerCommand("[STOPWATCH STOP]");
  } else if (action == "stopwatch_restart") {
    if (!(clockOnlyDuringDimming && dimActive))
      handleTimerCommand("[STOPWATCH RESTART]");
  } else if (action == "stopwatch_reset") {
    handleTimerCommand("[STOPWATCH RESET]");
  } else if (action == "stopwatch_clear") {
    handleTimerCommand("[STOPWATCH CLEAR]");
  } else if (action == "pomodoro_start" || action == "pom") {
    if (!(clockOnlyDuringDimming && dimActive)) handlePomodoroCommand("[POMODORO]");
  } else if (action == "pomodoro") {
    if (!(clockOnlyDuringDimming && dimActive)) handlePomodoroCommand("[POMODORO " + value + "]");
  } else if (action == "pomodoro_stop") {
    handlePomodoroCommand("[POMODORO STOP]");
  } else if (action == "pomodoro_restart") {
    if (!(clockOnlyDuringDimming && dimActive)) handlePomodoroCommand("[POMODORO RESTART]");
  } else if (action == "pomodoro_pause") {
    handleTimerCommand("[TIMER PAUSE]");
  } else if (action == "pomodoro_resume") {
    handleTimerCommand("[TIMER RESUME]");

  } else if (action == "restart") {
    pendingRestart = true;
    restartTimer = millis();

  } else if (action == "save") {
    configDirty = true;

  } else if (action == "language") {
    String lang = value;
    lang.trim();
    lang.toLowerCase();
    strlcpy(language, lang.c_str(), sizeof(language));
    shouldFetchWeatherNow = true;
    advanceDisplayMode();

  } else if (action == "clear_message") {
    allowInterrupt = true;
    forceMessageRestart = true;
    customMessage[0] = '\0';
    messageStartTime = 0;
    currentScrollCount = 0;
    messageDisplaySeconds = 0;
    messageScrollTimes = 0;

    if (strlen(lastPersistentMessage) > 0) {
      strlcpy(customMessage, lastPersistentMessage, sizeof(customMessage));
      messageScrollSpeed = GENERAL_SCROLL_SPEED;
      displayMode = 6;
      prevDisplayMode = 0;
      Serial.println(F("[MESSAGE] clear_message: restored persistent message"));
    } else {
      displayMode = 0;
      prevDisplayMode = 6;
      clockScrollDone = false;
      Serial.println(F("[MESSAGE] clear_message: no persistent message, returning to clock"));
    }

  } else if (action == "clear_message_all") {
    allowInterrupt = true;
    forceMessageRestart = true;

    customMessage[0] = '\0';
    lastPersistentMessage[0] = '\0';

    messageStartTime = 0;
    currentScrollCount = 0;
    messageDisplaySeconds = 0;
    messageScrollTimes = 0;

    displayMode = 0;
    prevDisplayMode = 6;
    lastSwitch = millis();
    clockScrollDone = false;

    saveCustomMessageToConfig("");

    Serial.println(F("[MESSAGE] clear_message_all: all messages cleared"));

  } else {
    Serial.print(F("[ACTION] Unknown action: "));
    Serial.println(action);
  }
}

void handleBrightnessChange(int newBrightness, bool isFromUI) {
  // --- CASE 1: Turn Display OFF ---
  if (newBrightness == -1) {
    if (!displayOff) {
      if (brightness >= 0) lastBrightnessBeforeOff = brightness;
      P.displayShutdown(true);
      P.displayClear();
      displayOff = true;
      brightness = -1;
      configDirty = true;
      lastBrightnessChange = millis();
      Serial.printf(PSTR("[BRIGHTNESS] Display turned OFF via %s\n"), isFromUI ? "UI" : "API");
    }
    return;
  }

  // --- CASE 2: Turn Display ON or Adjust ---
  newBrightness = constrain(newBrightness, 0, 15);

  if (newBrightness != brightness || displayOff) {
    bool wakingUp = displayOff;
    brightness = newBrightness;
    configDirty = true;
    lastBrightnessChange = millis();
    P.setIntensity(brightness);

    if (wakingUp) {
      advanceDisplayMode(true);
      P.displayShutdown(false);
      P.displayClear();
      displayOff = false;
      Serial.printf(PSTR("[BRIGHTNESS] Display woke from OFF via %s to %d\n"), isFromUI ? "UI" : "API", brightness);
    } else {
      Serial.printf(PSTR("[BRIGHTNESS] Intensity set to %d via %s\n"), brightness, isFromUI ? "UI" : "API");
    }
  }
}

void goToMode(const String &target) {
  int targetMode = -1;
  String v = target;
  v.toLowerCase();

  if (v == "0" || v == "clock") targetMode = 0;
  else if (v == "1" || v == "weather") targetMode = 1;
  else if (v == "2" || v == "weather_desc") targetMode = 2;
  else if (v == "3" || v == "countdown") targetMode = 3;
  else if (v == "4" || v == "nightscout" || v == "bridge") targetMode = 4;
  else if (v == "5" || v == "date") targetMode = 5;
  else if (v == "6" || v == "message") targetMode = 6;
  else if (v == "7" || v == "timer") targetMode = 7;

  if (targetMode == -1 || !isModeAvailable(targetMode)) {
    Serial.printf(PSTR("[DISPLAY] go_to_mode: invalid or unavailable target '%s'\n"), target.c_str());
    return;
  }

  // ---- CLEANUP CURRENT MODE ----
  if (displayMode == 3) {
    countdownSegment = 0;
    segmentStartTime = 0;
    countdownShowFinishedMessage = false;
    hourglassPlayed = false;
  }

  prevDisplayMode = displayMode;  // general line already there
  displayMode = targetMode;

  // ---- RESET TARGET MODE STATE ----
  if (targetMode == 0) {
    prevDisplayMode = 6;
    clockScrollDone = false;
    P.displayReset();
    P.displayClear();
    delay(100);
  }

  if (displayMode == 6 || displayMode == 2 || displayMode == 3) {
    P.displayReset();
    P.displayClear();
  }

  // ---- RESET TARGET MODE STATE ----
  if (targetMode == 3) {
    countdownSegment = 0;
    segmentStartTime = 0;
    countdownShowFinishedMessage = false;
    hourglassPlayed = false;
  }

  // ---- SYNC modeIndex ----
  for (int i = 0; i < MODE_COUNT; i++) {
    if (modeOrder[i] == displayMode) {
      modeIndex = i;
      break;
    }
  }

  // ---- RESET SCROLL STATE ----
  clockScrollDone = false;
  descScrolling = false;
  descScrollEndTime = 0;

  const char *modeNames[] = { "CLOCK", "WEATHER", "WEATHER DESC", "COUNTDOWN", "BRIDGE", "DATE", "CUSTOM MESSAGE", "TIMER" };
  Serial.printf(PSTR("[DISPLAY] go_to_mode: %s (from %s)\n"), modeNames[targetMode], modeNames[prevDisplayMode]);
  lastSwitch = millis();
}

void loadPins() {
  PinStorage.begin(EEPROM_SIZE);
  PinConfig cfg;
  PinStorage.get(EEPROM_ADDR, cfg);

  if (cfg.magic != EEPROM_MAGIC) {
    // Installer didn't write EEPROM — manual flash user
    // Write defaults so this only runs once
    Serial.println(F("[PIN CONFIG] No EEPROM data - writing defaults"));
    cfg.magic = EEPROM_MAGIC;
    cfg.clk = L_CLK;
    cfg.cs = L_CS;
    cfg.data = L_DATA;
    PinStorage.put(EEPROM_ADDR, cfg);
    PinStorage.commit();
  }

  // Basic validation
  if (cfg.clk == 0 || cfg.cs == 0 || cfg.data == 0 || cfg.clk == cfg.cs || cfg.clk == cfg.data || cfg.cs == cfg.data) {
    Serial.println(F("[PIN CONFIG] Invalid pins - fallback to defaults"));
    cfg.clk = L_CLK;
    cfg.cs = L_CS;
    cfg.data = L_DATA;
  }

  CLK_PIN = cfg.clk;
  CS_PIN = cfg.cs;
  DATA_PIN = cfg.data;

  Serial.printf(PSTR("[PIN CONFIG] Loaded pins - CLK:%d CS:%d DATA:%d\n"), CLK_PIN, CS_PIN, DATA_PIN);
}

// -----------------------------------------------------------------------------
// Buzzer: hardware driver (passive piezo, frequency + PWM duty for volume)
// -----------------------------------------------------------------------------
#if defined(ESP32)
const uint8_t BUZZER_LEDC_RES = 10;  // 0-1023

void buzzerHwSetup() {
  if (buzzerConfig.pin == 255) return;
  ledcAttach(buzzerConfig.pin, 2000, BUZZER_LEDC_RES);
}

void buzzerHwWrite(uint16_t freq, uint32_t duty) {
  if (buzzerConfig.pin == 255) return;
  if (freq == 0 || duty == 0) {
    ledcWrite(buzzerConfig.pin, 0);
    digitalWrite(buzzerConfig.pin, LOW);
  } else {
    ledcChangeFrequency(buzzerConfig.pin, freq, BUZZER_LEDC_RES);
    ledcWrite(buzzerConfig.pin, duty);
  }
}

uint32_t buzzerVolumeToDuty(uint8_t volume) {
  volume = constrain(volume, 1, 10);
  float pct = map(volume, 1, 10, 5, 50) / 100.0f;
  return (uint32_t)(pct * ((1 << BUZZER_LEDC_RES) - 1));
}

#elif defined(ESP8266)
const uint16_t BUZZER_PWM_RANGE = 1023;

void buzzerHwSetup() {
  if (buzzerConfig.pin == 255) return;
  pinMode(buzzerConfig.pin, OUTPUT);
  analogWriteRange(BUZZER_PWM_RANGE);
}

void buzzerHwWrite(uint16_t freq, uint32_t duty) {
  if (buzzerConfig.pin == 255) return;
  if (freq == 0 || duty == 0) {
    analogWrite(buzzerConfig.pin, 0);
  } else {
    analogWriteFreq(freq);
    analogWrite(buzzerConfig.pin, duty);
  }
}

uint32_t buzzerVolumeToDuty(uint8_t volume) {
  volume = constrain(volume, 1, 10);
  float pct = map(volume, 1, 10, 5, 50) / 100.0f;
  return (uint32_t)(pct * BUZZER_PWM_RANGE);
}
#endif

void buzzerApplyStep(uint8_t i) {
  if (!buzzerActivePattern || i >= buzzerActivePattern->stepCount) return;
  BuzzerStep s = buzzerActivePattern->steps[i];
  if (s.freq > 0) {
    buzzerHwWrite(s.freq, buzzerVolumeToDuty(buzzerConfig.volume));
  } else {
    buzzerHwWrite(0, 0);
  }
}

void showAlarmNotification(const String &message) {
  if (alarmTest) {
    alarmTest = false;
    return;
  }
  strncpy(customMessage, message.c_str(), sizeof(customMessage) - 1);
  customMessage[sizeof(customMessage) - 1] = '\0';

  messageBigNumbers = false;
  messageDisplaySeconds = 0;
  messageScrollTimes = 1;
  messageScrollSpeed = RSS_SCROLL_SPEED;

  prevDisplayMode = 6;
  displayMode = 6;
  forceMessageRestart = true;
  lastSwitch = millis();

  Serial.print(F("[ALARM] Notification: "));
  Serial.println(message);
}

void buzzerStop() {
  buzzerHwWrite(0, 0);
  buzzerState = BUZZER_IDLE;
  buzzerActivePattern = nullptr;
  buzzerRepeating = false;
  buzzerEventStopAt = 0;
  alarmStopAt = 0;

  if (buzzerVolumeOverridden) {
    buzzerConfig.volume = buzzerSavedVolumeForPreview;
    buzzerVolumeOverridden = false;
  }
  if (alarmRinging) {
    displayMode = 0;
    prevDisplayMode = 6;
    clockScrollDone = false;
    forceMessageRestart = true;
    lastSwitch = millis();
    rotationEnabled = alarmSavedRotationEnabled;
    if (alarmSavedDisplayOff) {
      handleBrightnessChange(-1, false);
    } else {
      handleBrightnessChange(alarmSavedBrightness, false);
    }
  }
  P.setInvert(0);
  alarmRinging = false;
  alarmRingingIndex = -1;
}

void buzzerTrigger(const BuzzerPattern *pattern, bool forceNoRepeat, bool ignoreEnabledFlag) {
  if (buzzerConfig.pin == 255) return;
  if (!ignoreEnabledFlag && !buzzerConfig.enabled) return;
  if (!pattern || pattern->stepCount == 0) return;

  buzzerHwWrite(0, 0);  // cut off whatever's currently playing before starting the new one

  buzzerActivePattern = pattern;
  buzzerCurrentStep = 0;
  buzzerRepeating = forceNoRepeat ? false : pattern->repeat;
  buzzerStepStartTime = millis();
  buzzerState = BUZZER_PLAYING;
  buzzerApplyStep(0);
}

void buzzerTrigger(const BuzzerPattern *pattern) {
  buzzerTrigger(pattern, false, false);
}

void buzzerLoop() {
  if (buzzerState != BUZZER_PLAYING || !buzzerActivePattern) return;

  if (buzzerEventStopAt != 0 && millis() >= buzzerEventStopAt) {
    buzzerStop();
    return;
  }

  BuzzerStep step = buzzerActivePattern->steps[buzzerCurrentStep];
  if (millis() - buzzerStepStartTime >= step.durationMs) {
    buzzerCurrentStep++;
    if (buzzerCurrentStep >= buzzerActivePattern->stepCount) {
      if (buzzerRepeating) {
        buzzerCurrentStep = 0;
      } else {
        buzzerStop();
        return;
      }
    }
    buzzerStepStartTime = millis();
    buzzerApplyStep(buzzerCurrentStep);
  }
}

// Call this from Timer/Pomodoro/Countdown finish points once you're ready to wire events in
void buzzerFireEvent(BuzzerEventIndex evt) {
  if (evt >= BUZZER_EVENT_COUNT || !buzzerConfig.enabled) return;
  if (!buzzerConfig.eventEnabled[evt]) return;

  buzzerTrigger(getSoundPattern(buzzerConfig.eventSound[evt]));
  buzzerRepeating = buzzerConfig.eventRepeat[evt];

  if (buzzerRepeating && evt != BUZZER_EVT_ALARM) {
    buzzerEventStopAt = millis() + BUZZER_EVENT_TIMEOUT_MS;
  } else {
    buzzerEventStopAt = 0;
  }
}

void fireAlarm(int index, int brightnessOverride, int soundOverride) {
  if (index < 0 || index >= MAX_ALARMS) return;
  if (!ntpSyncSuccessful) return;
  if (clockOnlyDuringDimming && dimActive) {
    Serial.println(F("[ALARM] Skipped: Clock-only Dimming is active."));
    return;
  }

  alarmRinging = true;
  alarmRingingIndex = index;
  alarmPreviousDisplayMode = displayMode;
  alarmSavedDisplayOff = displayOff;
  alarmSavedBrightness = brightness;
  alarmSavedRotationEnabled = rotationEnabled;
  alarmStopAt = millis() + ALARM_TIMEOUT_MS;

  rotationEnabled = false;
  int targetBrightness = (brightnessOverride >= 0 && brightnessOverride <= 15) ? brightnessOverride : alarmConfigs[index].brightness;
  handleBrightnessChange(targetBrightness, false);

  displayMode = 8;
  forceMessageRestart = true;

  if (buzzerConfig.enabled && buzzerConfig.eventEnabled[BUZZER_EVT_ALARM]) {
    int soundToUse = (soundOverride >= 1 && soundOverride <= 3) ? soundOverride : alarmConfigs[index].sound;
    buzzerTrigger(getSoundPattern(soundToUse), true, true);
    buzzerRepeating = true;
    buzzerEventStopAt = 0;
  }

  Serial.printf(PSTR("[ALARM] Firing (slot %d).\n"), index);
}

void snoozeAlarm() {
  if (!alarmRinging || alarmRingingIndex < 0) return;
  int idx = alarmRingingIndex;
  int snoozeMinutes = alarmConfigs[idx].snoozeMinutes;
  alarmSnoozedUntil[idx] = millis() + (unsigned long)snoozeMinutes * 60000UL;
  buzzerStop();
  char message[32];
  snprintf(message, sizeof(message), "SNOOZED %d MIN", snoozeMinutes);
  showAlarmNotification(message);
  Serial.printf(PSTR("[ALARM] Slot %d snoozed for %d minutes.\n"), idx, snoozeMinutes);
}

void checkAlarmSchedule() {
  if (isRebooting) return;

  for (int i = 0; i < MAX_ALARMS; i++) {
    if (alarmSnoozedUntil[i] > 0 && millis() >= alarmSnoozedUntil[i]) {
      alarmSnoozedUntil[i] = 0;
      fireAlarm(i, -1, -1);
      return;
    }
  }

  if (!ntpSyncSuccessful) return;

  time_t nowTime = time(nullptr);
  struct tm timeinfo;
  localtime_r(&nowTime, &timeinfo);

  if (timeinfo.tm_min == alarmLastCheckedMinute) return;
  alarmLastCheckedMinute = timeinfo.tm_min;

  if (alarmRinging) return;

  for (int i = 0; i < MAX_ALARMS; i++) {
    if (!alarmConfigs[i].enabled) continue;
    if (!alarmConfigs[i].days[timeinfo.tm_wday]) continue;
    if (timeinfo.tm_hour == alarmConfigs[i].hour && timeinfo.tm_min == alarmConfigs[i].minute) {
      fireAlarm(i, -1, -1);
      return;  // first match this minute wins; others wait for their next scheduled day
    }
  }
}

void checkAlarmTimeout() {
  if (!alarmRinging || !alarmStopAt) return;
  if (millis() >= alarmStopAt) {
    Serial.println(F("[ALARM] Timed out (15 min), stopping."));
    buzzerStop();
  }
}

void setupButtons() {
  for (int i = 0; i < 4; i++) {
    if (btnCfg[i].pin >= 0) {
      pinMode(btnCfg[i].pin, INPUT_PULLUP);
      Serial.printf(PSTR("[BUTTON] Button %d ready on GPIO %d\n"), i + 1, btnCfg[i].pin);
    }
  }
}

void handleButtons() {
  static unsigned long lastPress[4] = { 0, 0, 0, 0 };
  static unsigned long pressStart[4] = { 0, 0, 0, 0 };
  static bool lastState[4] = { HIGH, HIGH, HIGH, HIGH };
  static bool longFired[4] = { false, false, false, false };

  for (int i = 0; i < 4; i++) {
    if (btnCfg[i].pin < 0) continue;
    bool cur = digitalRead(btnCfg[i].pin);

    if (cur == LOW && lastState[i] == HIGH) {  // press down
      pressStart[i] = millis();
      longFired[i] = false;
    }
    if (cur == LOW && !longFired[i]) {  // held
      if (millis() - pressStart[i] >= BTN_LONG_MS) {
        longFired[i] = true;
        if (btnCfg[i].longAct.length() > 0) {
          Serial.printf(PSTR("[BUTTON] Button %d LONG → %s\n"), i + 1, btnCfg[i].longAct.c_str());
          executeAction(btnCfg[i].longAct, "");
        }
      }
    }
    if (cur == HIGH && lastState[i] == LOW) {  // release
      if (!longFired[i] && millis() - lastPress[i] > 200) {
        lastPress[i] = millis();
        if (btnCfg[i].shortAct.length() > 0) {
          Serial.printf(PSTR("[BUTTON] Button %d SHORT → %s\n"), i + 1, btnCfg[i].shortAct.c_str());
          executeAction(btnCfg[i].shortAct, "");
        }
      }
    }
    lastState[i] = cur;
  }
}


// -----------------------------------------------------------------------------
// Main setup() and loop()
// -----------------------------------------------------------------------------
/*
DisplayMode key:
  0: Clock
  1: Weather
  2: Weather Description
  3: Countdown
  4: Bridge
  5: Date
  6: Custom Message
*/
void setup() {
  // pinMode(BUTTON_PIN, INPUT_PULLUP);  // ← Uncomment if using button

  Serial.begin(115200);
  delay(1000);
  if (!LittleFS.begin()) {
    Serial.println(F("[ERROR] LittleFS mount failed in setup! Halting."));
    while (true) {
      delay(1000);
      yield();
    }
  }
  loadUptime();
  loadPins();
  new (&P) MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
  P.begin();
  P.setCharSpacing(0);
  P.setFont(mFactory);
  loadConfig();
  setupButtons();
  buzzerHwSetup();
  P.setIntensity(brightness);
  if (displayOff) {
    P.displayShutdown(true);
  } else {
    P.displayShutdown(false);
  }
  P.setZoneEffect(0, flipDisplay, PA_FLIP_UD);
  P.setZoneEffect(0, flipDisplay, PA_FLIP_LR);

#if defined(ESP8266)
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  mConnectHandler = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected &ev) {
    Serial.println(F("[WIFI EVENT] Connected"));
  });
  mDisConnectHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &ev) {
    if (isRebooting) {
      Serial.println(F("[WIFI] Disconnect ignored during reboot."));
      return;
    }
    Serial.printf(PSTR("[WIFI EVENT] Disconnected (Reason: %d)\n"), ev.reason);
    MDNS.end();
  });
  mGotIpHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &ev) {
    Serial.printf(PSTR("[WIFI EVENT] GOT_IP - IP: %s\n"), ev.ip.toString().c_str());
    lastWifiConnectTime = millis();
    setupMDNS();
  });
#endif
  setupWebServer();
  setupTime();
  connectWiFi();
#if defined(ESP32)
  if (!isAPMode && WiFi.status() == WL_CONNECTED) {
    setupMDNS();
  }
#endif
  displayMode = 0;
  lastSwitch = millis() - (clockDuration - 500);
  lastColonBlink = millis();
  bootMillis = millis();
  saveUptime();
}

void advanceDisplayMode(bool forced) {
  if (!rotationEnabled && !forced) return;
  // Sync modeIndex to current displayMode position before going backwards
  for (int i = 0; i < MODE_COUNT; i++) {
    if (modeOrder[i] == displayMode) {
      modeIndex = i;
      break;
    }
  }
  // ---- DIMMING LOCK ----
  if (clockOnlyDuringDimming && dimActive) {
    if (displayMode != 0) {
      displayMode = 0;
      Serial.println(F("[DISPLAY] Dimming lock: Forcing Mode 0"));
    }
    return;
  }

  if (clockOnlyDuringDimming) {
    time_t now = time(nullptr);
    struct tm local_tm;
    localtime_r(&now, &local_tm);
    int curTotal = local_tm.tm_hour * 60 + local_tm.tm_min;
    int startTotal = -1, endTotal = -1;
    bool currentlyDimmed = false;

    if (autoDimmingEnabled) {
      startTotal = sunsetHour * 60 + sunsetMinute;
      endTotal = sunriseHour * 60 + sunriseMinute;
      currentlyDimmed = (startTotal < endTotal)
                          ? (curTotal >= startTotal && curTotal < endTotal)
                          : (curTotal >= startTotal || curTotal < endTotal);
    } else if (dimmingEnabled) {
      startTotal = dimStartHour * 60 + dimStartMinute;
      endTotal = dimEndHour * 60 + dimEndMinute;
      currentlyDimmed = (startTotal < endTotal)
                          ? (curTotal >= startTotal && curTotal < endTotal)
                          : (curTotal >= startTotal || curTotal < endTotal);
    }

    if (currentlyDimmed) {
      displayMode = 0;
      lastSwitch = millis();
      Serial.println(F("[DISPLAY] Staying in CLOCK (dimming active)"));
      return;
    }
  }

  // ---- RESET COUNTDOWN STATE WHEN LEAVING ----
  if (displayMode == 3) {
    countdownSegment = 0;
    segmentStartTime = 0;
  }

  prevDisplayMode = displayMode;

  // ---- SAFE ROTATION ENGINE ----
  for (int i = 0; i < MODE_COUNT; i++) {
    modeIndex++;
    if (modeIndex >= MODE_COUNT)
      modeIndex = 0;

    int nextMode = modeOrder[modeIndex];

    // --- Bridge mode throttle (YouTube, Nightscout, and RSS) ---
    if (nextMode == 4) {
      BRIDGE_SHOW_EVERY = parseBridgeShowEvery(String(ntpServer2));
      if (bridgeRotationCount % BRIDGE_SHOW_EVERY != 0) {
        bridgeRotationCount++;
        continue;
      }
      bridgeRotationCount++;
    }

    if (isModeAvailable(nextMode)) {
      if (displayMode == 6 || displayMode == 2 || displayMode == 3) {
        // P.displayReset();
        // P.displayClear();
      }

      displayMode = nextMode;

      const char *modeNames[] = { "CLOCK", "WEATHER", "WEATHER DESC", "COUNTDOWN", "BRIDGE", "DATE", "CUSTOM MESSAGE", "TIMER" };
      const char *newName = displayMode < 8 ? modeNames[displayMode] : "UNKNOWN";
      const char *prevName = prevDisplayMode < 8 ? modeNames[prevDisplayMode] : "UNKNOWN";
      Serial.printf(PSTR("[DISPLAY] Switching to display mode: %s (from %s)\n"), newName, prevName);

      clockScrollDone = false;
      descScrolling = false;
      descScrollEndTime = 0;
      lastSwitch = millis();
      return;
    }
  }

  // ---- FALLBACK ----
  displayMode = 0;
  Serial.println(F("[DISPLAY] Fallback to CLOCK"));
}

void previousDisplayMode(bool forced) {
  if (!rotationEnabled && !forced) return;
  // Sync modeIndex to current displayMode position before going backwards
  for (int i = 0; i < MODE_COUNT; i++) {
    if (modeOrder[i] == displayMode) {
      modeIndex = i;
      break;
    }
  }
  if (clockOnlyDuringDimming && dimActive) {
    displayMode = 0;
    return;
  }

  if (displayMode == 3) {
    countdownSegment = 0;
    segmentStartTime = 0;
  }

  prevDisplayMode = displayMode;

  for (int i = 0; i < MODE_COUNT; i++) {
    if (modeIndex == 0)
      modeIndex = MODE_COUNT - 1;
    else
      modeIndex--;

    int nextMode = modeOrder[modeIndex];

    if (isModeAvailable(nextMode)) {
      if (displayMode == 6 || displayMode == 2 || displayMode == 3) {
        P.displayReset();
        P.displayClear();
      }

      displayMode = nextMode;

      const char *modeNames[] = { "CLOCK", "WEATHER", "WEATHER DESC", "COUNTDOWN", "BRIDGE", "DATE", "CUSTOM MESSAGE", "TIMER" };
      const char *newName = displayMode < 8 ? modeNames[displayMode] : "UNKNOWN";
      const char *prevName = prevDisplayMode < 8 ? modeNames[prevDisplayMode] : "UNKNOWN";
      Serial.printf(PSTR("[DISPLAY] Switching to display mode: %s (from %s)\n"), newName, prevName);

      clockScrollDone = false;
      descScrolling = false;
      descScrollEndTime = 0;
      lastSwitch = millis();
      return;
    }
  }

  displayMode = 0;
  Serial.println(F("[DISPLAY] Fallback to CLOCK"));
}

bool isModeAvailable(int mode) {
  SnsType snsType = detectSnsType(String(ntpServer2));
  switch (mode) {
    case 0: return true;  // CLOCK always available
    case 1: return weatherAvailable && strlen(openWeatherApiKey) == 32 && strlen(openWeatherCity) > 0 && strlen(openWeatherCountry) > 0;
    case 2: return showWeatherDescription && weatherAvailable && weatherDescription.length() > 0;
    case 3: return countdownEnabled && !countdownFinished && ntpSyncSuccessful;
    case 4: return snsType != SNS_NTP;  // nightscout, youtube, or instagram all use mode 4
    case 5: return showDate;
    case 6: return strlen(customMessage) > 0;
  }
  return false;
}

//config save after countdown finishes
bool saveCountdownConfig(bool enabled, time_t targetTimestamp, const String &label) {
  JsonDocument doc;

  File configFile = LittleFS.open("/config.json", "r");
  if (configFile) {
    DeserializationError err = deserializeJson(doc, configFile);
    configFile.close();
    if (err) {
      Serial.print(F("[COUNTDOWN] Error parsing config.json: "));
      Serial.println(err.f_str());
      return false;
    }
  }

  JsonObject countdownObj = doc["countdown"].is<JsonObject>() ? doc["countdown"].as<JsonObject>() : doc.createNestedObject("countdown");
  countdownObj["enabled"] = enabled;
  countdownObj["targetTimestamp"] = targetTimestamp;
  countdownObj["label"] = label;
  countdownObj["isDramaticCountdown"] = isDramaticCountdown;
  doc.remove("countdownEnabled");
  doc.remove("countdownDate");
  doc.remove("countdownTime");
  doc.remove("countdownLabel");

  if (LittleFS.exists("/config.json")) {
    LittleFS.rename("/config.json", "/config.bak");
  }

  File f = LittleFS.open("/config.json", "w");
  if (!f) {
    Serial.println(F("[COUNTDOWN] ERROR: Cannot write to /config.json"));
    return false;
  }

  size_t bytesWritten = serializeJson(doc, f);
  f.close();

  Serial.printf(PSTR("[COUNTDOWN] Config updated. %u bytes written.\n"), bytesWritten);
  return true;
}

bool saveConfigRuntime() {
  if (getLargestFreeBlock() < 4000) {
    Serial.println(F("[CONFIG] Skipping auto-save: heap too fragmented, will retry next cycle"));
    return false;
  }
  JsonDocument doc;

  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[CONFIG] Failed to open config for reading"));
    return false;
  }

  DeserializationError err = deserializeJson(doc, configFile);
  configFile.close();

  if (err) {
    Serial.print(F("[CONFIG] JSON parse error: "));
    Serial.println(err.f_str());
    return false;
  }

  // Update only runtime-changing fields
  doc["brightness"] = brightness;
  doc["displayOff"] = displayOff;
  doc["lastBrightnessBeforeOff"] = lastBrightnessBeforeOff;
  doc["flipDisplay"] = flipDisplay;
  doc["twelveHourToggle"] = twelveHourToggle;
  doc["showDayOfWeek"] = showDayOfWeek;
  doc["showDate"] = showDate;
  doc["showHumidity"] = showHumidity;
  doc["colonBlinkEnabled"] = colonBlinkEnabled;
  doc["clockOnlyDuringDimming"] = clockOnlyDuringDimming;
  doc["showWeatherDescription"] = showWeatherDescription;
  doc["weatherUnits"] = weatherUnits;
  doc["weatherDuration"] = weatherDuration;
  doc["clockDuration"] = clockDuration;
  doc["timeZone"] = timeZone;
  doc["ntpServer1"] = ntpServer1;
  doc["ntpServer2"] = ntpServer2;
  doc["openWeatherApiKey"] = openWeatherApiKey;
  doc["openWeatherCity"] = openWeatherCity;
  doc["openWeatherCountry"] = openWeatherCountry;
  doc["dimmingEnabled"] = dimmingEnabled;
  doc["autoDimmingEnabled"] = autoDimmingEnabled;
  doc["dimStartHour"] = dimStartHour;
  doc["dimStartMinute"] = dimStartMinute;
  doc["dimEndHour"] = dimEndHour;
  doc["dimEndMinute"] = dimEndMinute;
  doc["dimBrightness"] = dimBrightness;
  doc["hideDonationMsg"] = hideDonationMsg;
  doc["nextDonationTime"] = (uint32_t)nextDonationTime;

  doc.remove("buzzer");
  JsonObject bz = doc.createNestedObject("buzzer");
  bz["enabled"] = buzzerConfig.enabled;
  bz["pin"] = buzzerConfig.pin;
  bz["volume"] = buzzerConfig.volume;
  JsonArray events = bz.createNestedArray("events");
  for (int i = 0; i < BUZZER_EVENT_COUNT; i++) {
    JsonObject e = events.createNestedObject();
    e["name"] = buzzerEventNames[i];
    e["enabled"] = buzzerConfig.eventEnabled[i];
    e["sound"] = buzzerConfig.eventSound[i];
    e["repeat"] = buzzerConfig.eventRepeat[i];
  }

  doc.remove("alarm");
  doc.remove("alarms");
  JsonArray alarms = doc.createNestedArray("alarms");
  for (int i = 0; i < MAX_ALARMS; i++) {
    JsonObject al = alarms.createNestedObject();
    al["enabled"] = alarmConfigs[i].enabled;
    al["hour"] = alarmConfigs[i].hour;
    al["minute"] = alarmConfigs[i].minute;
    JsonArray days = al.createNestedArray("days");
    for (int d = 0; d < 7; d++) days.add(alarmConfigs[i].days[d]);
    al["snoozeMinutes"] = alarmConfigs[i].snoozeMinutes;
    al["brightness"] = alarmConfigs[i].brightness;
    al["sound"] = alarmConfigs[i].sound;
  }

  File configFileWrite = LittleFS.open("/config.json", "w");
  if (!configFileWrite) {
    Serial.println(F("[CONFIG] Failed to open config for writing"));
    return false;
  }

  serializeJson(doc, configFileWrite);
  configFileWrite.close();

  Serial.println(F("[CONFIG] Runtime config saved"));
  return true;
}

//Custom font format for days and months
String getFormattedDateText(const char *rawText) {
  String input = String(rawText);
  String output = "";

  // 1. Detect if it's Japanese/Multi-byte
  bool isMultiByte = false;
  for (int i = 0; i < input.length(); i++) {
    if ((uint8_t)input[i] > 127) {
      isMultiByte = true;
      break;
    }
  }

  if (isMultiByte) {
    // Keep Japanese symbols as they are (e.g., "³")
    output = input;
  } else {
    // Determine the separator: \016 for custom, " " for standard
    String separator = useCustomFont ? "\016" : " ";

    // If standard font, convert to uppercase first (e.g., "tue" -> "TUE")
    if (!useCustomFont) {
      input.toUpperCase();
    }

    // 2. Inject the separator between characters
    for (int i = 0; i < input.length(); i++) {
      output += input[i];
      if (i < input.length() - 1) {
        output += separator;
      }
    }
  }

  // 3. Add the trailing spaces (M\016O\016N   or T U E  )
  output += "  ";
  return output;
}

// -----------------------------------------------------------------------------
// Donation / Encouragement Message Scheduler
// -----------------------------------------------------------------------------
const char *const DONATION_MESSAGES[3] = {
  "SUPPORTING ESPTIMECAST KEEPS THE PROJECT ALIVE",
  "SUPPORTING ESPTIMECAST MAKES FUTURE UPDATES POSSIBLE",
  "SUPPORTING ESPTIMECAST HELPS KEEP IT GROWING"
};

time_t calcNextDonationTime(bool forceTomorrow) {
  time_t now = time(nullptr);
  struct tm local_tm;
  localtime_r(&now, &local_tm);

  struct tm midnight = local_tm;
  midnight.tm_hour = 0;
  midnight.tm_min = 0;
  midnight.tm_sec = 0;
  midnight.tm_isdst = -1;
  time_t todayMidnight = mktime(&midnight);

  const int windowStart = 10 * 60;  // 10:00
  const int windowEnd = 21 * 60;    // 21:00
  int curMinOfDay = local_tm.tm_hour * 60 + local_tm.tm_min;

  if (!forceTomorrow && curMinOfDay < windowEnd) {
    int rangeStart = max(curMinOfDay + 1, windowStart);
    if (rangeStart < windowEnd) {
      int chosen = random(rangeStart, windowEnd);
      Serial.printf(PSTR("[DONATION] Scheduled today at %02d:%02d\n"), chosen / 60, chosen % 60);
      return todayMidnight + ((time_t)chosen * 60L);
    }
  }

  time_t tomorrowMidnight = todayMidnight + 86400L;
  int chosen = random(windowStart, windowEnd);
  Serial.printf(PSTR("[DONATION] Scheduled tomorrow at %02d:%02d\n"), chosen / 60, chosen % 60);
  return tomorrowMidnight + ((time_t)chosen * 60L);
}

void triggerDonationMessage() {
  if (hideDonationMsg) return;
  if (isAPMode) return;
  if (!ntpSyncSuccessful) return;
  if (isNetworkBusy) return;
  if (timerActive) return;
  if (clockOnlyDuringDimming && dimActive) return;
  if (!allowInterrupt) return;

  int idx = random(0, 3);
  String msg = String(DONATION_MESSAGES[idx]);
  msg.toCharArray(customMessage, sizeof(customMessage));

  messageScrollSpeed = 60;
  messageScrollTimes = 1;
  messageDisplaySeconds = 0;
  messageBigNumbers = false;
  allowInterrupt = true;
  displayMode = 6;
  prevDisplayMode = 0;
  messageStartTime = millis();
  currentScrollCount = 0;
  currentDisplayCycleCount = 0;
  clockScrollDone = false;
  forceMessageRestart = true;

  Serial.printf(PSTR("[DONATION] Showing message %d: %s\n"), idx, DONATION_MESSAGES[idx]);

  nextDonationTime = calcNextDonationTime(true);  // always tomorrow after firing
  saveConfigRuntime();
}

void loop() {
  handleButtons();
  buzzerLoop();
  checkAlarmSchedule();
  checkAlarmTimeout();
  if (pendingModeShiftOut) {
    pendingModeShiftOut = false;
    for (uint8_t i = 0; i < 5; i++) {
      if (flipDisplay) P.getGraphicObject()->transform(MD_MAX72XX::TSR);
      else P.getGraphicObject()->transform(MD_MAX72XX::TSL);
      delay(messageScrollSpeed);
    }
  }

  // --- WIFI RECONNECTION ---
  static unsigned long lastReconnectAttempt = 0;
  static unsigned long reconnectInterval = 5000;
  static bool wasConnected = true;
  static bool staAttemptInProgress = false;
  static unsigned long staAttemptStart = 0;
  const unsigned long staAttemptTimeout = 8000;  // short bounded window, not the full interval

  if (!isRebooting && (!isAPMode || credentialsExist())) {
    bool apClientActive = isAPMode && WiFi.softAPgetStationNum() > 0;

    if (!apClientActive) {
      if (WiFi.status() != WL_CONNECTED) {

        if (staAttemptInProgress) {
          // We're mid-attempt — either it succeeds below, or bail out on timeout
          if (millis() - staAttemptStart > staAttemptTimeout) {
            Serial.println(F("[WIFI] Background attempt timed out, dropping STA."));
            staAttemptInProgress = false;
            if (isAPMode) {
              WiFi.mode(WIFI_AP);  // explicitly release STA, back to AP-only
            }
          }
        } else if (millis() - lastReconnectAttempt > reconnectInterval) {
          lastReconnectAttempt = millis();
          Serial.printf(
            "[WIFI] Reconnecting... (next attempt in %lus)\n",
            min(reconnectInterval * 2, 300000UL) / 1000);

          if (isAPMode) {
            WiFi.mode(WIFI_AP_STA);  // explicit, and only for the attempt window
          }
          WiFi.begin(ssid, password);
          staAttemptInProgress = true;
          staAttemptStart = millis();

          if (wasConnected) {
            Serial.println(F("[WIFI] Connection lost. Will attempt reconnection..."));
            wasConnected = false;
            reconnectInterval = 5000;
          }
          reconnectInterval = min(reconnectInterval * 2, 300000UL);
        }

      } else if (!wasConnected) {
        Serial.println(F("[WIFI] Reconnected!"));
        wasConnected = true;
        staAttemptInProgress = false;
        reconnectInterval = 5000;
      }
    }
  }

  if (timerActive && (displayMode != 7 && displayMode != 6)) {
    if (!(clockOnlyDuringDimming && dimActive)) {
      displayMode = 7;
      timerSubState = 0;
      lastSwitch = millis();
      forceMessageRestart = true;
    }
  }
  // 1. REBOOT HANDLER: Execute the restart outside of the Async callback
  if (pendingRestart && (millis() - restartTimer > 2000)) {
    Serial.println(F("[SYSTEM] Rebooting now..."));
    ESP.restart();
  }
  // 2. OTA LOCK: If updating, yield to WiFi and stop everything else
  if (isUpdating) {
    yield();
    return;
  }
  if (isAPMode) {
    dnsServer.processNextRequest();
    if (credentialsExist()) {
      static unsigned long apStartTime = 0;
      if (apStartTime == 0) apStartTime = millis();  // Mark the start time once

      // 3 Minutes = 180000 ms
      if (millis() - apStartTime > 180000) {
        Serial.println(F("[WIFI] AP Timeout: Saved credentials found. Rebooting to retry connection..."));
        delay(500);
        ESP.restart();
      }
    }
    // AP Mode animation
    static unsigned long apAnimTimer = 0;
    static int apAnimFrame = 0;
    unsigned long now = millis();
    if (now - apAnimTimer > 750) {
      apAnimTimer = now;
      apAnimFrame++;
    }
    P.setTextAlignment(PA_CENTER);
    switch (apAnimFrame % 3) {
      case 0:
        {
          char buf[] = { 5, ' ', (char)169, '\0' };
          P.print(buf);
          break;
        }
      case 1:
        {
          char buf[] = { 5, ' ', (char)170, '\0' };
          P.print(buf);
          break;
        }
      case 2:
        {
          char buf[] = { 5, ' ', (char)171, '\0' };
          P.print(buf);
          break;
        }
    }
    yield();
    return;
  }

  static bool colonVisible = true;
  const unsigned long colonBlinkInterval = 800;
  if (millis() - lastColonBlink > colonBlinkInterval) {
    colonVisible = !colonVisible;
    lastColonBlink = millis();
  }

  static unsigned long ntpAnimTimer = 0;
  static int ntpAnimFrame = 0;
  static bool tzSetAfterSync = false;

  static unsigned long lastFetch = 0;
  const unsigned long fetchInterval = 300000;  // 5 minutes


  // mDNS update 8266 only
  MDNS.update();


  // -----------------------------
  // Dimming (auto + manual)
  // -----------------------------
  time_t now_time = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now_time, &timeinfo);
  int curHour = timeinfo.tm_hour;
  int curMinute = timeinfo.tm_min;
  int curTotal = curHour * 60 + curMinute;

  if (autoDimmingEnabled) {
    startTotal = sunsetHour * 60 + sunsetMinute;
    endTotal = sunriseHour * 60 + sunriseMinute;
  } else if (dimmingEnabled) {
    startTotal = dimStartHour * 60 + dimStartMinute;
    endTotal = dimEndHour * 60 + dimEndMinute;
  } else {
    startTotal = endTotal = -1;  // not used
  }

  // -----------------------------
  // Check if dimming should be active
  // -----------------------------
  if (autoDimmingEnabled || dimmingEnabled) {
    if (startTotal < endTotal) {
      dimActive = (curTotal >= startTotal && curTotal < endTotal);
    } else {
      dimActive = (curTotal >= startTotal || curTotal < endTotal);  // overnight
    }
  }

  // -----------------------------
  // Apply brightness / display on-off
  // -----------------------------
  static bool lastDimActive = false;  // remembers last state
  int targetBrightness = dimActive ? dimBrightness : brightness;

  // Log only when transitioning
  if (dimActive != lastDimActive) {
    if (dimActive) {
      if (autoDimmingEnabled)
        Serial.printf(PSTR("[DISPLAY] Automatic dimming setting brightness to %d\n"), targetBrightness);
      else if (dimmingEnabled)
        Serial.printf(PSTR("[DISPLAY] Custom dimming setting brightness to %d\n"), targetBrightness);
    } else {
      Serial.println(F("[DISPLAY] Waking display (dimming end)"));
    }
    lastDimActive = dimActive;
  }

  // Apply brightness or shutdown
  if (targetBrightness == -1) {
    if (!displayOff) {
      Serial.println(F("[DISPLAY] Turning display OFF (dimming -1)"));
      P.displayShutdown(true);
      P.displayClear();
      displayOff = true;
      displayOffByDimming = dimActive;
      displayOffByBrightness = !dimActive;
    }
  } else {
    if (displayOff && ((dimActive && displayOffByBrightness) || (!dimActive && displayOffByDimming))) {
      P.displayShutdown(false);
      displayOff = false;
      displayOffByDimming = false;
      displayOffByBrightness = false;
    }
    P.setIntensity(targetBrightness);
  }

  // Enforce "Clock only during dimming" if enabled
  if (clockOnlyDuringDimming && dimActive) {
    if (displayMode != 0) {
      prevDisplayMode = displayMode;
      displayMode = 0;
      lastSwitch = millis();
      Serial.println(F("[DISPLAY] Forcing CLOCK because 'Clock only during dimming' is enabled and dimming is active."));
    }
  }

  // --- IMMEDIATE COUNTDOWN FINISH TRIGGER ---
  if (countdownEnabled && !countdownFinished && ntpSyncSuccessful && countdownTargetTimestamp > 0 && now_time >= countdownTargetTimestamp) {
    countdownFinished = true;
    displayMode = 3;  // Let main loop handle animation + TIMES UP
    countdownShowFinishedMessage = true;
    hourglassPlayed = false;
    countdownFinishedMessageStartTime = millis();

    Serial.println(F("[SYSTEM] Countdown target reached! Switching to Mode 3 to display finish sequence."));
    buzzerFireEvent(BUZZER_EVT_COUNTDOWN);
    yield();
  }


  // --- IP Display ---
  if (showingIp) {
    if (P.displayAnimate()) {
      ipDisplayCount++;
      if (ipDisplayCount < ipDisplayMax) {
        textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);
        P.displayScroll(pendingIpToShow.c_str(), PA_CENTER, actualScrollDirection, 120);
      } else {
        showingIp = false;
        P.displayClear();
        delay(500);  // Blocking delay as in working copy
        displayMode = 0;
        lastSwitch = millis();
      }
    }
    yield();
    return;  // Exit loop early if showing IP
  }


  // --- BRIGHTNESS/OFF CHECK ---
  if (brightness == -1) {
    if (!displayOff) {
      Serial.println(F("[DISPLAY] Turning display OFF"));
      P.displayShutdown(true);  // fully off
      P.displayClear();
      displayOff = true;
    }
    yield();
  }


  // --- NTP State Machine ---
  switch (ntpState) {
    case NTP_IDLE: break;
    case NTP_SYNCING:
      {
        time_t now = time(nullptr);
        if (now > 1000) {  // NTP sync successful
          Serial.println(F("[TIME] NTP sync successful."));
          ntpSyncSuccessful = true;
          ntpState = NTP_SUCCESS;
        } else if (millis() - ntpStartTime > ntpTimeout || ntpRetryCount >= maxNtpRetries) {
          Serial.println(F("[TIME] NTP sync failed."));
          ntpSyncSuccessful = false;
          ntpState = NTP_FAILED;
        } else {
          // Periodically print a more descriptive status message
          if (millis() - lastNtpStatusPrintTime >= ntpStatusPrintInterval) {
            Serial.printf(PSTR("[TIME] NTP sync in progress (attempt %d of %d)...\n"), ntpRetryCount + 1, maxNtpRetries);
            lastNtpStatusPrintTime = millis();
          }
          // Still increment ntpRetryCount based on your original timing for the timeout logic
          // (even if you don't print a dot for every increment)
          if (millis() - ntpStartTime > ((unsigned long)(ntpRetryCount + 1) * 1000UL)) {
            ntpRetryCount++;
          }
        }
        break;
      }
    case NTP_SUCCESS:
      if (!tzSetAfterSync) {
        const char *posixTz = ianaToPosix(timeZone);
        setenv("TZ", posixTz, 1);
        tzset();
        tzSetAfterSync = true;
        // Schedule donation time now that we know the real local time
        if (!hideDonationMsg && nextDonationTime == 0) {
          nextDonationTime = calcNextDonationTime(donationFirstBoot);
          saveConfigRuntime();
          Serial.println(F("[DONATION] NTP synced. First donation time scheduled."));
        }
      }
      ntpAnimTimer = 0;
      ntpAnimFrame = 0;
      break;

    case NTP_FAILED:
      ntpAnimTimer = 0;
      ntpAnimFrame = 0;

      static unsigned long lastNtpRetryAttempt = 0;
      static bool firstRetry = true;

      if (lastNtpRetryAttempt == 0) {
        lastNtpRetryAttempt = millis();  // set baseline on first fail
      }

      unsigned long ntpRetryInterval = firstRetry ? 30000UL : 300000UL;  // first retry after 30s, after that every 5 minutes

      if (millis() - lastNtpRetryAttempt > ntpRetryInterval) {
        lastNtpRetryAttempt = millis();
        ntpRetryCount = 0;
        ntpStartTime = millis();
        ntpState = NTP_SYNCING;
        Serial.println(F("[TIME] Retrying NTP sync..."));

        firstRetry = false;
      }
      break;
  }


  // Only advance mode by timer for clock/weather, not description!
  unsigned long displayDuration = (displayMode == 0) ? clockDuration : weatherDuration;
  if (rotationEnabled && (displayMode == 0 || displayMode == 1) && millis() - lastSwitch > displayDuration) {
    advanceDisplayMode();
  }

  // --- DONATION MESSAGE CHECK ---
  if (!hideDonationMsg && ntpSyncSuccessful && !isAPMode && nextDonationTime > 0) {
    time_t now_d = time(nullptr);
    if (now_d >= nextDonationTime) {
      triggerDonationMessage();
    }
  }

  // --- MODIFIED WEATHER FETCHING LOGIC ---
  if (WiFi.status() == WL_CONNECTED) {
    if (!weatherFetchInitiated || shouldFetchWeatherNow || (millis() - lastFetch > fetchInterval)) {
      if (shouldFetchWeatherNow) {
        Serial.println(F("[LOOP] Immediate weather fetch requested by web server."));
        shouldFetchWeatherNow = false;
      } else if (!weatherFetchInitiated) {
        Serial.println(F("[LOOP] Initial weather fetch."));
      } else {
        Serial.println(F("[LOOP] Regular interval weather fetch."));
      }
      weatherFetchInitiated = true;
      weatherFetched = false;
      fetchWeather();
      lastFetch = millis();
    }
  } else {
    weatherFetchInitiated = false;
    shouldFetchWeatherNow = false;
  }

  // --- NIGHTSCOUT FETCH TIMER ---
  SnsType snsTypeLoop = detectSnsType(String(ntpServer2));
  if (snsTypeLoop == SNS_NIGHTSCOUT && WiFi.status() == WL_CONNECTED && ntpSyncSuccessful) {
    if (millis() < nightscoutBackoffUntil) {
      // backoff active (404 bad URL or 429 rate limit) — skip fetch entirely
    } else if (currentGlucose == -1 || millis() - lastNightscoutFetchTime >= NIGHTSCOUT_FETCH_INTERVAL) {
      fetchNightscout();
      lastNightscoutFetchTime = millis();
    }
  }

  // --- SNS (YouTube / Instagram / RSS) FETCH TIMER ---
  if ((snsTypeLoop == SNS_YOUTUBE || snsTypeLoop == SNS_INSTAGRAM || snsTypeLoop == SNS_RSS) && WiFi.status() == WL_CONNECTED) {

    bool instagramThrottleRetryDue =
      snsTypeLoop == SNS_INSTAGRAM && instagramThrottleRetryAt != 0 && (int32_t)(millis() - instagramThrottleRetryAt) >= 0;

    if (snsTypeLoop == SNS_YOUTUBE && !isNetworkBusy && (lastYoutubeFetchTime == 0 || millis() - lastYoutubeFetchTime >= YOUTUBE_FETCH_INTERVAL)) {

      isNetworkBusy = true;

      // Grab the user input from the stored variable, stripping ESPTimeCast params first
      String rawUrl = stripUrlParam(String(ntpServer2), "show_every");
      String targetId = "";

      // Check if the input contains an "@" handle
      int atIndex = rawUrl.indexOf("@");
      if (atIndex != -1) {
        // Extract everything from the "@" to the end (e.g., "@linustech")
        targetId = rawUrl.substring(atIndex);
      }
      // Check if it's a traditional channel URL
      else if (rawUrl.indexOf("channel/") != -1) {
        int channelIdx = rawUrl.indexOf("channel/");
        targetId = rawUrl.substring(channelIdx + 8);
      }
      // Fallback: assume they pasted the raw ID directly
      else {
        targetId = rawUrl;
      }

      // Send the extracted ID or Handle to the PHP bridge
      String bridgeUrl = "http://esptimecast.com/youtube-bridge.php?id=" + targetId;
      Serial.print(F("[YOUTUBE] Fetching via PHP bridge: "));
      Serial.println(bridgeUrl);

      WiFiClient client;
      HTTPClient http;
      http.begin(client, bridgeUrl);
      http.setUserAgent("ESPTimeCast-Firmware");
      http.setTimeout(4000);

      int httpCode = http.GET();
      if (httpCode == 200) {
        String payload = http.getString();
        payload.trim();

        // Find the key inside the JSON payload
        int subKeyIdx = payload.indexOf("\"subscribers\":");
        if (subKeyIdx != -1) {
          // Cut the string starting right after '"subscribers":'
          String subValueStr = payload.substring(subKeyIdx + 14);

          // Remove the closing brace '}' if any, and convert to integer
          subValueStr.replace("}", "");
          subValueStr.trim();

          long parsedSubs = subValueStr.toInt();
          if (parsedSubs >= 0) {
            youtubeSubscribers = parsedSubs;
            Serial.printf(PSTR("[YOUTUBE] Subscribers fetched from JSON: %ld\n"), youtubeSubscribers);
          } else {
            Serial.println(F("[YOUTUBE] Bridge JSON reported an error count (-1)"));
          }
        } else {
          Serial.println(F("[YOUTUBE] Failed to find 'subscribers' key in JSON payload"));
        }
      } else {
        Serial.printf(PSTR("[YOUTUBE] HTTP failed! Code: %d, Message: %s\n"), httpCode, http.errorToString(httpCode).c_str());
      }

      http.end();
      isNetworkBusy = false;
      lastYoutubeFetchTime = millis();
    }

    if (snsTypeLoop == SNS_INSTAGRAM && !isNetworkBusy && (instagramThrottleRetryDue || lastInstagramFetchTime == 0 || millis() - lastInstagramFetchTime >= INSTAGRAM_FETCH_INTERVAL)) {

      isNetworkBusy = true;

      // Grab the user input from the stored variable, stripping ESPTimeCast params first
      String rawUrl = stripUrlParam(String(ntpServer2), "show_every");
      String targetUsername = "";

      // Pull the username out of an instagram.com/<username> URL if present
      int igIdx = rawUrl.indexOf("instagram.com/");
      if (igIdx != -1) {
        targetUsername = rawUrl.substring(igIdx + 14);
      } else {
        // Fallback: assume they pasted the raw username (optionally with a leading @)
        targetUsername = rawUrl;
      }

      if (targetUsername.startsWith("@")) {
        targetUsername = targetUsername.substring(1);
      }

      // Trim off anything after the username itself
      int slashIdx = targetUsername.indexOf('/');
      if (slashIdx != -1) targetUsername = targetUsername.substring(0, slashIdx);

      int qIdx = targetUsername.indexOf('?');
      if (qIdx != -1) targetUsername = targetUsername.substring(0, qIdx);

      targetUsername.trim();

      String bridgeUrl =
        "http://esptimecast.com/instagram-bridge.php?username=" + targetUsername;

      Serial.print(F("[INSTAGRAM] Fetching via PHP bridge: "));
      Serial.println(bridgeUrl);

      WiFiClient client;
      HTTPClient http;
      http.begin(client, bridgeUrl);
      http.setUserAgent("ESPTimeCast-Firmware");
      http.setTimeout(4000);

      int httpCode = http.GET();
      String payload = http.getString();
      payload.trim();

      if (httpCode == 429 && payload.indexOf("\"status\":\"throttled\"") != -1) {

        unsigned long retryDelay =
          INSTAGRAM_THROTTLE_RETRY_MIN_MS + random(INSTAGRAM_THROTTLE_RETRY_JITTER_MS + 1);

        instagramThrottleRetryAt = millis() + retryDelay;

        Serial.printf(
          "[INSTAGRAM] Bridge throttled; retrying in %.1f seconds.\n",
          retryDelay / 1000.0);

      } else if (httpCode == 200) {
        // A real response clears a previous short throttled retry schedule.
        instagramThrottleRetryAt = 0;

        int folKeyIdx = payload.indexOf("\"followers\":");
        if (folKeyIdx != -1) {
          String folValueStr = payload.substring(folKeyIdx + 12);
          folValueStr.replace("}", "");
          folValueStr.trim();

          long parsedFollowers = folValueStr.toInt();
          if (parsedFollowers >= 0) {
            instagramFollowers = parsedFollowers;
            Serial.printf(
              "[INSTAGRAM] Followers fetched from JSON: %ld\n",
              instagramFollowers);
          } else {
            Serial.println(
              "[INSTAGRAM] Bridge JSON reported an error/not-found count (-1)");
          }
        } else {
          Serial.println(
            "[INSTAGRAM] Failed to find 'followers' key in JSON payload");
        }

      } else {
        // Keep the normal hourly retry behavior for blocked and ordinary failures.
        instagramThrottleRetryAt = 0;

        Serial.printf(
          "[INSTAGRAM] HTTP failed! Code: %d, Message: %s\n",
          httpCode,
          http.errorToString(httpCode).c_str());
      }

      http.end();
      isNetworkBusy = false;
      lastInstagramFetchTime = millis();
    }

    if (snsTypeLoop == SNS_RSS && !isNetworkBusy && (lastRssFetchTime == 0 || millis() - lastRssFetchTime >= RSS_FETCH_INTERVAL)) {
      isNetworkBusy = true;

      auto urlEncode = [](String str) -> String {
        String encoded = "";
        for (int i = 0; i < str.length(); i++) {
          char c = str.charAt(i);
          if (isalnum(c)) {
            encoded += c;
          } else {
            char code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
            char c2 = (c >> 4) & 0xf;
            char code0 = c2 + '0';
            if (c2 > 9) code0 = c2 - 10 + 'A';
            encoded += '%';
            encoded += code0;
            encoded += code1;
          }
        }
        return encoded;
      };

      // Strip show_every=N before passing to bridge (value already in BRIDGE_SHOW_EVERY via advanceDisplayMode)
      String feedUrl = stripUrlParam(String(ntpServer2), "show_every");

      String bridgeUrl = "http://esptimecast.com/rss-bridge.php?url=" + urlEncode(feedUrl);
      Serial.print(F("[RSS] Fetching via PHP bridge: "));
      Serial.println(bridgeUrl);

      WiFiClient client;
      HTTPClient http;
      http.begin(client, bridgeUrl);
      http.setUserAgent("ESPTimeCast-Firmware");
      http.setTimeout(4000);

      int httpCode = http.GET();
      if (httpCode == 200) {
        String payload = http.getString();
        payload.trim();
        bool isError = (payload == "RSS ERROR" || payload == "INVALID RSS" || payload == "NO ENTRY" || payload == "FORBIDDEN" || payload == "NO URL" || payload == "INVALID URL");
        if (!isError && payload.length() > 0) {
          rssTitle = payload;
          Serial.print(F("[RSS] Title fetched: "));
          Serial.println(rssTitle);
        } else {
          Serial.print(F("[RSS] Bridge returned error: "));
          Serial.println(payload);
        }
      } else {
        Serial.printf(PSTR("[RSS] HTTP failed! Code: %d\n"), httpCode);
      }

      http.end();
      isNetworkBusy = false;
      lastRssFetchTime = millis();
    }
  }

  const char *const *daysOfTheWeek = getDaysOfWeek(language);
  // Call our new formatting function
  String daySymbol = getFormattedDateText(daysOfTheWeek[timeinfo.tm_wday]);

  // build base HH:MM first ---
  char baseTime[24];
  if (twelveHourToggle) {
    int hour12 = timeinfo.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    sprintf(baseTime, "%d:%02d", hour12, timeinfo.tm_min);
  } else {
    sprintf(baseTime, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }

  // add seconds only if colon blink enabled AND weekday hidden ---
  char timeWithSeconds[12];
  if (!showDayOfWeek && colonBlinkEnabled) {
    // Remove any leading space from baseTime
    const char *trimmedBase = baseTime;
    if (baseTime[0] == ' ') trimmedBase++;  // skip leading space
    sprintf(timeWithSeconds, "%s:%02d", trimmedBase, timeinfo.tm_sec);
  } else if (!showDayOfWeek && !colonBlinkEnabled) {
    sprintf(timeWithSeconds, "  %s  ", baseTime);
  } else {
    strcpy(timeWithSeconds, baseTime);  // no seconds
  }

  // keep spacing logic the same ---
  char timeSpacedStr[24];
  int j = 0;
  for (int i = 0; timeWithSeconds[i] != '\0'; i++) {
    timeSpacedStr[j++] = timeWithSeconds[i];
    if (timeWithSeconds[i + 1] != '\0') {
      timeSpacedStr[j++] = ' ';
    }
  }
  timeSpacedStr[j] = '\0';

  // build final string ---
  String formattedTime;
  if (showDayOfWeek) {
    // daySymbol now has either "t\016u\016e  " or "T U E  "
    // In both cases, the padding is already inside daySymbol.
    formattedTime = daySymbol + String(timeSpacedStr);
  } else {
    formattedTime = String(timeSpacedStr);
  }

  unsigned long currentDisplayDuration = 0;
  if (displayMode == 0) {
    currentDisplayDuration = clockDuration;
  } else if (displayMode == 1) {  // Weather
    currentDisplayDuration = weatherDuration;
  }

  // Only advance mode by timer for clock/weather static (Mode 0 & 1).
  // Other modes (2, 3) have their own internal timers/conditions for advancement.
  if (rotationEnabled && (displayMode == 0 || displayMode == 1) && (millis() - lastSwitch > currentDisplayDuration)) {
    advanceDisplayMode();
  }


  // --- CLOCK Display Mode ---
  if (displayMode == 0) {
    if (forceMessageRestart) {
      P.displayReset();
      P.displayClear();
      forceMessageRestart = false;
      clockScrollDone = false;  // Ensure it scrolls in
    }
    if (forceMessageRestart) return;
    P.setCharSpacing(0);

    // --- NTP SYNC ---
    if (ntpState == NTP_SYNCING) {
      P.setTextAlignment(PA_CENTER);
      if (ntpSyncSuccessful || ntpRetryCount >= maxNtpRetries || millis() - ntpStartTime > ntpTimeout) {
        ntpState = NTP_FAILED;
      } else if (millis() - ntpAnimTimer > 750) {
        if (forceMessageRestart) return;
        ntpAnimTimer = millis();
        switch (ntpAnimFrame % 3) {
          case 0:
            {
              String s = F("S Y N C ");
              s += char(174);
              P.print(s.c_str());
              break;
            }
          case 1:
            {
              String s = F("S Y N C ");
              s += char(175);
              P.print(s.c_str());
              break;
            }
          case 2:
            {
              String s = F("S Y N C ");
              s += char(186);
              P.print(s.c_str());
              break;
            }
        }
        ntpAnimFrame++;
      }
    }
    // --- NTP / WEATHER ERROR ---
    else if (!ntpSyncSuccessful) {
      if (forceMessageRestart) return;
      P.setTextAlignment(PA_CENTER);
      static unsigned long errorAltTimer = 0;
      static bool showNtpError = true;

      if (!ntpSyncSuccessful && !weatherAvailable) {
        if (millis() - errorAltTimer > 2000) {
          errorAltTimer = millis();
          showNtpError = !showNtpError;
        }
        if (showNtpError) {
          P.write(2);  // NTP error glyph
        } else {
          P.write(1);  // Weather error glyph
        }

      } else if (!ntpSyncSuccessful) {
        P.write(2);
      } else if (!weatherAvailable) {
        P.write(1);
      }
    }
    // --- DISPLAY CLOCK ---
    else {
      String timeString = formattedTime;
      if (showDayOfWeek && colonBlinkEnabled && !colonVisible) {
        timeString.replace(":", " ");
      }

      // --- SCROLL IN ONLY WHEN COMING FROM SPECIFIC MODES OR FIRST BOOT ---
      bool shouldScrollIn = false;
      if (prevDisplayMode == -1 || prevDisplayMode == 3 || prevDisplayMode == 4) {
        shouldScrollIn = true;  // first boot or other special modes
      } else if (prevDisplayMode == 2 && weatherDescription.length() > 8) {
        shouldScrollIn = true;  // only scroll in if weather was scrolling
      } else if (prevDisplayMode == 6) {
        shouldScrollIn = true;  // scroll in when coming from custom message
      }

      if (shouldScrollIn && !clockScrollDone) {
        textEffect_t inDir = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);

        P.displayText(
          timeString.c_str(),
          PA_CENTER,
          GENERAL_SCROLL_SPEED,
          0,
          inDir,
          PA_NO_EFFECT);
        while (!P.displayAnimate()) {
          if (displayMode != 0) {
            clockScrollDone = false;
            return;
          }
          if (forceMessageRestart) {
            clockScrollDone = false;
            return;
          }
          handleButtons();
          buzzerLoop();
          yield();
        }
        // Only if we finish the while loop naturally do we mark it done
        clockScrollDone = true;
      } else {
        P.setTextAlignment(PA_CENTER);
        P.print(timeString);
      }
    }

    yield();
  } else {
    // --- leaving clock mode ---
    if (prevDisplayMode == 0) {
      clockScrollDone = false;  // reset for next time we enter clock
    }
  }


  // --- WEATHER Display Mode ---
  static bool weatherWasAvailable = false;
  if (displayMode == 1) {
    if (forceMessageRestart) return;
    P.setCharSpacing(1);
    P.setTextAlignment(PA_CENTER);
    if (weatherAvailable) {
      String weatherDisplay;
      if (showHumidity && currentHumidity != -1) {
        int cappedHumidity = (currentHumidity > 99) ? 99 : currentHumidity;
        weatherDisplay = currentTemp + " " + String(cappedHumidity) + "%";
      } else {
        weatherDisplay = currentTemp + tempSymbol;
      }
      P.print(weatherDisplay.c_str());
      weatherWasAvailable = true;
    } else {
      if (weatherWasAvailable) {
        Serial.println(F("[DISPLAY] Weather not available, showing clock..."));
        weatherWasAvailable = false;
      }
      if (ntpSyncSuccessful) {
        String timeString = formattedTime;
        if (!colonVisible) timeString.replace(":", " ");
        P.setCharSpacing(0);
        P.print(timeString);
      } else {
        P.setCharSpacing(0);
        P.setTextAlignment(PA_CENTER);
        P.write(1);
      }
    }
    yield();
    return;
  }


  // --- WEATHER DESCRIPTION Display Mode ---
  if (displayMode == 2 && showWeatherDescription && weatherAvailable && weatherDescription.length() > 0) {
    P.setCharSpacing(1);
    P.setTextAlignment(PA_CENTER);
    if (forceMessageRestart) return;
    String desc = weatherDescription;

    // --- Check if humidity is actually visible ---
    bool humidityVisible = showHumidity && weatherAvailable && strlen(openWeatherApiKey) == 32 && strlen(openWeatherCity) > 0 && strlen(openWeatherCountry) > 0;

    // --- Conditional padding ---
    bool addPadding = false;
    if (prevDisplayMode == 1 && humidityVisible) {
      addPadding = true;
    }
    if (addPadding) {
      desc = "    " + desc;  // 4-space padding before scrolling
    }

    // prepare safe buffer
    static char descBuffer[128];  // large enough for OWM translations
    desc.toCharArray(descBuffer, sizeof(descBuffer));

    if (desc.length() > 8) {
      if (!descScrolling) {
        textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);
        P.displayScroll(descBuffer, PA_CENTER, actualScrollDirection, GENERAL_SCROLL_SPEED);
        descScrolling = true;
        descScrollEndTime = 0;  // reset end time at start
      }
      if (displayMode != 2) return;
      if (P.displayAnimate()) {
        if (descScrollEndTime == 0) {
          descScrollEndTime = millis();  // mark the time when scroll finishes
        }
        // wait small pause after scroll stops
        if (millis() - descScrollEndTime > descriptionScrollPause) {
          if (forceMessageRestart) return;
          descScrolling = false;
          descScrollEndTime = 0;
          advanceDisplayMode();
        }
      } else {
        descScrollEndTime = 0;  // reset if not finished
      }
      yield();
      return;
    } else {
      if (descStartTime == 0) {
        P.setTextAlignment(PA_CENTER);
        P.setCharSpacing(1);
        P.print(descBuffer);
        descStartTime = millis();
      }
      if (millis() - descStartTime > descriptionDuration) {
        descStartTime = 0;
        advanceDisplayMode();
      }
      if (forceMessageRestart) return;
      yield();
      return;
    }
  }


  // --- Countdown Display Mode ---
  if (displayMode == 3 && countdownEnabled && ntpSyncSuccessful) {
    if (forceMessageRestart) return;
    const unsigned long SEGMENT_DISPLAY_DURATION = 1500;  // 1.5 seconds for each static segment

    long timeRemaining = countdownTargetTimestamp - now_time;

    // --- Countdown Finished Logic ---
    // This part of the code remains unchanged.
    if (timeRemaining <= 0 || countdownShowFinishedMessage) {
      // NEW: Only show "TIMES UP" if countdown target timestamp is valid and expired
      time_t now = time(nullptr);
      if (countdownTargetTimestamp == 0 || countdownTargetTimestamp > now) {
        // Target invalid or in the future, don't show "TIMES UP" yet, advance display instead
        countdownShowFinishedMessage = false;
        countdownFinished = false;
        countdownFinishedMessageStartTime = 0;
        hourglassPlayed = false;  // Reset if we decide not to show it
        Serial.println(F("[COUNTDOWN-FINISH] Countdown target invalid or not reached yet, skipping 'TIMES UP'. Advancing display."));
        advanceDisplayMode();
        yield();
        return;
      }

      // Define these static variables here if they are not global (or already defined in your loop())
      static const char *flashFrames[] = { "\x08", "\x09" };
      static unsigned long lastFlashingSwitch = 0;
      static int flashingMessageFrame = 0;

      // This 'if' runs ONLY ONCE when the "finished" sequence begins.
      if (!hourglassPlayed) {                          // <-- This is the single entry point for the combined sequence
        countdownFinished = true;                      // Mark as finished overall
        countdownShowFinishedMessage = true;           // Confirm we are in the finished sequence
        countdownFinishedMessageStartTime = millis();  // Start the 15-second timer for the flashing duration

        // Initialize Flashing "TIMES UP" for its very first frame
        flashingMessageFrame = 0;
        lastFlashingSwitch = millis();  // Set initial time for first flash frame
        P.setTextAlignment(PA_CENTER);
        P.setCharSpacing(0);
        P.print(flashFrames[flashingMessageFrame]);             // Display the first frame immediately
        flashingMessageFrame = (flashingMessageFrame + 1) % 2;  // Prepare for the next frame

        hourglassPlayed = true;  // <-- Mark that this initial combined sequence has completed!
        countdownSegment = 0;    // Reset segment counter after finished sequence initiation
        segmentStartTime = 0;    // Reset segment timer after finished sequence initiation
      }

      // --- Continue Flashing "TIMES UP" for its duration (after initial combined sequence) ---
      // This part runs in subsequent loop iterations after the hourglass has played.
      const unsigned long timesUpFlashDuration =
        (buzzerConfig.enabled && buzzerConfig.eventEnabled[BUZZER_EVT_COUNTDOWN]) ? 5000 : 15000;
      if (millis() - countdownFinishedMessageStartTime < timesUpFlashDuration) {  // Flashing duration
        if (displayMode != 3) return;
        if (forceMessageRestart) return;
        if (millis() - lastFlashingSwitch >= 500) {  // Check for flashing interval
          lastFlashingSwitch = millis();
          P.displayClear();
          P.setTextAlignment(PA_CENTER);
          P.setCharSpacing(0);
          P.print(flashFrames[flashingMessageFrame]);
          flashingMessageFrame = (flashingMessageFrame + 1) % 2;
        }
        P.displayAnimate();  // Ensure display updates
        yield();
        return;  // Stay in this mode until the 15 seconds are over
      } else {
        // 15 seconds are over, clean up and advance
        Serial.println(F("[COUNTDOWN-FINISH] Flashing duration over. Advancing to Clock."));
        buzzerStop();
        countdownShowFinishedMessage = false;
        countdownFinishedMessageStartTime = 0;
        hourglassPlayed = false;  // <-- RESET this flag for the next countdown cycle!

        // Final cleanup (persisted)
        countdownEnabled = false;
        countdownTargetTimestamp = 0;
        countdownLabel[0] = '\0';
        saveCountdownConfig(false, 0, "");

        P.displayClear();
        P.setInvert(false);
        displayMode = 0;
        prevDisplayMode = 6;
        clockScrollDone = false;
        forceMessageRestart = true;
        lastSwitch = millis();
        yield();
        return;  // Exit loop after processing
      }
    }  // END of 'if (timeRemaining <= 0 || countdownShowFinishedMessage)'


    // --- NORMAL COUNTDOWN LOGIC ---
    // This 'else' block will only run if `timeRemaining > 0` and `!countdownShowFinishedMessage`
    else {

      // The new variable `isDramaticCountdown` toggles between the two modes
      if (isDramaticCountdown) {
        // --- EXISTING DRAMATIC COUNTDOWN LOGIC ---
        long days = timeRemaining / (24 * 3600);
        long hours = (timeRemaining % (24 * 3600)) / 3600;
        long minutes = (timeRemaining % 3600) / 60;
        long seconds = timeRemaining % 60;
        String currentSegmentText = "";

        if (segmentStartTime == 0 || (millis() - segmentStartTime > SEGMENT_DISPLAY_DURATION)) {
          segmentStartTime = millis();
          P.displayClear();

          switch (countdownSegment) {
            case 0:  // Days
              if (days > 0) {
                currentSegmentText = String(days) + " " + (days == 1 ? "DAY" : "DAYS");
                Serial.printf(PSTR("[COUNTDOWN-STATIC] Displaying segment %d: %s\n"), countdownSegment, currentSegmentText.c_str());
                countdownSegment++;
              } else {
                // Skip days if zero
                countdownSegment++;
                segmentStartTime = 0;
              }
              break;
            case 1:
              {  // Hours
                char buf[10];
                sprintf(buf, "%02ld HRS", hours);  // pad hours with 0
                currentSegmentText = String(buf);
                Serial.printf(PSTR("[COUNTDOWN-STATIC] Displaying segment %d: %s\n"), countdownSegment, currentSegmentText.c_str());
                countdownSegment++;
                break;
              }
            case 2:
              {  // Minutes
                char buf[10];
                sprintf(buf, "%02ld MINS", minutes);  // pad minutes with 0
                currentSegmentText = String(buf);
                Serial.printf(PSTR("[COUNTDOWN-STATIC] Displaying segment %d: %s\n"), countdownSegment, currentSegmentText.c_str());
                countdownSegment++;
                break;
              }
            case 3:
              {  // Seconds & Label Scroll
                time_t segmentNow = time(nullptr);
                unsigned long segmentStartMillis = millis();

                long nowRemaining = countdownTargetTimestamp - segmentNow;
                long currentSecond = nowRemaining % 60;
                char secondsBuf[10];
                sprintf(secondsBuf, "%02ld %s", currentSecond, currentSecond == 1 ? "SEC" : "SECS");
                String secondsText = String(secondsBuf);
                Serial.printf(PSTR("[COUNTDOWN-STATIC] Displaying segment 3: %s\n"), secondsText.c_str());
                P.displayClear();
                P.setTextAlignment(PA_CENTER);
                P.setCharSpacing(1);
                P.print(secondsText.c_str());
                delay(SEGMENT_DISPLAY_DURATION - 400);

                unsigned long elapsed = millis() - segmentStartMillis;
                long adjustedSecond = (countdownTargetTimestamp - segmentNow - (elapsed / 1000)) % 60;
                sprintf(secondsBuf, "%02ld %s", adjustedSecond, adjustedSecond == 1 ? "SEC" : "SECS");
                secondsText = String(secondsBuf);
                P.displayClear();
                P.setTextAlignment(PA_CENTER);
                P.setCharSpacing(1);
                P.print(secondsText.c_str());
                delay(400);

                String label;
                if (strlen(countdownLabel) > 0) {
                  label = String(countdownLabel);
                  label.trim();
                  if (!label.startsWith("TO:") && !label.startsWith("to:")) {
                    label = "TO: " + label;
                  }
                  label.replace('.', ',');
                } else {
                  static const char *fallbackLabels[] = {
                    "TO: PARTY TIME!", "TO: SHOWTIME!", "TO: CLOCKOUT!", "TO: BLASTOFF!",
                    "TO: GO TIME!", "TO: LIFTOFF!", "TO: THE BIG REVEAL!",
                    "TO: ZERO HOUR!", "TO: THE FINAL COUNT!", "TO: MISSION COMPLETE"
                  };
                  int randomIndex = random(0, 10);
                  label = fallbackLabels[randomIndex];
                }

                P.setTextAlignment(PA_LEFT);
                P.setCharSpacing(1);
                textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);
                P.displayScroll(label.c_str(), PA_LEFT, actualScrollDirection, GENERAL_SCROLL_SPEED);

                while (!P.displayAnimate()) {
                  if (displayMode != 3) return;
                  if (forceMessageRestart) return;
                  handleButtons();
                  buzzerLoop();
                  yield();
                }
                countdownSegment++;
                segmentStartTime = millis();
                break;
              }
            case 4:  // Exit countdown
              Serial.println(F("[COUNTDOWN-STATIC] All segments and label displayed. Advancing to Clock."));
              countdownSegment = 0;
              segmentStartTime = 0;
              P.setTextAlignment(PA_CENTER);
              P.setCharSpacing(1);
              advanceDisplayMode();
              yield();
              return;

            default:
              Serial.println(F("[COUNTDOWN-ERROR] Invalid countdownSegment, resetting."));
              countdownSegment = 0;
              segmentStartTime = 0;
              break;
          }

          if (currentSegmentText.length() > 0) {
            P.setTextAlignment(PA_CENTER);
            P.setCharSpacing(1);
            P.print(currentSegmentText.c_str());
          }
        }
        P.displayAnimate();
      }

      // --- NEW: SINGLE-LINE COUNTDOWN LOGIC ---
      else {
        long days = timeRemaining / (24 * 3600);
        long hours = (timeRemaining % (24 * 3600)) / 3600;
        long minutes = (timeRemaining % 3600) / 60;
        long seconds = timeRemaining % 60;

        String label;
        // Check if countdownLabel is empty and grab a random one if needed
        if (strlen(countdownLabel) > 0) {
          label = String(countdownLabel);
          label.trim();

          // Replace standard digits 0–9 with your custom font character codes
          for (int i = 0; i < label.length(); i++) {
            if (isDigit(label[i])) {
              int num = label[i] - '0';           // 0–9
              label[i] = 145 + ((num + 9) % 10);  // Maps 0→154, 1→145, ... 9→153
            }
          }

        } else {
          static const char *fallbackLabels[] = {
            "PARTY TIME", "SHOWTIME", "CLOCKOUT", "BLASTOFF",
            "GO TIME", "LIFTOFF", "THE BIG REVEAL",
            "ZERO HOUR", "THE FINAL COUNT", "MISSION COMPLETE"
          };
          int randomIndex = random(0, 10);
          label = fallbackLabels[randomIndex];
        }

        // Format the full string
        char buf[50];
        // Only show days if there are any, otherwise start with hours
        if (days > 0) {
          sprintf(buf, "%s IN: %ldD %02ldH %02ldM %02ldS", label.c_str(), days, hours, minutes, seconds);
        } else {
          sprintf(buf, "%s IN: %02ldH %02ldM %02ldS", label.c_str(), hours, minutes, seconds);
        }

        String fullString = String(buf);
        bool addPadding = false;
        bool humidityVisible = showHumidity && weatherAvailable && strlen(openWeatherApiKey) == 32 && strlen(openWeatherCity) > 0 && strlen(openWeatherCountry) > 0;

        // Padding logic
        if (prevDisplayMode == 0 && (showDayOfWeek || colonBlinkEnabled)) {
          addPadding = true;
        } else if (prevDisplayMode == 1 && humidityVisible) {
          addPadding = true;
        }
        if (addPadding) {
          fullString = "    " + fullString;  // 4 spaces
        }

        // Display the full string and scroll it
        P.setTextAlignment(PA_LEFT);
        P.setCharSpacing(1);
        textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);
        P.displayScroll(fullString.c_str(), PA_LEFT, actualScrollDirection, GENERAL_SCROLL_SPEED);

        // Blocking loop to ensure the full message scrolls
        while (!P.displayAnimate()) {
          if (displayMode != 3) return;
          if (forceMessageRestart) break;
          handleButtons();
          buzzerLoop();
          yield();
        }

        // After scrolling is complete, we're done with this display mode
        // Move to the next mode and exit the function.
        P.setTextAlignment(PA_CENTER);
        advanceDisplayMode();
        yield();
        return;
      }
    }

    // Keep alignment reset just in case
    P.setTextAlignment(PA_CENTER);
    P.setCharSpacing(1);
    yield();
    return;
  }  // End of if (displayMode == 3 && ...)


  // --- BRIDGE Display Mode ---
  if (displayMode == 4) {
    if (forceMessageRestart) return;
    SnsType snsType = detectSnsType(String(ntpServer2));

    if (snsType == SNS_YOUTUBE || snsType == SNS_INSTAGRAM) {
      long count = (snsType == SNS_YOUTUBE) ? youtubeSubscribers : instagramFollowers;
      char icon = (snsType == SNS_YOUTUBE) ? 157 : 155;

      if (count < 0) {
        P.setTextAlignment(PA_CENTER);
        P.setCharSpacing(0);

        String displayText = "";
        displayText += icon;
        displayText += " - - ";

        P.print(displayText.c_str());

        unsigned long snsStart = millis();
        while (millis() - snsStart < weatherDuration) {
          if (displayMode != 4) return;
          if (forceMessageRestart) return;
          handleButtons();
          buzzerLoop();
          yield();
        }

        advanceDisplayMode();
        return;
      }

      String countStr;

      if (count < 10000) {
        countStr = String(count);
      } else if (count < 100000) {
        float v = count / 1000.0f;
        countStr = (v == (int)v) ? String((int)v) : String(v, 1);
        countStr += char(193);  // K
      } else if (count < 1000000) {
        countStr = String(count / 1000);
        countStr += char(193);  // K
      } else if (count < 100000000) {
        float v = count / 1000000.0f;
        countStr = (v == (int)v) ? String((int)v) : String(v, 1);
        countStr += char(192);  // M
      } else {
        countStr = String(count / 1000000);
        countStr += char(192);  // M
      }

      if (countStr.length() <= 5) {
        // --- STATIC: icon + up to 5 digits fits the 32px display ---
        P.setTextAlignment(PA_CENTER);
        P.setCharSpacing(0);  // explicit 2px gap below, not auto-spacing
        String iconStr = String(icon) + " ";
        // 1. Manually add a 1px space between each digit of the number
        String spacedCountStr = "";
        for (unsigned int i = 0; i < countStr.length(); i++) {
          spacedCountStr += countStr[i];
          if (i < countStr.length() - 1) {
            spacedCountStr += " ";  // Inject a 1px font space between digits
          }
        }
        String endpaddedCount = String(spacedCountStr) + " ";
        String displayText = String(iconStr) + " " + endpaddedCount;
        P.print(displayText.c_str());

        unsigned long snsStart = millis();
        while (millis() - snsStart < weatherDuration) {
          if (displayMode != 4) return;
          if (forceMessageRestart) return;
          handleButtons();
          buzzerLoop();
          yield();
        }
        advanceDisplayMode();
        return;
      }

      // --- SCROLL: more than 5 digits, same technique as Custom Message ---
      P.setTextAlignment(PA_LEFT);
      P.setCharSpacing(0);  // explicit 2px gap below, not auto-spacing
      String iconStr = String(icon) + " ";
      // 1. Manually add a 1px space between each digit of the number
      String spacedCountStr = "";
      for (unsigned int i = 0; i < countStr.length(); i++) {
        spacedCountStr += countStr[i];
        if (i < countStr.length() - 1) {
          spacedCountStr += " ";  // Inject a 1px font space between digits
        }
      }
      String scrollText = String(iconStr) + " " + spacedCountStr;
      textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);
      P.displayScroll(scrollText.c_str(), PA_LEFT, actualScrollDirection, GENERAL_SCROLL_SPEED);

      while (!P.displayAnimate()) {
        if (displayMode != 4) return;
        if (forceMessageRestart) return;
        handleButtons();
        buzzerLoop();
        yield();
      }

      advanceDisplayMode();
      return;
    }

    // --- RSS display ---
    if (snsType == SNS_RSS) {
      P.setCharSpacing(1);
      char charIcon = 194;
      String rssIcon = String(charIcon) + " ";

      if (rssTitle.length() == 0) {
        // Not yet fetched
        P.setTextAlignment(PA_CENTER);
        P.setCharSpacing(0);
        String waitText = String(rssIcon) + " - -";
        P.print(waitText.c_str());
        unsigned long rssStart = millis();
        while (millis() - rssStart < weatherDuration) {
          if (displayMode != 4) return;
          if (forceMessageRestart) return;
          handleButtons();
          buzzerLoop();
          yield();
        }
        advanceDisplayMode();
        return;
      }

      // Title is ready — remap digits to small font glyphs, then scroll
      String rssDisplay = rssTitle;
      for (int i = 0; i < rssDisplay.length(); i++) {
        if (isDigit(rssDisplay[i])) {
          int num = rssDisplay[i] - '0';
          rssDisplay[i] = 145 + ((num + 9) % 10);
        }
      }

      textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);

      // Padding logic (same as weather scroll)
      bool addPadding = false;
      bool humidityVisible = showHumidity && weatherAvailable && strlen(openWeatherApiKey) == 32 && strlen(openWeatherCity) > 0 && strlen(openWeatherCountry) > 0;
      if (prevDisplayMode == 0 && (showDayOfWeek || colonBlinkEnabled)) {
        addPadding = true;
      } else if (prevDisplayMode == 1 && humidityVisible) {
        addPadding = true;
      }
      String scrollText = String(rssIcon) + rssDisplay;
      if (addPadding) {
        scrollText = "   " + scrollText;  // 3 spaces
      }
      P.setTextAlignment(PA_LEFT);
      P.setCharSpacing(1);
      P.displayScroll(scrollText.c_str(), PA_LEFT, actualScrollDirection, RSS_SCROLL_SPEED);
      while (!P.displayAnimate()) {
        if (displayMode != 4) return;
        if (forceMessageRestart) return;
        handleButtons();
        buzzerLoop();
        yield();
      }
      advanceDisplayMode();
      return;
    }

    P.setCharSpacing(1);

    if (currentGlucose != -1) {
      time_t nowUTC = time(nullptr);

      bool isOutdated = false;
      int ageMinutes = 0;

      if (lastGlucoseTime > 0) {
        double diffSec = difftime(nowUTC, lastGlucoseTime);
        ageMinutes = (int)(diffSec / 60.0);
        isOutdated = (ageMinutes > NIGHTSCOUT_IDLE_THRESHOLD_MIN);
        Serial.printf(PSTR("[NIGHTSCOUT] Data age: %d minutes old (threshold: %d)\n"), ageMinutes, NIGHTSCOUT_IDLE_THRESHOLD_MIN);
      }

      char arrow;
      if (currentDirection == "Flat") arrow = 139;
      else if (currentDirection == "SingleUp") arrow = 134;
      else if (currentDirection == "DoubleUp") arrow = 135;
      else if (currentDirection == "SingleDown") arrow = 136;
      else if (currentDirection == "DoubleDown") arrow = 137;
      else if (currentDirection == "FortyFiveUp") arrow = 138;
      else if (currentDirection == "FortyFiveDown") arrow = 140;
      else arrow = '?';

      // --- Build glucose display string ---
      String glucoseDisplay;
      if (nightscoutMmol) {
        char tmp[8];
        dtostrf(currentGlucose / 18.018f, 4, 1, tmp);
        glucoseDisplay = String(tmp);
        glucoseDisplay.trim();
      } else {
        glucoseDisplay = String(currentGlucose);
      }

      String displayText = "";
      if (isOutdated) {
        // First pass: convert digits to dimmed variants, dot to char(206)
        String styledStr = "";
        for (int i = 0; i < glucoseDisplay.length(); i++) {
          char c = glucoseDisplay[i];
          if (c == '.') {
            styledStr += char(206);  // mid-line dot for decimal point
          } else if (isDigit(c)) {
            int num = c - '0';
            styledStr += char(195 + ((num + 9) % 10));  // dimmed digit
          } else {
            styledStr += c;
          }
        }

        // Second pass: wrap and separate every character with char(205)
        // Result: (205)(char)(205)(char)(205)...(char)(205)
        String separatedStr = "";
        separatedStr += char(205);  // leading cap
        for (int i = 0; i < styledStr.length(); i++) {
          separatedStr += styledStr[i];
          separatedStr += char(205);  // after every character including last
        }

        displayText += separatedStr;
        displayText += " ";
        displayText += arrow;
        P.setCharSpacing(0);
      } else {
        displayText += glucoseDisplay + String(arrow);
        P.setCharSpacing(1);
      }

      P.setTextAlignment(PA_CENTER);
      P.print(displayText.c_str());
      unsigned long nightscoutStart = millis();
      while (millis() - nightscoutStart < weatherDuration) {
        if (displayMode != 4) return;
        if (forceMessageRestart) return;
        handleButtons();
        buzzerLoop();
        yield();
      }
      advanceDisplayMode();
      return;
    } else {
      P.setTextAlignment(PA_CENTER);
      P.setCharSpacing(0);
      P.write(15);
      unsigned long errorStart = millis();
      while (millis() - errorStart < 2000) {
        if (displayMode != 4) return;
        if (forceMessageRestart) return;
        handleButtons();
        buzzerLoop();
        yield();
      }
      advanceDisplayMode();
      return;
    }
  }


  // DATE Display Mode
  else if (displayMode == 5 && showDate) {
    if (forceMessageRestart) return;

    if (timeinfo.tm_year < 120 || timeinfo.tm_mday <= 0 || timeinfo.tm_mon < 0 || timeinfo.tm_mon > 11) {
      advanceDisplayMode();
      return;
    }

    // 1. Month uses the custom font logic (lowercase + \016)
    const char *const *months = getMonthsOfYear(language);
    String monthAbbr = getFormattedDateText(months[timeinfo.tm_mon]);

    // 2. Day digits ALWAYS use standard spaces (" "), never the custom \016
    String dayString = String(timeinfo.tm_mday);
    String spacedDay = "";
    for (size_t i = 0; i < dayString.length(); i++) {
      spacedDay += dayString[i];
      if (i < dayString.length() - 1) {
        spacedDay += " ";  // Hardcoded standard space
      }
    }

    String dateString;
    String langStr = String(language);

    if (langStr == "ja") {
      // Japanese: "1 ²  2 4 ±"
      dateString = monthAbbr + spacedDay + " \xB1";
    } else {
      auto isDayFirst = [](const String &lang) {
        const char *dayFirstLangs[] = { "af", "cs", "da", "de", "eo", "es", "et", "fi", "fr", "ga", "hr", "hu", "it", "lt", "lv", "nl", "no", "pl", "pt", "ro", "ru", "sk", "sl", "sr", "sv", "sw", "tr" };
        for (auto lf : dayFirstLangs) {
          if (lang.equalsIgnoreCase(lf)) return true;
        }
        return false;
      };

      // monthAbbr already has trailing "  " from your function
      if (isDayFirst(langStr)) {
        // Result: "2 4  f\016e\016b  "
        dateString = spacedDay + "  " + monthAbbr;
      } else {
        // Result: "f\016e\016b  2 4"
        dateString = monthAbbr + spacedDay;
      }
    }

    P.setTextAlignment(PA_CENTER);
    P.setCharSpacing(0);
    P.print(dateString.c_str());

    if (millis() - lastSwitch > weatherDuration) {
      advanceDisplayMode();
    }
  }


  // --- Custom Message Display Mode (displayMode == 6) ---
  if (displayMode == 6) {
    totalPixelWidth = 0;

    if (forceMessageRestart) {
      P.displayReset();
      P.displayClear();

      // RESET TIMERS/COUNTERS so new messages start fresh
      messageStartTime = millis();
      currentScrollCount = 0;
      currentDisplayCycleCount = 0;

      forceMessageRestart = false;
    }

    if (strlen(customMessage) == 0) {
      advanceDisplayMode();
      yield();
      return;
    }

    String msg = String(customMessage);

    // --- Strip brackets around numeric tokens ONLY ---
    if (messageBigNumbers) {
      while (true) {
        int start = msg.indexOf('[');
        int end = msg.indexOf(']', start);

        if (start == -1 || end == -1) break;

        String inside = msg.substring(start + 1, end);

        bool isNumber = true;
        for (char c : inside) {
          if (!isdigit(c)) {
            isNumber = false;
            break;
          }
        }

        if (isNumber) {
          msg.remove(end, 1);
          msg.remove(start, 1);
        } else {
          break;  // leave icon tokens like [MOON]
        }
      }
    }

    replaceIconTokens(msg, totalPixelWidth);

    if (!messageBigNumbers) {
      for (int i = 0; i < msg.length(); i++) {
        if (isDigit(msg[i])) {
          int num = msg[i] - '0';
          msg[i] = 145 + ((num + 9) % 10);
        }
      }
    }

    // --- TIMEOUT & LIMIT CHECKS ---
    bool timedOut = (messageDisplaySeconds > 0 && (millis() - messageStartTime) >= (messageDisplaySeconds * 1000UL));
    bool scrollsComplete = (messageScrollTimes > 0 && currentScrollCount >= messageScrollTimes);
    bool cyclesComplete = (messageScrollTimes > 0 && currentDisplayCycleCount >= messageScrollTimes);

    if (timedOut || scrollsComplete || cyclesComplete) {
      allowInterrupt = true;
      if (strlen(lastPersistentMessage) > 0) {
        strncpy(customMessage, lastPersistentMessage, sizeof(customMessage));
        messageScrollSpeed = GENERAL_SCROLL_SPEED;
      } else {
        customMessage[0] = '\0';
      }
      currentScrollCount = 0;
      messageStartTime = 0;
      currentDisplayCycleCount = 0;
      messageDisplaySeconds = 0;
      messageScrollTimes = 0;
      prevDisplayMode = 6;  // Set for Clock scroll-in
      advanceDisplayMode();
      yield();
      return;
    }

    // --- BRANCH A: STATIC (0-32 pixels) ---
    if (totalPixelWidth <= 32) {
      unsigned long durationMs = (messageDisplaySeconds > 0) ? (messageDisplaySeconds * 1000UL) : weatherDuration;

      // 1. Initial Centered Display
      P.setTextAlignment(PA_CENTER);
      P.setCharSpacing(1);
      P.print(msg.c_str());

      unsigned long displayUntil = millis() + durationMs;
      while (millis() < displayUntil) {
        if (displayMode != 6) return;
        if (forceMessageRestart) return;
        yield();
      }

      // 2. THE MANUAL SHIFT (Create 4-5px of pure black)
      // We only want to "push" the text off-screen if this is the VERY LAST cycle
      bool isLastCycle = (messageScrollTimes > 0 && (currentDisplayCycleCount + 1 >= messageScrollTimes))
                         || (messageScrollTimes == 0);

      if (totalPixelWidth >= 27 && isLastCycle && rotationEnabled) {
        // Shift the internal pixel buffer 5 times
        for (uint8_t i = 0; i < 5; i++) {
          if (displayMode != 6) return;
          if (flipDisplay) {
            P.getGraphicObject()->transform(MD_MAX72XX::TSR);  // shift right
          } else {
            P.getGraphicObject()->transform(MD_MAX72XX::TSL);  // shift left
          }
          delay(messageScrollSpeed);
        }
      }

      // 3. Handover to Clock
      if (messageScrollTimes > 0) {
        currentDisplayCycleCount++;
      } else {
        advanceDisplayMode();
        prevDisplayMode = 6;
        clockScrollDone = false;
      }
      yield();
      return;
    }

    // --- BRANCH B: SCROLLING ---
    bool addPadding = false;
    bool humidityVisible = showHumidity && weatherAvailable && strlen(openWeatherApiKey) == 32 && strlen(openWeatherCity) > 0 && strlen(openWeatherCountry) > 0;
    if (prevDisplayMode == 0 && (showDayOfWeek || colonBlinkEnabled)) addPadding = true;
    else if (prevDisplayMode == 1 && humidityVisible) addPadding = true;

    if (addPadding) msg = "    " + msg;

    P.setTextAlignment(PA_LEFT);
    P.setCharSpacing(1);
    textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);

    P.displayScroll(msg.c_str(), PA_LEFT, actualScrollDirection, messageScrollSpeed);

    while (!P.displayAnimate()) {
      if (displayMode != 6) return;
      if (forceMessageRestart) return;  // Exit immediately to top level
      handleButtons();
      buzzerLoop();
      yield();
    }

    currentScrollCount++;

    if (messageDisplaySeconds == 0 && messageScrollTimes == 0) {
      prevDisplayMode = 6;
      advanceDisplayMode();
    }
    yield();
    return;
  }

  if (displayMode == 7) {
    showTimerMode7();
  }

  if (displayMode == 8) {
    showAlarmMode();
  }

  unsigned long currentMillis = millis();
  unsigned long runtimeSeconds = (currentMillis - bootMillis) / 1000;
  unsigned long currentTotal = totalUptimeSeconds + runtimeSeconds;

  // --- Log and save uptime every 10 minutes ---
  const unsigned long uptimeLogInterval = 600000UL;  // 10 minutes in ms

  // ---- CONFIG AUTO SAVE ----
  if (configDirty && !alarmRinging && millis() - lastBrightnessChange > saveDelay) {
    saveConfigRuntime();
    configDirty = false;
    Serial.println(F("[CONFIG] Auto-saved"));
  }

  if (currentMillis - lastUptimeLog >= uptimeLogInterval) {
    lastUptimeLog = currentMillis;
    Serial.printf(PSTR("[UPTIME] Runtime: %s (total %.2f hours)\n"),
                  formatUptime(currentTotal).c_str(), currentTotal / 3600.0);
    saveUptime();  // Save accumulated uptime every 10 minutes
  }
  yield();
}

char getPomodoroWorkIcon() {
  switch (pomodoroSession) {
    case 1: return '\xBC';  // quarter
    case 2: return '\xBD';  // half
    case 3: return '\xBE';  // three quarters
    case 4: return '\x83';  // full
    default: return '\xBC';
  }
}

void showTimerMode7() {
  unsigned long now = millis();
  P.setCharSpacing(1);

  // --- 1. INTERRUPT LOGIC ---
  if (allowInterrupt == false) {
    unsigned long waitTime = (unsigned long)clockDuration;
    if (now - lastSwitch >= waitTime) {
      Serial.println(F("[TIMER] clockDuration reached. Switching to Mode 6 (Infinite)"));
      displayMode = 6;
      lastSwitch = now;
      return;
    }
  }

  // --- 2. STOPWATCH / POMODORO BREAK ---
  if (isStopwatch) {
    unsigned long elapsed = timerPaused ? timerRemainingAtPause : (millis() - timerEndTime);

    if (isPomodoroActive) {
      unsigned long currentBreakMs = (pomodoroSession == 4) ? pomodoroLongBreakMs : pomodoroBreakMs;
      if (!timerPaused && elapsed >= currentBreakMs) {
        pomodoroSession++;
        if (pomodoroSession > 4) pomodoroSession = 1;
        pomodoroInBreak = false;
        isStopwatch = false;
        timerOriginalDuration = pomodoroWorkMs;
        timerEndTime = millis() + pomodoroWorkMs;
        timerFinished = false;
        timerPaused = false;
        Serial.printf(PSTR("[POMODORO] Break over. Starting session %d.\n"), pomodoroSession);
        buzzerFireEvent(BUZZER_EVT_POMODORO_BREAK);
        return;
      }
      unsigned long remaining = (currentBreakMs > elapsed) ? (currentBreakMs - elapsed) : 0;
      unsigned long totalSec = remaining / 1000;
      int m = totalSec / 60;
      int s = totalSec % 60;
      char buf[10];
      sprintf(buf, "%c %02d:%02d", '\xBF', m, s);
      P.setTextAlignment(PA_CENTER);
      P.print(buf);
      return;
    }

    // Normal stopwatch with centiseconds
    unsigned long totalSec = elapsed / 1000;
    int cs = (elapsed % 1000) / 10;
    int m = totalSec / 60;
    int s = totalSec % 60;
    char buf[12];
    if (m > 59) {
      int h = m / 60;
      m = m % 60;
      sprintf(buf, "%02d:%02d:%02d", h, m, s);
    } else {
      sprintf(buf, "%02d:%02d.%02d", m, s, cs);
    }
    P.setTextAlignment(PA_CENTER);
    P.print(buf);
    return;
  }

  // --- 3. COUNTDOWN TIMER ---
  if (!timerFinished) {
    long remaining = 0;
    if (timerPaused) {
      remaining = (long)(timerRemainingAtPause / 1000);
    } else {
      if (now >= timerEndTime) {
        timerFinished = true;
        timerFinishStartTime = now;
        if (!isPomodoroActive) {
          buzzerFireEvent(BUZZER_EVT_TIMER);
        }
      } else {
        remaining = (long)((timerEndTime - now) / 1000);
        if (remaining < 0) remaining = 0;

        int h = remaining / 3600;
        int m = (remaining % 3600) / 60;
        int s = remaining % 60;

        char buf[12];
        if (isPomodoroActive) {
          char icon = getPomodoroWorkIcon();
          if (h > 0) sprintf(buf, "%c %02d:%02d:%02d", icon, h, m, s);
          else sprintf(buf, "%c %02d:%02d", icon, m, s);
        } else {
          if (h > 0) sprintf(buf, "%02d:%02d:%02d", h, m, s);
          else sprintf(buf, "%02d:%02d", m, s);
        }

        P.setTextAlignment(PA_CENTER);
        P.print(buf);
        return;
      }
    }

    if (timerPaused) {
      int h = remaining / 3600;
      int m = (remaining % 3600) / 60;
      int s = remaining % 60;
      char buf[12];
      if (isPomodoroActive) {
        char icon = getPomodoroWorkIcon();
        if (h > 0) sprintf(buf, "%c %02d:%02d:%02d", icon, h, m, s);
        else sprintf(buf, "%c %02d:%02d", icon, m, s);
      } else {
        if (h > 0) sprintf(buf, "%02d:%02d:%02d", h, m, s);
        else sprintf(buf, "%02d:%02d", m, s);
      }
      P.setTextAlignment(PA_CENTER);
      P.print(buf);
      return;
    }
  }

  // --- 4. FINISHED STATE ---
  if (timerFinished) {
    if (isPomodoroActive) {
      pomodoroInBreak = true;
      isStopwatch = true;
      // Session 4 gets the long break
      timerEndTime = millis();
      timerActive = true;
      timerPaused = false;
      timerFinished = false;
      Serial.printf(PSTR("[POMODORO] Session %d done. Starting %s break.\n"),
                    pomodoroSession, pomodoroSession == 4 ? "long" : "short");
      buzzerFireEvent(BUZZER_EVT_POMODORO_WORK);
      return;
    }

    // Normal timer: alarm animation for 5 seconds
    if (now - timerFinishStartTime > 5000) {
      buzzerStop();
      timerActive = false;
      timerFinished = false;
      displayMode = 0;
      prevDisplayMode = 6;
      clockScrollDone = false;
      forceMessageRestart = true;
      lastSwitch = millis();
      return;
    }
    if ((now / 500) % 2 == 0) P.print("\x08");
    else P.print("\x09");
  }
}

void showAlarmMode() {
  if (!alarmRinging) return;

  static bool alarmInvert = false;
  static unsigned long alarmLastInvertSwitch = 0;
  const unsigned long alarmInvertInterval = 500;

  if (millis() - alarmLastInvertSwitch >= alarmInvertInterval) {
    alarmInvert = !alarmInvert;
    alarmLastInvertSwitch = millis();

    struct tm timeinfo;
    time_t nowTime = time(nullptr);
    localtime_r(&nowTime, &timeinfo);

    char buf[9];
    strftime(buf, sizeof(buf), twelveHourToggle ? "%I:%M" : "%H:%M", &timeinfo);

    String timeString = String(buf);
    if (alarmInvert) {
      timeString.replace(":", " ");
    }

    P.setTextAlignment(PA_CENTER);
    P.setCharSpacing(1);
    P.setInvert(alarmInvert);
    P.print(timeString.c_str());
  }
}