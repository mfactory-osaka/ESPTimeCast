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

#include "mfactoryfont.h"   // Custom font
#include "tz_lookup.h"      // Timezone lookup, do not duplicate mapping here!
#include "days_lookup.h"    // Languages for the Days of the Week
#include "months_lookup.h"  // Languages for the Months of the Year
#include "index_html.h"     // Web UI

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 14   //D5
#define CS_PIN 13    //D7
#define DATA_PIN 15  //D8

#ifdef ESP8266
WiFiEventHandler mConnectHandler;
WiFiEventHandler mDisConnectHandler;
WiFiEventHandler mGotIpHandler;
#endif

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
AsyncWebServer server(80);

// --- Global Scroll Speed Settings ---
const int GENERAL_SCROLL_SPEED = 85;  // Default: Adjust this for Weather Description and Countdown Label (e.g., 50 for faster, 200 for slower)
const int IP_SCROLL_SPEED = 115;      // Default: Adjust this for the IP Address display (slower for readability)
int messageScrollSpeed = 85;          // default fallback

// --- Nightscout setting ---
const unsigned int NIGHTSCOUT_IDLE_THRESHOLD_MIN = 10;  // minutes before data is considered outdated

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

// Timing and display settings
unsigned long clockDuration = 10000;
unsigned long weatherDuration = 5000;
bool displayOff = false;
int brightness = 7;
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

//Countdown Globals - NEW
bool countdownEnabled = false;
time_t countdownTargetTimestamp = 0;  // Unix timestamp
char countdownLabel[64] = "";         // Label for the countdown
bool isDramaticCountdown = true;      // Default to the dramatic countdown mode

// Runtime Uptime Tracker
unsigned long bootMillis = 0;                      // Stores millis() at boot
unsigned long lastUptimeLog = 0;                   // Timer for hourly logging
const unsigned long uptimeLogInterval = 600000UL;  // 10 minutes in ms
unsigned long totalUptimeSeconds = 0;              // Persistent accumulated uptime in seconds

// State management
bool weatherCycleStarted = false;
WiFiClient client;
const byte DNS_PORT = 53;
DNSServer dnsServer;

String currentTemp = "";
String weatherDescription = "";
bool showWeatherDescription = false;
bool weatherAvailable = false;
bool weatherFetched = false;
bool weatherFetchInitiated = false;
bool isAPMode = false;
char tempSymbol = '[';
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

// --- NEW GLOBAL VARIABLES FOR IMMEDIATE COUNTDOWN FINISH ---
bool countdownFinished = false;                       // Tracks if the countdown has permanently finished
bool countdownShowFinishedMessage = false;            // Flag to indicate "TIMES UP" message is active
unsigned long countdownFinishedMessageStartTime = 0;  // Timer for the 10-second message duration
unsigned long lastFlashToggleTime = 0;                // For controlling the flashing speed
bool currentInvertState = false;                      // Current state of display inversion for flashing
static bool hourglassPlayed = false;

// Weather Description Mode handling
unsigned long descStartTime = 0;  // For static description
bool descScrolling = false;
const unsigned long descriptionDuration = 3000;    // 3s for short text
static unsigned long descScrollEndTime = 0;        // for post-scroll delay (re-used for scroll timing)
const unsigned long descriptionScrollPause = 300;  // 300ms pause after scroll

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
    DynamicJsonDocument doc(1024);
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

    File f = LittleFS.open("/config.json", "w");
    if (f) {
      serializeJsonPretty(doc, f);
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

  DynamicJsonDocument doc(1024);  // Size based on ArduinoJson Assistant + buffer
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print(F("[ERROR] JSON parse failed during load: "));
    Serial.println(error.f_str());
    return;
  }

  bool configChanged = false;

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
    tempSymbol = ']';
  else
    tempSymbol = '[';


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

  // --- Save migrated config if needed ---
  if (configChanged) {
    Serial.println(F("[CONFIG] Saving migrated config.json"));

    File f = LittleFS.open("/config.json", "w");
    if (f) {
      serializeJsonPretty(doc, f);
      f.close();
      Serial.println(F("[CONFIG] Migration saved successfully."));
    } else {
      Serial.println(F("[ERROR] Failed to save migrated config.json"));
    }
  }

  Serial.println(F("[CONFIG] Configuration loaded."));
}


// -----------------------------------------------------------------------------
// WiFi Setup
// -----------------------------------------------------------------------------
const char *DEFAULT_AP_PASSWORD = "12345678";
const char *AP_SSID = "ESPTimeCast";

void connectWiFi() {
  Serial.println(F("[WIFI] Connecting to WiFi..."));

  bool credentialsExist = (strlen(ssid) > 0);

  if (!credentialsExist) {
    Serial.println(F("[WIFI] No saved credentials. Starting AP mode directly."));
    WiFi.mode(WIFI_AP);
    WiFi.disconnect(true);
    delay(100);

    if (strlen(DEFAULT_AP_PASSWORD) < 8) {
      WiFi.softAP(AP_SSID);
      Serial.println(F("[WIFI] AP Mode started (no password, too short)."));
    } else {
      WiFi.softAP(AP_SSID, DEFAULT_AP_PASSWORD);
      Serial.println(F("[WIFI] AP Mode started."));
    }

    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    Serial.print(F("[WIFI] AP IP address: "));
    Serial.println(WiFi.softAPIP());
    isAPMode = true;

    WiFiMode_t mode = WiFi.getMode();
    Serial.printf("[WIFI] WiFi mode after setting AP: %s\n",
                  mode == WIFI_OFF ? "OFF" : mode == WIFI_STA    ? "STA ONLY"
                                           : mode == WIFI_AP     ? "AP ONLY"
                                           : mode == WIFI_AP_STA ? "AP + STA (Error!)"
                                                                 : "UNKNOWN");

    Serial.println(F("[WIFI] AP Mode Started"));
    return;
  }

  // If credentials exist, attempt STA connection
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  const unsigned long timeout = 30000;
  unsigned long animTimer = 0;
  int animFrame = 0;
  bool animating = true;

  while (animating) {
    unsigned long now = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("[WIFI] Connected: ") + WiFi.localIP().toString());
      isAPMode = false;

      WiFiMode_t mode = WiFi.getMode();
      Serial.printf("[WIFI] WiFi mode after STA connection: %s\n",
                    mode == WIFI_OFF ? "OFF" : mode == WIFI_STA    ? "STA ONLY"
                                             : mode == WIFI_AP     ? "AP ONLY"
                                             : mode == WIFI_AP_STA ? "AP + STA (Error!)"
                                                                   : "UNKNOWN");

      // --- IP Display initiation ---
      pendingIpToShow = WiFi.localIP().toString();

      // Replace all dots with your custom font code 184
      for (int i = 0; i < pendingIpToShow.length(); i++) {
        if (pendingIpToShow[i] == '.') {
          pendingIpToShow[i] = 184;
        }
      }

      showingIp = true;
      ipDisplayCount = 0;  // Reset count for IP display
      P.displayClear();
      P.setCharSpacing(1);  // Set spacing for IP scroll
      textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);
      P.displayScroll(pendingIpToShow.c_str(), PA_CENTER, actualScrollDirection, IP_SCROLL_SPEED);
      // --- END IP Display initiation ---

      animating = false;  // Exit the connection loop
      break;
    } else if (now - startAttemptTime >= timeout) {
      Serial.println(F("[WIFI] Failed. Starting AP mode..."));
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID, DEFAULT_AP_PASSWORD);
      Serial.print(F("[WIFI] AP IP address: "));
      Serial.println(WiFi.softAPIP());
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      isAPMode = true;

      WiFiMode_t mode = WiFi.getMode();
      Serial.printf("[WIFI] WiFi mode after STA failure and setting AP: %s\n",
                    mode == WIFI_OFF ? "OFF" : mode == WIFI_STA    ? "STA ONLY"
                                             : mode == WIFI_AP     ? "AP ONLY"
                                             : mode == WIFI_AP_STA ? "AP + STA (Error!)"
                                                                   : "UNKNOWN");

      animating = false;
      Serial.println(F("[WIFI] AP Mode Started"));
      break;
    }
    if (now - animTimer > 750) {
      animTimer = now;
      P.setTextAlignment(PA_CENTER);
      switch (animFrame % 3) {
        case 0: P.print(F("# ©")); break;
        case 1: P.print(F("# ª")); break;
        case 2: P.print(F("# «")); break;
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
  const char *hostName = "esptimecast";  // your device name
  MDNS.end();
  bool mdnsStarted = false;
  mdnsStarted = MDNS.begin(hostName);

  if (mdnsStarted) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[WIFI] mDNS started: http://%s.local\n", hostName);
  } else {
    Serial.println("[WIFI] mDNS failed to start");
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

  // ---------------------------------------------------------------------------
  // DIMMING SECTION
  // ---------------------------------------------------------------------------
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
      Serial.printf("Automatic Dimming Schedule: Sunrise: %02d:%02d → Sunset: %02d:%02d\n",
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

      Serial.printf("Current Auto-Dimming Status: %s\n", autoActive ? "ACTIVE" : "Inactive");
      Serial.printf("Dimming Brightness (night): %d\n", dimBrightness);
    }
  } else {
    // --- Manual (Custom Schedule) dimming mode ---
    Serial.printf("Custom Dimming Schedule: %02d:%02d → %02d:%02d\n",
                  dimStartHour, dimStartMinute, dimEndHour, dimEndMinute);
    Serial.printf("Dimming Brightness: %d\n", dimBrightness);
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

  Serial.print(F("Total Runtime: "));
  if (getTotalRuntimeSeconds() > 0) {
    Serial.println(formatTotalRuntime());
  } else {
    Serial.println(F("No runtime recorded yet."));
  }

  Serial.println(F("========================================"));
  Serial.println();
}


// -----------------------------------------------------------------------------
// Web Server and Captive Portal
// -----------------------------------------------------------------------------
void handleCaptivePortal(AsyncWebServerRequest *request);

void setupWebServer() {
  Serial.println(F("[WEBSERVER] Setting up web server..."));

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[WEBSERVER] Request: /"));
    // Create a response from LittleFS file so we can attach cache-control headers
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html", "text/html");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    request->send(response);
  });

  server.on("/generate_204", HTTP_GET, handleCaptivePortal);         // Android
  server.on("/fwlink", HTTP_GET, handleCaptivePortal);               // Windows
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortal);  // iOS/macOS
  server.on("/ncsi.txt", HTTP_GET, handleCaptivePortal);             // Windows NCSI (variation)
  server.on("/cp/success.txt", HTTP_GET, handleCaptivePortal);       // Android/Generic Success Check
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(204);  // 204 No Content response
  });
  server.on("/apple-touch-icon.png", HTTP_GET, [](AsyncWebServerRequest *request) {  // iOS icon check
    request->send(204);
  });
  server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) {  // Android short probe (already in handleCaptivePortal, but safe to also silence if somehow missed)
    request->send(204);
  });
  server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest *request) {  // iOS/macOS generic check
    request->send(204);
  });
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {  // Windows NCSI check
    request->send(204);
  });
  server.on("/msdownload/update/v3/static/trustedr/en/disallowedcertstl.cab", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(204);
  });
  server.on("/msdownload/update/v3/static/trustedr/en/authrootstl.cab", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(204);
  });
  server.on("/msdownload/update/v3/static/trustedr/en/pinrulesstl.cab", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(204);
  });
  server.on("/r/r1.crl", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(204);
  });

  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[WEBSERVER] Request: /config.json"));
    File f = LittleFS.open("/config.json", "r");
    if (!f) {
      Serial.println(F("[WEBSERVER] Error opening /config.json"));
      request->send(500, "application/json", "{\"error\":\"Failed to open config.json\"}");
      return;
    }
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
      Serial.print(F("[WEBSERVER] Error parsing /config.json: "));
      Serial.println(err.f_str());
      request->send(500, "application/json", "{\"error\":\"Failed to parse config.json\"}");
      return;
    }

    // Always sanitize before sending to browser
    doc[F("ssid")] = getSafeSsid();
    doc[F("password")] = getSafePassword();
    doc[F("openWeatherApiKey")] = getSafeApiKey();
    doc[F("mode")] = isAPMode ? "ap" : "sta";

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[WEBSERVER] Request: /save"));
    DynamicJsonDocument doc(2048);

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

      else if (n == "password") {
        if (v != "********" && v.length() > 0) {
          doc[n] = v;  // user entered a new password
        } else {
          Serial.println(F("[SAVE] Password unchanged."));
          // do nothing, keep the one already in doc
        }
      }

      else if (n == "openWeatherApiKey") {
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
        Serial.println("[SAVE] Error converting countdown date/time to timestamp.");
        newTargetTimestamp = 0;
      } else {
        Serial.printf("[SAVE] Converted countdown target: %s -> %lu\n", countdownDateStr.c_str(), newTargetTimestamp);
      }
    }

    JsonObject countdownObj = doc.createNestedObject("countdown");
    countdownObj["enabled"] = newCountdownEnabled;
    countdownObj["targetTimestamp"] = newTargetTimestamp;
    countdownObj["label"] = countdownLabelStr;
    countdownObj["isDramaticCountdown"] = newIsDramaticCountdown;

    FSInfo fs_info;
    LittleFS.info(fs_info);
    Serial.printf("[SAVE] LittleFS total bytes: %u, used bytes: %u\n", fs_info.totalBytes, fs_info.usedBytes);

    if (LittleFS.exists("/config.json")) {
      Serial.println(F("[SAVE] Renaming /config.json to /config.bak"));
      LittleFS.rename("/config.json", "/config.bak");
    }
    File f = LittleFS.open("/config.json", "w");
    if (!f) {
      Serial.println(F("[SAVE] ERROR: Failed to open /config.json for writing!"));
      DynamicJsonDocument errorDoc(256);
      errorDoc[F("error")] = "Failed to write config file.";
      String response;
      serializeJson(errorDoc, response);
      request->send(500, "application/json", response);
      return;
    }

    size_t bytesWritten = serializeJson(doc, f);
    Serial.printf("[SAVE] Bytes written to /config.json: %u\n", bytesWritten);
    f.close();
    Serial.println(F("[SAVE] /config.json file closed."));

    File verify = LittleFS.open("/config.json", "r");
    if (!verify) {
      Serial.println(F("[SAVE] ERROR: Failed to open /config.json for reading during verification!"));
      DynamicJsonDocument errorDoc(256);
      errorDoc[F("error")] = "Verification failed: Could not re-open config file.";
      String response;
      serializeJson(errorDoc, response);
      request->send(500, "application/json", response);
      return;
    }

    while (verify.available()) {
      verify.read();
    }
    verify.seek(0);

    DynamicJsonDocument test(2048);
    DeserializationError err = deserializeJson(test, verify);
    verify.close();

    if (err) {
      Serial.print(F("[SAVE] Config corrupted after save: "));
      Serial.println(err.f_str());
      DynamicJsonDocument errorDoc(256);
      errorDoc[F("error")] = String("Config corrupted. Reboot cancelled. Error: ") + err.f_str();
      String response;
      serializeJson(errorDoc, response);
      request->send(500, "application/json", response);
      return;
    }

    Serial.println(F("[SAVE] Config verification successful."));
    DynamicJsonDocument okDoc(128);
    strlcpy(customMessage, doc["customMessage"] | "", sizeof(customMessage));
    okDoc[F("message")] = "Saved successfully. Rebooting...";
    String response;
    serializeJson(okDoc, response);
    request->send(200, "application/json", response);
    Serial.println(F("[WEBSERVER] Sending success response and scheduling reboot..."));

    request->onDisconnect([]() {
      Serial.println(F("[WEBSERVER] Client disconnected, rebooting ESP..."));
      saveUptime();
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
        DynamicJsonDocument errorDoc(128);
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
        DynamicJsonDocument errorDoc(128);
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

      DynamicJsonDocument okDoc(128);
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
      DynamicJsonDocument errorDoc(128);
      errorDoc[F("error")] = "No backup found.";
      String response;
      serializeJson(errorDoc, response);
      request->send(404, "application/json", response);
    }
  });

  server.on("/ap_status", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.print(F("[WEBSERVER] Request: /ap_status. isAPMode = "));
    Serial.println(isAPMode);
    String json = "{\"isAP\": ";
    json += (isAPMode) ? "true" : "false";
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/set_brightness", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("value", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing value\"}");
      return;
    }

    String sourceHeader = request->header("X-Source");
    bool isFromUI = (sourceHeader == "UI");
    bool isFromHA = !isFromUI;

    int newBrightness = request->getParam("value", true)->value().toInt();

    // Handle OFF request
    if (newBrightness == -1) {
      P.displayShutdown(true);
      P.displayClear();
      displayOff = true;

      Serial.printf("[BRIGHTNESS] Display OFF via %s\n",
                    isFromUI ? "UI" : "HA");

      request->send(200, "application/json", "{\"ok\":true, \"display\":\"off\"}");
      return;
    }

    // Clamp brightness range (0–15)
    newBrightness = constrain(newBrightness, 0, 15);

    if (displayOff) {
      // Wake from OFF
      P.setIntensity(newBrightness);
      advanceDisplayModeSafe();
      P.displayShutdown(false);
      brightness = newBrightness;
      displayOff = false;

      Serial.printf("[BRIGHTNESS] Display woke from OFF via %s → %d\n",
                    isFromUI ? "UI" : "HA",
                    newBrightness);
    } else {
      // Display already ON
      brightness = newBrightness;
      P.setIntensity(brightness);

      Serial.printf("[BRIGHTNESS] Set to %d via %s\n",
                    brightness,
                    isFromUI ? "UI" : "HA");
    }
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_flip", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool flip = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      flip = (v == "1" || v == "true" || v == "on");
    }
    flipDisplay = flip;
    P.setZoneEffect(0, flipDisplay, PA_FLIP_UD);
    P.setZoneEffect(0, flipDisplay, PA_FLIP_LR);
    Serial.printf("[WEBSERVER] Set flipDisplay to %d\n", flipDisplay);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_twelvehour", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool twelveHour = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      twelveHour = (v == "1" || v == "true" || v == "on");
    }
    twelveHourToggle = twelveHour;
    Serial.printf("[WEBSERVER] Set twelveHourToggle to %d\n", twelveHourToggle);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_dayofweek", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool showDay = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      showDay = (v == "1" || v == "true" || v == "on");
    }
    showDayOfWeek = showDay;
    Serial.printf("[WEBSERVER] Set showDayOfWeek to %d\n", showDayOfWeek);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_showdate", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool showDateVal = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      showDateVal = (v == "1" || v == "true" || v == "on");
    }
    showDate = showDateVal;
    Serial.printf("[WEBSERVER] Set showDate to %d\n", showDate);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_humidity", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool showHumidityNow = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      showHumidityNow = (v == "1" || v == "true" || v == "on");
    }
    showHumidity = showHumidityNow;
    Serial.printf("[WEBSERVER] Set showHumidity to %d\n", showHumidity);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_colon_blink", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool enableBlink = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      enableBlink = (v == "1" || v == "true" || v == "on");
    }
    colonBlinkEnabled = enableBlink;
    Serial.printf("[WEBSERVER] Set colonBlinkEnabled to %d\n", colonBlinkEnabled);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_language", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("value", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing value\"}");
      return;
    }

    String lang = request->getParam("value", true)->value();
    lang.trim();         // Remove whitespace/newlines
    lang.toLowerCase();  // Normalize to lowercase

    strlcpy(language, lang.c_str(), sizeof(language));              // Safe copy to char[]
    Serial.printf("[WEBSERVER] Set language to '%s'\n", language);  // Use quotes for debug

    shouldFetchWeatherNow = true;

    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_weatherdesc", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool showDesc = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      showDesc = (v == "1" || v == "true" || v == "on");
    }

    if (showWeatherDescription == true && showDesc == false) {
      Serial.println(F("[WEBSERVER] showWeatherDescription toggled OFF. Checking display mode..."));
      if (displayMode == 2) {
        Serial.println(F("[WEBSERVER] Currently in Weather Description mode. Forcing mode advance/cleanup."));
        advanceDisplayMode();
      }
    }

    showWeatherDescription = showDesc;
    Serial.printf("[WEBSERVER] Set Show Weather Description to %d\n", showWeatherDescription);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_units", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      if (v == "1" || v == "true" || v == "on") {
        strcpy(weatherUnits, "imperial");
        tempSymbol = ']';
      } else {
        strcpy(weatherUnits, "metric");
        tempSymbol = '[';
      }
      Serial.printf("[WEBSERVER] Set weatherUnits to %s\n", weatherUnits);
      shouldFetchWeatherNow = true;
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
    }
  });

  server.on("/set_countdown_enabled", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool enableCountdownNow = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      enableCountdownNow = (v == "1" || v == "true" || v == "on");
    }

    if (countdownEnabled == enableCountdownNow) {
      Serial.println(F("[WEBSERVER] Countdown enable state unchanged, ignoring."));
      request->send(200, "application/json", "{\"ok\":true}");
      return;
    }

    if (countdownEnabled == true && enableCountdownNow == false) {
      Serial.println(F("[WEBSERVER] Countdown toggled OFF. Checking display mode..."));
      if (displayMode == 3) {
        Serial.println(F("[WEBSERVER] Currently in Countdown mode. Forcing mode advance/cleanup."));
        advanceDisplayMode();
      }
    }

    countdownEnabled = enableCountdownNow;
    Serial.printf("[WEBSERVER] Set Countdown Enabled to %d\n", countdownEnabled);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_dramatic_countdown", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool enableDramaticNow = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      enableDramaticNow = (v == "1" || v == "true" || v == "on");
    }

    // Check if the state has changed
    if (isDramaticCountdown == enableDramaticNow) {
      Serial.println(F("[WEBSERVER] Dramatic Countdown state unchanged, ignoring."));
      request->send(200, "application/json", "{\"ok\":true}");
      return;
    }

    // Update the global variable
    isDramaticCountdown = enableDramaticNow;

    // Call saveCountdownConfig with only the existing parameters.
    // It will read the updated global variable 'isDramaticCountdown'.
    saveCountdownConfig(countdownEnabled, countdownTargetTimestamp, countdownLabel);

    Serial.printf("[WEBSERVER] Set Dramatic Countdown to %d\n", isDramaticCountdown);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  // Set Clock-only-during-dimming (no reboot)
  server.on("/set_clock_only_dimming", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool enableNow = false;
    if (request->hasParam("value", true)) {
      String v = request->getParam("value", true)->value();
      enableNow = (v == "1" || v == "true" || v == "on");
    }

    // Update runtime variable immediately
    clockOnlyDuringDimming = enableNow;
    Serial.printf("[WEBSERVER] Set clockOnlyDuringDimming to %d (requested)\n", clockOnlyDuringDimming);

    // Read existing config.json (if present)
    DynamicJsonDocument doc(2048);
    bool needToWrite = true;
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      DeserializationError err = deserializeJson(doc, configFile);
      configFile.close();
      if (err) {
        Serial.print(F("[WEBSERVER] Error parsing existing config.json: "));
        Serial.println(err.f_str());
        // proceed to write (will create a new doc)
        doc.clear();
      } else {
        // If the key exists and matches the requested value, skip write
        bool existing = doc["clockOnlyDuringDimming"] | false;
        if (existing == enableNow) {
          Serial.println(F("[WEBSERVER] clockOnlyDuringDimming unchanged — skipping write."));
          // Send immediate OK response without touching FS
          DynamicJsonDocument okDoc(128);
          okDoc[F("ok")] = true;
          okDoc[F("clockOnlyDuringDimming")] = enableNow;
          String response;
          serializeJson(okDoc, response);
          request->send(200, "application/json", response);
          return;
        }
      }
    } else {
      // No config file found — doc is empty and we will write
      doc.clear();
    }

    // Set/update the key in the JSON doc
    doc[F("clockOnlyDuringDimming")] = clockOnlyDuringDimming;

    // Backup existing file only if it exists (and only because we're about to replace it)
    if (LittleFS.exists("/config.json")) {
      if (!LittleFS.rename("/config.json", "/config.bak")) {
        Serial.println(F("[WEBSERVER] Warning: failed to create config backup"));
        // continue anyway
      }
    }

    File f = LittleFS.open("/config.json", "w");
    if (!f) {
      Serial.println(F("[WEBSERVER] ERROR: Failed to open /config.json for writing"));
      DynamicJsonDocument errDoc(128);
      errDoc[F("error")] = "Failed to write config file.";
      String response;
      serializeJson(errDoc, response);
      request->send(500, "application/json", response);
      return;
    }

    size_t bytesWritten = serializeJson(doc, f);
    f.close();
    Serial.printf("[WEBSERVER] Saved clockOnlyDuringDimming=%d to /config.json (%u bytes written)\n", clockOnlyDuringDimming, bytesWritten);

    // Send immediate response (no reboot)
    DynamicJsonDocument okDoc(128);
    okDoc[F("ok")] = true;
    okDoc[F("clockOnlyDuringDimming")] = clockOnlyDuringDimming;
    String response;
    serializeJson(okDoc, response);
    request->send(200, "application/json", response);
  });

  // --- Custom Message Endpoint ---
  server.on("/set_custom_message", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("message", true)) {
      String msg = request->getParam("message", true)->value();
      msg.trim();

      String sourceHeader = request->header("X-Source");
      bool isFromUI = (sourceHeader == "UI");
      bool isFromHA = !isFromUI;

      messageDisplaySeconds = 0;  // Reset
      if (request->hasParam("seconds", true)) {
        messageDisplaySeconds = constrain(request->getParam("seconds", true)->value().toInt(), 0, 3600);  // 1 hour max
      }

      messageScrollTimes = 0;  // Reset
      if (request->hasParam("scrolltimes", true)) {
        messageScrollTimes = constrain(request->getParam("scrolltimes", true)->value().toInt(), 0, 100);  // 100 max scrolls
      }

      // --- Local speed variable (does not modify global GENERAL_SCROLL_SPEED) ---
      int localSpeed = GENERAL_SCROLL_SPEED;  // Default for UI messages
      if (request->hasParam("speed", true)) {
        localSpeed = constrain(request->getParam("speed", true)->value().toInt(), 10, 200);
      }


      // --- CLEAR MESSAGE ---
      if (msg.length() == 0) {
        if (isFromUI) {
          // Web UI clear: The "real" clear, resets everything.
          customMessage[0] = '\0';
          lastPersistentMessage[0] = '\0';
          displayMode = 0;
          messageStartTime = 0;
          currentScrollCount = 0;
          messageDisplaySeconds = 0;
          messageScrollTimes = 0;
          Serial.println(F("[MESSAGE] All messages cleared by UI. Returning to normal mode."));
          request->send(200, "text/plain", "CLEARED (UI)");

          // --- SAVE CLEAR STATE ---
          saveCustomMessageToConfig("");
        } else {
          // HA clear: remove only temporary message, reset time/scroll variables.
          customMessage[0] = '\0';  // Clear the currently active message

          // Reset the temporary HA timing/scroll limits.
          messageStartTime = 0;
          currentScrollCount = 0;
          messageDisplaySeconds = 0;
          messageScrollTimes = 0;

          if (strlen(lastPersistentMessage) > 0) {
            // Restore the last persistent message
            strncpy(customMessage, lastPersistentMessage, sizeof(customMessage));
            messageScrollSpeed = GENERAL_SCROLL_SPEED;  // Use global speed for persistent

            // Ensure displayMode is set to 6 so the restored persistent message is shown immediately.
            displayMode = 6;
            prevDisplayMode = 0;

            Serial.printf("[MESSAGE] Temporary HA message cleared. Restored persistent message: '%s' (speed=%d)\n",
                          customMessage, messageScrollSpeed);
            request->send(200, "text/plain", "CLEARED (HA temporary, persistent restored)");
          } else {
            // No persistent message to restore, return to clock mode.
            displayMode = 0;
            Serial.println(F("[MESSAGE] Temporary HA message cleared. No persistent message to restore."));
            request->send(200, "text/plain", "CLEARED (HA temporary, no persistent)");
          }
        }
        return;
      }

      // --- SANITIZE MESSAGE ---
      msg.toUpperCase();
      String filtered = "";
      for (size_t i = 0; i < msg.length(); i++) {
        char c = msg[i];
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == ':' || c == '!' || c == '\'' || c == '-' || c == '.' || c == ',' || c == '_' || c == '+' || c == '%' || c == '/' || c == '?') {
          filtered += c;
        }
        // Check for degree symbol (UTF-8 0xC2 0xB0)
        else if ((unsigned char)c == 0xC2 && i + 1 < msg.length() && (unsigned char)msg[i + 1] == 0xB0) {
          filtered += "°";  // add single character
          i++;              // skip next byte
        }
      }

      filtered.toCharArray(customMessage, sizeof(customMessage));

      // --- STORE MESSAGE ---
      if (isFromHA) {
        // --- Only backup if lastPersistentMessage exists ---
        if (strlen(lastPersistentMessage) > 0) {
          Serial.printf("[HA] Will preserve persistent message: '%s'\n", lastPersistentMessage);
        } else {
          Serial.println(F("[HA] No persistent message to preserve. HA message is temporary only."));
        }

        // --- Overwrite customMessage with new temporary HA message ---
        filtered.toCharArray(customMessage, sizeof(customMessage));
        messageScrollSpeed = localSpeed;

        Serial.printf("[HA] Temporary HA message received: '%s' (persistent: '%s', duration: %ds, scrolls: %d, speed: %d)\n",
                      customMessage,
                      strlen(lastPersistentMessage) ? lastPersistentMessage : "(none)",
                      messageDisplaySeconds,  // Added seconds
                      messageScrollTimes,     // Added scrolltimes
                      localSpeed);            // Added speed
      } else {
        // --- UI-originated message: permanent ---
        filtered.toCharArray(customMessage, sizeof(customMessage));
        strlcpy(lastPersistentMessage, customMessage, sizeof(lastPersistentMessage));
        messageScrollSpeed = GENERAL_SCROLL_SPEED;  // Always global for UI

        Serial.printf("[UI] Persistent message stored: %s (speed=%d)\n",
                      customMessage, messageScrollSpeed);

        // --- Persist to config.json immediately ---
        saveCustomMessageToConfig(customMessage);
      }

      // --- Activate display ---
      displayMode = 6;
      prevDisplayMode = 0;
      messageStartTime = millis();  // Start the timer
      currentScrollCount = 0;

      String response = String(isFromHA ? "OK (HA message, speed=" : "OK (UI message, speed=") + String(localSpeed);
      response += String(", duration=") + String(messageDisplaySeconds) + "s, scrolls=" + String(messageScrollTimes) + ")";
      request->send(200, "text/plain", response);
    } else {
      Serial.println(F("[MESSAGE] Error: missing 'message' parameter in request."));
      request->send(400, "text/plain", "Missing message parameter");
    }
  });

  server.on("/uptime", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!LittleFS.exists("/uptime.dat")) {
      request->send(200, "text/plain", "No uptime recorded yet.");
      return;
    }

    File f = LittleFS.open("/uptime.dat", "r");
    if (!f) {
      request->send(500, "text/plain", "Error reading uptime file.");
      return;
    }

    String content = f.readString();

    unsigned long seconds = content.toInt();
    String formatted = formatUptime(seconds);
    request->send(200, "text/plain", formatted);
  });

  server.on("/export", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println(F("[WEBSERVER] Request: /export"));

    File f;
    if (LittleFS.exists("/config.json")) {
      f = LittleFS.open("/config.json", "r");
      Serial.println(F("[EXPORT] Using /config.json"));
    } else if (LittleFS.exists("/config.bak")) {
      f = LittleFS.open("/config.bak", "r");
      Serial.println(F("[EXPORT] /config.json not found, using /config.bak"));
    } else {
      request->send(404, "application/json", "{\"error\":\"No config found\"}");
      return;
    }

    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
      Serial.print(F("[EXPORT] Error parsing config: "));
      Serial.println(err.f_str());
      request->send(500, "application/json", "{\"error\":\"Failed to parse config\"}");
      return;
    }

    // Only sanitize if NOT in AP mode
    if (!isAPMode) {
      doc["ssid"] = "********";
      doc["password"] = "********";
      doc["openWeatherApiKey"] = "********************************";
    }

    doc["mode"] = isAPMode ? "ap" : "sta";

    String jsonOut;
    serializeJsonPretty(doc, jsonOut);

    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", jsonOut);
    resp->addHeader("Content-Disposition", "attachment; filename=\"config.json\"");
    request->send(resp);
  });

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html style="background: radial-gradient(ellipse at 70% 0%, #2b425a 0%, #171e23 100%); height: 100%;">
      <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <style>
          body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, "Helvetica Neue", sans-serif;
            color: #FFFFFF;
            transition: opacity 0.6s cubic-bezier(.4, 0, .2, 1);
            line-height: 1.5;
            max-width: 300px;
            margin: 3rem auto;
            background: linear-gradient(120deg, rgba(45, 65, 90, 0.72) 0%, rgba(53, 133, 183, 0.38) 100%);
            padding: 1.5rem;
            border-radius: 24px;
            box-shadow: 0 10px 36px 0 rgba(40, 170, 255, 0.11), 0 2px 8px 0 rgba(44, 70, 110, 0.08);
            border: 1.5px solid rgba(180, 230, 255, 0.10);
            text-align: center;
            }
          
          h3 {
            margin-top: 0;
            }

          input::file-selector-button {
            background: linear-gradient(90deg, #3e99bc, #47add4 85%);
            color: white;
            padding: 0.9rem;
            font-size: 1rem;
            font-weight: 600;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            text-align: center;
            transition: background 0.25s, transform 0.15s 
            ease-in-out;
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
      <html style="background: radial-gradient(ellipse at 70% 0%, #2b425a 0%, #171e23 100%); height: 100%;">
        <head>
          <meta charset="UTF-8" />
          <meta name="viewport" content="width=device-width, initial-scale=1" />
          <title>Upload Successful</title>
          <meta http-equiv="refresh" content="1; url=/" />
          <style>
            body {
              font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, "Helvetica Neue", sans-serif;
              color: #FFFFFF;
              transition: opacity 0.6s cubic-bezier(.4, 0, .2, 1);
              line-height: 1.5;
              max-width: 300px;
              margin: 3rem auto;
              background: linear-gradient(120deg, rgba(45, 65, 90, 0.72) 0%, rgba(53, 133, 183, 0.38) 100%);
              padding: 1.5rem;
              border-radius: 24px;
              box-shadow: 0 10px 36px 0 rgba(40, 170, 255, 0.11), 0 2px 8px 0 rgba(44, 70, 110, 0.08);
              border: 1.5px solid rgba(180, 230, 255, 0.10);
              text-align: center;
              }
            
            h3 {
              margin-top: 0;
              }

            input::file-selector-button {
              background: linear-gradient(90deg, #3e99bc, #47add4 85%);
              color: white;
              padding: 0.9rem;
              font-size: 1rem;
              font-weight: 600;
              border: none;
              border-radius: 8px;
              cursor: pointer;
              text-align: center;
              transition: background 0.25s, transform 0.15s 
              ease-in-out;
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

  server.on("/factory_reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    // If not in AP mode, block and return a 403 response
    if (!isAPMode) {
      request->send(403, "text/plain", "Factory reset only allowed in AP mode.");
      Serial.println(F("[RESET] Factory reset attempt blocked (not in AP mode)."));
      return;
    }
    const char *FACTORY_RESET_HTML = R"rawliteral(
      <!DOCTYPE html>
      <html style="background: radial-gradient(ellipse at 70% 0%, #2b425a 0%, #171e23 100%); height: 100%;">
        <head>
          <meta charset="UTF-8" />
          <meta name="viewport" content="width=device-width, initial-scale=1" />
          <title>Resetting Device</title>
          <style>
            body {
              font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, "Helvetica Neue", sans-serif;
              color: #FFFFFF;
              line-height: 1.5;
              max-width: 300px;
              margin: 3rem auto;
              background: linear-gradient(120deg, rgba(144, 45, 45, 0.72) 0%, rgba(183, 53, 53, 0.38) 100%);
              padding: 1.5rem;
              border-radius: 24px;
              box-shadow: 0 10px 36px 0 rgba(255, 40, 40, 0.11), 0 2px 8px 0 rgba(110, 44, 44, 0.08);
              border: 1.5px solid rgba(255, 180, 180, 0.10);
              text-align: center;
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
            Serial.printf("[RESET] Deleted %s\n", file);
          } else {
            Serial.printf("[RESET] ERROR deleting %s\n", file);
          }
        } else {
          Serial.printf("[RESET] %s not found, skipping delete.\n", file);
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

  server.onNotFound(handleCaptivePortal);
  server.begin();
  Serial.println(F("[WEBSERVER] Web server started"));
}

void handleCaptivePortal(AsyncWebServerRequest *request) {
  String uri = request->url();

  // Filter out system-generated probe requests
  if (!uri.endsWith("/204") && !uri.endsWith("/ipv6check") && !uri.endsWith("connecttest.txt") && !uri.endsWith("/generate_204") && !uri.endsWith("/fwlink") && !uri.endsWith("/hotspot-detect.html")) {

    Serial.print(F("[WEBSERVER] Captive Portal triggered for URL: "));
    Serial.println(uri);
  }

  if (isAPMode) {
    IPAddress apIP = WiFi.softAPIP();
    String redirectUrl = "http://" + apIP.toString() + "/";
    Serial.print(F("[WEBSERVER] Redirecting to captive portal: "));
    Serial.println(redirectUrl);
    request->redirect(redirectUrl);
  } else {
    Serial.println(F("[WEBSERVER] Not in AP mode — sending 404"));
    request->send(404, "text/plain", "Not found");
  }
}

String normalizeWeatherDescription(String str) {
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
    char c = str.charAt(i);
    if ((c >= 'A' && c <= 'Z') || c == ' ') {
      result += c;
    }
  }
  return result;
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
  String base = "https://api.openweathermap.org/data/2.5/weather?";

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
  Serial.println(F("[WEATHER] URL: ") + url);

  WiFiClientSecure client;  // use secure client for HTTPS
  client.stop();            // ensure previous session closed
  yield();                  // Allow OS to process socket closure
  client.setInsecure();     // no cert validation
  HTTPClient http;          // Create an HTTPClient object
  http.begin(client, url);  // Pass the WiFiClient object and the URL
  client.setBufferSizes(512, 512);
  http.setTimeout(10000);  // Sets both connection and stream timeout to 10 seconds

  Serial.println(F("[WEATHER] Sending GET request..."));
  int httpCode = http.GET();  // Send the GET request

  if (httpCode == HTTP_CODE_OK) {  // Check if HTTP response code is 200 (OK)
    Serial.println(F("[WEATHER] HTTP 200 OK. Reading payload..."));

    String payload = http.getString();
    Serial.println(F("[WEATHER] Response received."));
    Serial.println(F("[WEATHER] Payload: ") + payload);

    DynamicJsonDocument doc(1536);  // Adjust size as needed, use ArduinoJson Assistant
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("[WEATHER] JSON parse error: "));
      Serial.println(error.f_str());
      weatherAvailable = false;
      return;
    }

    if (doc.containsKey(F("main")) && doc[F("main")].containsKey(F("temp"))) {
      float temp = doc[F("main")][F("temp")];
      currentTemp = String((int)round(temp)) + "°";
      Serial.printf("[WEATHER] Temp: %s\n", currentTemp.c_str());
      weatherAvailable = true;
    } else {
      Serial.println(F("[WEATHER] Temperature not found in JSON payload"));
      weatherAvailable = false;
      return;
    }

    if (doc.containsKey(F("main")) && doc[F("main")].containsKey(F("humidity"))) {
      currentHumidity = doc[F("main")][F("humidity")];
      Serial.printf("[WEATHER] Humidity: %d%%\n", currentHumidity);
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
    } else {
      Serial.println(F("[WEATHER] Weather description not found in JSON payload"));
    }

    weatherDescription = normalizeWeatherDescription(detailedDesc);
    Serial.printf("[WEATHER] Description used: %s\n", weatherDescription.c_str());

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

        Serial.printf("[WEATHER] Adjusted Sunrise/Sunset (local): %02d:%02d | %02d:%02d\n",
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
      DynamicJsonDocument doc(1024);

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
              serializeJsonPretty(doc, f);
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
    Serial.printf("[WEATHER] HTTP GET failed, error code: %d, reason: %s\n",
                  httpCode, http.errorToString(httpCode).c_str());
    weatherAvailable = false;
    weatherFetched = false;
  }

  http.end();
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
      Serial.printf("[UPTIME] Loaded accumulated uptime: %lu seconds (%.2f hours)\n",
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
    Serial.printf("[UPTIME] Saved accumulated uptime: %s\n", formatTotalRuntime().c_str());
  } else {
    Serial.println(F("[UPTIME] Failed to write /uptime.dat"));
  }
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

  DynamicJsonDocument doc(2048);

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
  Serial.printf("[CONFIG] Saved customMessage='%s' (%u bytes written)\n", msg, bytesWritten);
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


// -----------------------------------------------------------------------------
// Main setup() and loop()
// -----------------------------------------------------------------------------
/*
DisplayMode key:
  0: Clock
  1: Weather
  2: Weather Description
  3: Countdown
  4: Nightscout
  5: Date
  6: Custom Message
*/
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("[SETUP] Starting setup..."));

  if (!LittleFS.begin()) {
    Serial.println(F("[ERROR] LittleFS mount failed in setup! Halting."));
    while (true) {
      delay(1000);
      yield();
    }
  }
  Serial.println(F("[SETUP] LittleFS file system mounted successfully."));
  loadUptime();
  ensureHtmlFileExists();
  P.begin();  // Initialize Parola library

  P.setCharSpacing(0);
  P.setFont(mFactory);
  loadConfig();  // This function now has internal yields and prints

  P.setIntensity(brightness);
  P.setZoneEffect(0, flipDisplay, PA_FLIP_UD);
  P.setZoneEffect(0, flipDisplay, PA_FLIP_LR);

  Serial.println(F("[SETUP] Parola (LED Matrix) initialized"));

#if defined(ESP32)
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    const char *name = nullptr;
    switch (event) {
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        name = "GOT_IP";
        lastWifiConnectTime = millis();
        Serial.println("[WIFI EVENT] Re-initializing mDNS due to new IP.");
        setupMDNS();
        break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        name = "DISCONNECTED";
        MDNS.end();
        Serial.println("[WIFI EVENT] mDNS stopped.");
        break;
      default: return;  // ignore all other events
    }
    Serial.printf("[WIFI EVENT] %s (%d)\n", name, event);
  });

#elif defined(ESP8266)
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  mConnectHandler = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected &ev) {
    Serial.println("[WIFI EVENT] Connected");
  });

  mDisConnectHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &ev) {
    Serial.printf("[WIFI EVENT] Disconnected (Reason: %d)\n", ev.reason);
    MDNS.end();
    Serial.println("[WIFI EVENT] mDNS stopped.");
  });

  mGotIpHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &ev) {
    Serial.printf("[WIFI EVENT] GOT_IP - IP: %s\n", ev.ip.toString().c_str());
    lastWifiConnectTime = millis();
    Serial.println("[WIFI EVENT] Re-initializing mDNS due to new IP.");
    setupMDNS();
  });
#endif

  connectWiFi();

  if (isAPMode) {
    Serial.println(F("[SETUP] WiFi connection failed. Device is in AP Mode."));
  } else if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("[SETUP] WiFi connected successfully to local network."));
  } else {
    Serial.println(F("[SETUP] WiFi state is uncertain after connection attempt."));
  }

  setupMDNS();
  setupWebServer();
  Serial.println(F("[SETUP] Webserver setup complete"));
  Serial.println(F("[SETUP] Setup complete"));
  Serial.println();
  printConfigToSerial();
  setupTime();
  displayMode = 0;
  lastSwitch = millis() - (clockDuration - 500);
  lastColonBlink = millis();
  bootMillis = millis();
  saveUptime();
}

void ensureHtmlFileExists() {
  Serial.println(F("[FS] Checking for /index.html on LittleFS..."));

  // Length of embedded HTML in PROGMEM
  size_t expectedSize = strlen_P(index_html);

  // If the file exists, verify size before deciding to trust it
  if (LittleFS.exists("/index.html")) {
    File f = LittleFS.open("/index.html", "r");

    if (!f) {
      Serial.println(F("[FS] ERROR: /index.html exists but failed to open! Will rewrite."));
    } else {
      size_t actualSize = f.size();
      f.close();

      if (actualSize == expectedSize) {
        Serial.printf("[FS] /index.html found (size OK: %u bytes). Using file system version.\n", actualSize);
        return;  // STOP HERE — file is good
      }

      Serial.printf(
        "[FS] /index.html size mismatch! Expected %u bytes, found %u. Rewriting...\n",
        expectedSize, actualSize);
    }
  } else {
    Serial.println(F("[FS] /index.html NOT found. Writing embedded content to LittleFS..."));
  }

  // -------------------------------
  // Write embedded HTML to LittleFS
  // -------------------------------

  File f = LittleFS.open("/index.html", "w");
  if (!f) {
    Serial.println(F("[FS] ERROR: Failed to create /index.html for writing!"));
    return;
  }

  size_t htmlLength = expectedSize;
  size_t bytesWritten = 0;

  for (size_t i = 0; i < htmlLength; i++) {
    char c = pgm_read_byte_near(index_html + i);

    if (f.write((uint8_t *)&c, 1) == 1) {
      bytesWritten++;
    } else {
      Serial.printf("[FS] Write failure at character %u. Aborting write.\n", i);
      f.close();
      return;
    }
  }

  f.close();

  if (bytesWritten == htmlLength) {
    Serial.printf("[FS] Successfully wrote %u bytes to /index.html.\n", bytesWritten);
  } else {
    Serial.printf("[FS] WARNING: Only wrote %u of %u bytes to /index.html (might be incomplete).\n",
                  bytesWritten, htmlLength);
  }
}

void advanceDisplayMode() {

  // If user requested clock-only during dimming and we are currently dimmed, stay on clock
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
      Serial.println(F("[DISPLAY] advanceDisplayMode(): Staying in CLOCK because Clock-only-dimming is enabled and dimming is active."));
      return;
    }
  }

  prevDisplayMode = displayMode;
  int oldMode = displayMode;
  String ntpField = String(ntpServer2);
  bool nightscoutConfigured = ntpField.startsWith("https://");

  if (displayMode == 0) {  // Clock -> ...
    if (showDate) {
      displayMode = 5;  // Date mode right after Clock
      Serial.println(F("[DISPLAY] Switching to display mode: DATE (from Clock)"));
    } else if (weatherAvailable && (strlen(openWeatherApiKey) == 32) && (strlen(openWeatherCity) > 0) && (strlen(openWeatherCountry) > 0)) {
      displayMode = 1;
      Serial.println(F("[DISPLAY] Switching to display mode: WEATHER (from Clock)"));
    } else if (countdownEnabled && !countdownFinished && ntpSyncSuccessful && countdownTargetTimestamp > 0 && countdownTargetTimestamp > time(nullptr)) {
      displayMode = 3;
      Serial.println(F("[DISPLAY] Switching to display mode: COUNTDOWN (from Clock, weather skipped)"));
    } else if (nightscoutConfigured) {
      displayMode = 4;  // Clock -> Nightscout (if weather & countdown are skipped)
      Serial.println(F("[DISPLAY] Switching to display mode: NIGHTSCOUT (from Clock, weather & countdown skipped)"));
    } else {
      displayMode = 0;
      Serial.println(F("[DISPLAY] Staying in CLOCK (from Clock)"));
    }
  } else if (displayMode == 5) {  // Date mode
    if (weatherAvailable && (strlen(openWeatherApiKey) == 32) && (strlen(openWeatherCity) > 0) && (strlen(openWeatherCountry) > 0)) {
      displayMode = 1;
      Serial.println(F("[DISPLAY] Switching to display mode: WEATHER (from Date)"));
    } else if (countdownEnabled && !countdownFinished && ntpSyncSuccessful && countdownTargetTimestamp > 0 && countdownTargetTimestamp > time(nullptr)) {
      displayMode = 3;
      Serial.println(F("[DISPLAY] Switching to display mode: COUNTDOWN (from Date, weather skipped)"));
    } else if (nightscoutConfigured) {
      displayMode = 4;
      Serial.println(F("[DISPLAY] Switching to display mode: NIGHTSCOUT (from Date, weather & countdown skipped)"));
    } else {
      displayMode = 0;
      Serial.println(F("[DISPLAY] Switching to display mode: CLOCK (from Date)"));
    }
  } else if (displayMode == 1) {  // Weather -> ...
    if (showWeatherDescription && weatherAvailable && weatherDescription.length() > 0) {
      displayMode = 2;
      Serial.println(F("[DISPLAY] Switching to display mode: DESCRIPTION (from Weather)"));
    } else if (countdownEnabled && !countdownFinished && ntpSyncSuccessful && countdownTargetTimestamp > 0 && countdownTargetTimestamp > time(nullptr)) {
      displayMode = 3;
      Serial.println(F("[DISPLAY] Switching to display mode: COUNTDOWN (from Weather)"));
    } else if (nightscoutConfigured) {
      displayMode = 4;  // Weather -> Nightscout (if description & countdown are skipped)
      Serial.println(F("[DISPLAY] Switching to display mode: NIGHTSCOUT (from Weather, description & countdown skipped)"));
    } else {
      displayMode = 0;
      Serial.println(F("[DISPLAY] Switching to display mode: CLOCK (from Weather)"));
    }
  } else if (displayMode == 2) {  // Weather Description -> ...
    if (countdownEnabled && !countdownFinished && ntpSyncSuccessful && countdownTargetTimestamp > 0 && countdownTargetTimestamp > time(nullptr)) {
      displayMode = 3;
      Serial.println(F("[DISPLAY] Switching to display mode: COUNTDOWN (from Description)"));
    } else if (nightscoutConfigured) {
      displayMode = 4;  // Description -> Nightscout (if countdown is skipped)
      Serial.println(F("[DISPLAY] Switching to display mode: NIGHTSCOUT (from Description, countdown skipped)"));
    } else {
      displayMode = 0;
      Serial.println(F("[DISPLAY] Switching to display mode: CLOCK (from Description)"));
    }
  } else if (displayMode == 3) {  // Countdown -> Nightscout
    if (nightscoutConfigured) {
      displayMode = 4;
      Serial.println(F("[DISPLAY] Switching to display mode: NIGHTSCOUT (from Countdown)"));
    } else {
      displayMode = 0;
      Serial.println(F("[DISPLAY] Switching to display mode: CLOCK (from Countdown)"));
    }
  } else if (displayMode == 4) {  // Nightscout -> Custom Message
    displayMode = 6;
    Serial.println(F("[DISPLAY] Switching to display mode: CUSTOM MESSAGE (from Nightscout)"));
  } else if (displayMode == 6) {  // Custom Message -> Clock
    displayMode = 0;
    Serial.println(F("[DISPLAY] Switching to display mode: CLOCK (from Custom Message)"));
  }

  // --- Common cleanup/reset logic remains the same ---
  if ((displayMode == 0) && strlen(customMessage) > 0 && oldMode != 6) {
    displayMode = 6;
    Serial.println(F("[DISPLAY] Custom Message display before returning to CLOCK"));
  }
  lastSwitch = millis();
}


void advanceDisplayModeSafe() {
  int attempts = 0;
  const int MAX_ATTEMPTS = 7;  // Number of possible modes + 1
  int startMode = displayMode;
  bool valid = false;
  do {
    advanceDisplayMode();  // One step advance
    attempts++;
    // Recalculate validity for the new mode
    valid = false;
    String ntpField = String(ntpServer2);
    bool nightscoutConfigured = ntpField.startsWith("https://");

    if (displayMode == 0) valid = true;  // Clock always valid
    else if (displayMode == 5 && showDate) valid = true;
    else if (displayMode == 1 && weatherAvailable && (strlen(openWeatherApiKey) == 32) && (strlen(openWeatherCity) > 0) && (strlen(openWeatherCountry) > 0)) valid = true;
    else if (displayMode == 2 && showWeatherDescription && weatherAvailable && weatherDescription.length() > 0) valid = true;
    else if (displayMode == 3 && countdownEnabled && !countdownFinished && ntpSyncSuccessful) valid = true;
    else if (displayMode == 4 && nightscoutConfigured) valid = true;
    else if (displayMode == 6 && strlen(customMessage) > 0) valid = true;

    // If we've looped back to where we started, break to avoid infinite loop
    if (displayMode == startMode) break;

    if (valid) break;
  } while (attempts < MAX_ATTEMPTS);

  // If no valid mode found, fall back to Clock
  if (!valid) {
    displayMode = 0;
    Serial.println(F("[DISPLAY] Safe fallback to CLOCK"));
  }
  lastSwitch = millis();
}


//config save after countdown finishes
bool saveCountdownConfig(bool enabled, time_t targetTimestamp, const String &label) {
  DynamicJsonDocument doc(2048);

  File configFile = LittleFS.open("/config.json", "r");
  if (configFile) {
    DeserializationError err = deserializeJson(doc, configFile);
    configFile.close();
    if (err) {
      Serial.print(F("[saveCountdownConfig] Error parsing config.json: "));
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
    Serial.println(F("[saveCountdownConfig] ERROR: Cannot write to /config.json"));
    return false;
  }

  size_t bytesWritten = serializeJson(doc, f);
  f.close();

  Serial.printf("[saveCountdownConfig] Config updated. %u bytes written.\n", bytesWritten);
  return true;
}


void loop() {
  if (isAPMode) {
    dnsServer.processNextRequest();
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


  // AP Mode animation
  static unsigned long apAnimTimer = 0;
  static int apAnimFrame = 0;
  if (isAPMode) {
    unsigned long now = millis();
    if (now - apAnimTimer > 750) {
      apAnimTimer = now;
      apAnimFrame++;
    }
    P.setTextAlignment(PA_CENTER);
    switch (apAnimFrame % 3) {
      case 0: P.print(F("= ©")); break;
      case 1: P.print(F("= ª")); break;
      case 2: P.print(F("= «")); break;
    }
    yield();
    return;
  }

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

  // -----------------------------
  // Determine dimming start/end
  // -----------------------------
  int startTotal, endTotal;
  bool dimActive = false;

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
        Serial.printf("[DISPLAY] Automatic dimming setting brightness to %d\n", targetBrightness);
      else if (dimmingEnabled)
        Serial.printf("[DISPLAY] Custom dimming setting brightness to %d\n", targetBrightness);
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

    Serial.println("[SYSTEM] Countdown target reached! Switching to Mode 3 to display finish sequence.");
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
            Serial.printf("[TIME] NTP sync in progress (attempt %d of %d)...\n", ntpRetryCount + 1, maxNtpRetries);
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
  if ((displayMode == 0 || displayMode == 1) && millis() - lastSwitch > displayDuration) {
    advanceDisplayMode();
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


  const char *const *daysOfTheWeek = getDaysOfWeek(language);
  const char *daySymbol = daysOfTheWeek[timeinfo.tm_wday];

  // build base HH:MM first ---
  char baseTime[9];
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
    formattedTime = String(daySymbol) + "   " + String(timeSpacedStr);
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
  if ((displayMode == 0 || displayMode == 1) && (millis() - lastSwitch > currentDisplayDuration)) {
    advanceDisplayMode();
  }


  // --- CLOCK Display Mode ---
  if (displayMode == 0) {
    P.setCharSpacing(0);

    // --- NTP SYNC ---
    if (ntpState == NTP_SYNCING) {
      if (ntpSyncSuccessful || ntpRetryCount >= maxNtpRetries || millis() - ntpStartTime > ntpTimeout) {
        ntpState = NTP_FAILED;
      } else if (millis() - ntpAnimTimer > 750) {
        ntpAnimTimer = millis();
        switch (ntpAnimFrame % 3) {
          case 0: P.print(F("S Y N C ®")); break;
          case 1: P.print(F("S Y N C ¯")); break;
          case 2: P.print(F("S Y N C º")); break;
        }
        ntpAnimFrame++;
      }
    }
    // --- NTP / WEATHER ERROR ---
    else if (!ntpSyncSuccessful) {
      P.setTextAlignment(PA_CENTER);
      static unsigned long errorAltTimer = 0;
      static bool showNtpError = true;

      if (!ntpSyncSuccessful && !weatherAvailable) {
        if (millis() - errorAltTimer > 2000) {
          errorAltTimer = millis();
          showNtpError = !showNtpError;
        }
        P.print(showNtpError ? F("(<") : F("(*"));
      } else if (!ntpSyncSuccessful) {
        P.print(F("(<"));
      } else if (!weatherAvailable) {
        P.print(F("(*"));
      }
    }
    // --- DISPLAY CLOCK ---
    else {
      String timeString = formattedTime;
      if (showDayOfWeek && colonBlinkEnabled && !colonVisible) {
        timeString.replace(":", " ");
      }

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
        while (!P.displayAnimate()) yield();
        clockScrollDone = true;  // mark scroll done
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
    P.setCharSpacing(1);
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
        P.print(F("(*"));
      }
    }
    yield();
    return;
  }


  // --- WEATHER DESCRIPTION Display Mode ---
  if (displayMode == 2 && showWeatherDescription && weatherAvailable && weatherDescription.length() > 0) {
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
      if (P.displayAnimate()) {
        if (descScrollEndTime == 0) {
          descScrollEndTime = millis();  // mark the time when scroll finishes
        }
        // wait small pause after scroll stops
        if (millis() - descScrollEndTime > descriptionScrollPause) {
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
      yield();
      return;
    }
  }


  // --- Countdown Display Mode ---
  if (displayMode == 3 && countdownEnabled && ntpSyncSuccessful) {
    static int countdownSegment = 0;
    static unsigned long segmentStartTime = 0;
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
        Serial.println("[COUNTDOWN-FINISH] Countdown target invalid or not reached yet, skipping 'TIMES UP'. Advancing display.");
        advanceDisplayMode();
        yield();
        return;
      }

      // Define these static variables here if they are not global (or already defined in your loop())
      static const char *flashFrames[] = { "{|", "}~" };
      static unsigned long lastFlashingSwitch = 0;
      static int flashingMessageFrame = 0;

      // --- Initial Combined Sequence: Play Hourglass THEN start Flashing ---
      // This 'if' runs ONLY ONCE when the "finished" sequence begins.
      if (!hourglassPlayed) {                          // <-- This is the single entry point for the combined sequence
        countdownFinished = true;                      // Mark as finished overall
        countdownShowFinishedMessage = true;           // Confirm we are in the finished sequence
        countdownFinishedMessageStartTime = millis();  // Start the 15-second timer for the flashing duration

        // 1. Play Hourglass Animation (Blocking)
        const char *hourglassFrames[] = { "¡", "¢", "£", "¤" };
        for (int repeat = 0; repeat < 3; repeat++) {
          for (int i = 0; i < 4; i++) {
            P.setTextAlignment(PA_CENTER);
            P.setCharSpacing(0);
            P.print(hourglassFrames[i]);
            delay(350);  // This is blocking! (Total ~4.2 seconds for hourglass)
          }
        }
        Serial.println("[COUNTDOWN-FINISH] Played hourglass animation.");
        P.displayClear();  // Clear display after hourglass animation

        // 2. Initialize Flashing "TIMES UP" for its very first frame
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
      if (millis() - countdownFinishedMessageStartTime < 15000) {  // Flashing duration
        if (millis() - lastFlashingSwitch >= 500) {                // Check for flashing interval
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
        Serial.println("[COUNTDOWN-FINISH] Flashing duration over. Advancing to Clock.");
        countdownShowFinishedMessage = false;
        countdownFinishedMessageStartTime = 0;
        hourglassPlayed = false;  // <-- RESET this flag for the next countdown cycle!

        // Final cleanup (persisted)
        countdownEnabled = false;
        countdownTargetTimestamp = 0;
        countdownLabel[0] = '\0';
        saveCountdownConfig(false, 0, "");

        P.setInvert(false);
        advanceDisplayMode();
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
                Serial.printf("[COUNTDOWN-STATIC] Displaying segment %d: %s\n", countdownSegment, currentSegmentText.c_str());
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
                Serial.printf("[COUNTDOWN-STATIC] Displaying segment %d: %s\n", countdownSegment, currentSegmentText.c_str());
                countdownSegment++;
                break;
              }
            case 2:
              {  // Minutes
                char buf[10];
                sprintf(buf, "%02ld MINS", minutes);  // pad minutes with 0
                currentSegmentText = String(buf);
                Serial.printf("[COUNTDOWN-STATIC] Displaying segment %d: %s\n", countdownSegment, currentSegmentText.c_str());
                countdownSegment++;
                break;
              }
            case 3:
              {  // Seconds & Label Scroll
                time_t segmentStartTime = time(nullptr);
                unsigned long segmentStartMillis = millis();

                long nowRemaining = countdownTargetTimestamp - segmentStartTime;
                long currentSecond = nowRemaining % 60;
                char secondsBuf[10];
                sprintf(secondsBuf, "%02ld %s", currentSecond, currentSecond == 1 ? "SEC" : "SECS");
                String secondsText = String(secondsBuf);
                Serial.printf("[COUNTDOWN-STATIC] Displaying segment 3: %s\n", secondsText.c_str());
                P.displayClear();
                P.setTextAlignment(PA_CENTER);
                P.setCharSpacing(1);
                P.print(secondsText.c_str());
                delay(SEGMENT_DISPLAY_DURATION - 400);

                unsigned long elapsed = millis() - segmentStartMillis;
                long adjustedSecond = (countdownTargetTimestamp - segmentStartTime - (elapsed / 1000)) % 60;
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
                  yield();
                }
                countdownSegment++;
                segmentStartTime = millis();
                break;
              }
            case 4:  // Exit countdown
              Serial.println("[COUNTDOWN-STATIC] All segments and label displayed. Advancing to Clock.");
              countdownSegment = 0;
              segmentStartTime = 0;
              P.setTextAlignment(PA_CENTER);
              P.setCharSpacing(1);
              advanceDisplayMode();
              yield();
              return;

            default:
              Serial.println("[COUNTDOWN-ERROR] Invalid countdownSegment, resetting.");
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


  // // --- NIGHTSCOUT Display Mode ---
  if (displayMode == 4) {
    String ntpField = String(ntpServer2);

    // These static variables will retain their values between calls to this block
    static unsigned long lastNightscoutFetchTime = 0;
    const unsigned long NIGHTSCOUT_FETCH_INTERVAL = 150000;  // 2.5 minutes
    static int currentGlucose = -1;
    static String currentDirection = "?";
    static time_t lastGlucoseTime = 0;  // store timestamp from JSON

    // --- Small helper inside this block ---
    auto makeTimeUTC = [](struct tm *tm) -> time_t {
#if defined(ESP32)
      // ESP32: timegm() is not implemented — emulate correctly
      struct tm tm_copy = *tm;
      // mktime() interprets tm as local, but system time is UTC already
      // so we can safely assume input is UTC
      return mktime(&tm_copy);
#elif defined(ESP8266)
      // ESP8266: timegm() not available either, same logic
      struct tm tm_copy = *tm;
      return mktime(&tm_copy);
#else
      // Platforms with proper timegm()
      return timegm(tm);
#endif
    };
    // --------------------------------------

    // Check if it's time to fetch new data or if we have no data yet
    if (currentGlucose == -1 || millis() - lastNightscoutFetchTime >= NIGHTSCOUT_FETCH_INTERVAL) {
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient https;
      https.begin(client, ntpField);
      client.setBufferSizes(512, 512);
      https.setTimeout(5000);

      Serial.println("[HTTPS] Nightscout fetch initiated...");
      int httpCode = https.GET();

      if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error && doc.is<JsonArray>() && doc.size() > 0) {
          JsonObject firstReading = doc[0].as<JsonObject>();
          currentGlucose = firstReading["glucose"] | firstReading["sgv"] | -1;
          currentDirection = firstReading["direction"] | "?";
          const char *dateStr = firstReading["dateString"];

          // --- Parse ISO 8601 UTC time ---
          if (dateStr) {
            struct tm tm {};
            if (sscanf(dateStr, "%4d-%2d-%2dT%2d:%2d:%2dZ",
                       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                       &tm.tm_hour, &tm.tm_min, &tm.tm_sec)
                == 6) {
              tm.tm_year -= 1900;
              tm.tm_mon -= 1;
              lastGlucoseTime = makeTimeUTC(&tm);
            }
          }

          Serial.printf("Nightscout data fetched: %d mg/dL %s\n", currentGlucose, currentDirection.c_str());
        } else {
          Serial.println("Failed to parse Nightscout JSON");
        }
      } else {
        Serial.printf("[HTTPS] GET failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
      lastNightscoutFetchTime = millis();
    }

    // --- Display the data ---
    if (currentGlucose != -1) {
      // Calculate age of reading
      // Get current UTC time (avoid local timezone offset)
      time_t nowLocal = time(nullptr);
      struct tm *gmt = gmtime(&nowLocal);
      time_t nowUTC = mktime(gmt);

      bool isOutdated = false;
      int ageMinutes = 0;

      if (lastGlucoseTime > 0) {
        double diffSec = difftime(nowUTC, lastGlucoseTime);
        ageMinutes = (int)(diffSec / 60.0);
        isOutdated = (ageMinutes > NIGHTSCOUT_IDLE_THRESHOLD_MIN);
        Serial.printf("[NIGHTSCOUT] Data age: %d minutes old (threshold: %d)\n", ageMinutes, NIGHTSCOUT_IDLE_THRESHOLD_MIN);
      }

      // Pick arrow character
      char arrow;
      if (currentDirection == "Flat") arrow = 139;
      else if (currentDirection == "SingleUp") arrow = 134;
      else if (currentDirection == "DoubleUp") arrow = 135;
      else if (currentDirection == "SingleDown") arrow = 136;
      else if (currentDirection == "DoubleDown") arrow = 137;
      else if (currentDirection == "FortyFiveUp") arrow = 138;
      else if (currentDirection == "FortyFiveDown") arrow = 140;
      else arrow = '?';

      // Build display text
      String displayText = "";
      // ADD crossed digits
      if (isOutdated) {

        String glucoseStr = String(currentGlucose);

        for (int i = 0; i < glucoseStr.length(); i++) {
          if (isDigit(glucoseStr[i])) {
            int num = glucoseStr[i] - '0';           // 0–9
            glucoseStr[i] = 195 + ((num + 9) % 10);  // Maps 0→204, 1→195, ...
          }
        }

        String separatedStr = "";
        for (int i = 0; i < glucoseStr.length(); i++) {
          separatedStr += glucoseStr[i];
          if (i < glucoseStr.length() - 1) {
            separatedStr += char(255);  // insert separator between digits
          }
        }

        displayText += char(255);
        displayText += char(255);
        displayText += separatedStr;
        displayText += char(255);
        displayText += char(255);
        displayText += " ";  // extra space
        displayText += arrow;
        P.setCharSpacing(0);
      } else {
        displayText += String(currentGlucose) + String(arrow);
        P.setCharSpacing(1);
      }

      P.setTextAlignment(PA_CENTER);
      P.print(displayText.c_str());
      delay(weatherDuration);
      advanceDisplayMode();
      return;
    } else {
      P.setTextAlignment(PA_CENTER);
      P.setCharSpacing(0);
      P.print(F("())"));
      delay(2000);
      advanceDisplayMode();
      return;
    }
  }


  //DATE Display Mode
  else if (displayMode == 5 && showDate) {

    // --- VALID DATE CHECK ---
    if (timeinfo.tm_year < 120 || timeinfo.tm_mday <= 0 || timeinfo.tm_mon < 0 || timeinfo.tm_mon > 11) {
      advanceDisplayMode();
      return;  // skip drawing
    }
    // -------------------------
    String dateString;

    // Get localized month names
    const char *const *months = getMonthsOfYear(language);
    String monthAbbr = String(months[timeinfo.tm_mon]).substring(0, 5);
    monthAbbr.toLowerCase();

    // Add spaces between day digits
    String dayString = String(timeinfo.tm_mday);
    String spacedDay = "";
    for (size_t i = 0; i < dayString.length(); i++) {
      spacedDay += dayString[i];
      if (i < dayString.length() - 1) spacedDay += " ";
    }

    // Function to check if day should come first for given language
    auto isDayFirst = [](const String &lang) {
      // Languages with DD-MM order
      const char *dayFirstLangs[] = {
        "af",  // Afrikaans
        "cs",  // Czech
        "da",  // Danish
        "de",  // German
        "eo",  // Esperanto
        "es",  // Spanish
        "et",  // Estonian
        "fi",  // Finnish
        "fr",  // French
        "ga",  // Irish
        "hr",  // Croatian
        "hu",  // Hungarian
        "it",  // Italian
        "lt",  // Lithuanian
        "lv",  // Latvian
        "nl",  // Dutch
        "no",  // Norwegian
        "pl",  // Polish
        "pt",  // Portuguese
        "ro",  // Romanian
        "ru",  // Russian
        "sk",  // Slovak
        "sl",  // Slovenian
        "sr",  // Serbian
        "sv",  // Swedish
        "sw",  // Swahili
        "tr"   // Turkish
      };
      for (auto lf : dayFirstLangs) {
        if (lang.equalsIgnoreCase(lf)) {
          return true;
        }
      }
      return false;
    };

    String langForDate = String(language);

    if (langForDate == "ja") {
      // Japanese: month number (spaced digits) + day + symbol
      String spacedMonth = "";
      String monthNum = String(timeinfo.tm_mon + 1);
      dateString = monthAbbr + "  " + spacedDay + " ±";

    } else {
      if (isDayFirst(language)) {
        dateString = spacedDay + "   " + monthAbbr;
      } else {
        dateString = monthAbbr + "   " + spacedDay;
      }
    }

    P.setTextAlignment(PA_CENTER);
    P.setCharSpacing(0);
    P.print(dateString);

    if (millis() - lastSwitch > weatherDuration) {
      advanceDisplayMode();
    }
  }


  // --- Custom Message Display Mode (displayMode == 6) ---
  if (displayMode == 6) {

    // 1. Initial Check: If message is empty, skip mode 6.
    if (strlen(customMessage) == 0) {
      advanceDisplayMode();
      yield();
      return;
    }

    // --- CHARACTER REPLACEMENT AND PADDING (Common to both short and long) ---
    const size_t MAX_NON_SCROLLING_CHARS = 8;
    String msg = String(customMessage);

    // Replace standard digits 0–9 with your custom font character codes
    for (int i = 0; i < msg.length(); i++) {
      if (isDigit(msg[i])) {
        int num = msg[i] - '0';
        msg[i] = 145 + ((num + 9) % 10);
      }
    }

    // --- CHECK FOR TIMEOUT (Applies to temporary short & long messages) ---
    bool timedOut = false;
    // Check if a time limit (messageDisplaySeconds > 0) has been exceeded
    if (messageDisplaySeconds > 0 && (millis() - messageStartTime) >= (messageDisplaySeconds * 1000UL)) {
      Serial.printf("[MESSAGE] HA message timed out after %d seconds.\n", messageDisplaySeconds);
      timedOut = true;
    }

    // --- CHECK FOR SCROLL/CYCLE LIMIT BEFORE DISPLAYING ---
    // Scrolls complete applies to long messages.
    bool scrollsComplete = (messageScrollTimes > 0) && (currentScrollCount >= messageScrollTimes);

    // Cycles complete applies to short messages.
    extern int currentDisplayCycleCount;  // Use the dedicated short message counter
    bool cyclesComplete = (messageScrollTimes > 0) && (currentDisplayCycleCount >= messageScrollTimes);


    // --- ADVANCE MODE CHECK (Check if HA parameters are complete) ---
    // If either timer or cycle/scroll count is finished, we clean up the temporary message.
    if (scrollsComplete || cyclesComplete) {
      Serial.println(F("[MESSAGE] HA-controlled message finished."));

      // Reset common counters
      currentScrollCount = 0;
      messageStartTime = 0;
      currentDisplayCycleCount = 0;  // Reset the cycle counter

      // CRITICAL LOGIC: RESTORE PERSISTENT MESSAGE (Exit Mode 6 Logic)
      if (strlen(lastPersistentMessage) > 0) {
        // A persistent message exists, restore it
        strncpy(customMessage, lastPersistentMessage, sizeof(customMessage));
        messageScrollSpeed = GENERAL_SCROLL_SPEED;
        messageDisplaySeconds = 0;
        messageScrollTimes = 0;
        Serial.printf("[MESSAGE] Restored persistent message: '%s'. Staying in mode 6.\n", customMessage);
      } else {
        // No persistent message to restore. Clear the temporary HA message and Exit mode 6.
        customMessage[0] = '\0';
        Serial.println(F("[MESSAGE] No persistent message to restore. Advancing display mode."));
        advanceDisplayMode();
      }
      yield();
      return;
    }

    // ----------------------------------------------------------------------
    // BRANCH A: NON-SCROLLING (Short Message: strlen <= 8)
    // ----------------------------------------------------------------------
    if (msg.length() <= MAX_NON_SCROLLING_CHARS) {

      // Determine the duration: use HA seconds if set, otherwise use weatherDuration.
      unsigned long durationMs = (messageDisplaySeconds > 0)
                                   ? (messageDisplaySeconds * 1000UL)
                                   : weatherDuration;

      // If HA seconds is set, we use the timedOut check at the top.
      // If only scrollTimes is set, we still display for weatherDuration before incrementing the cycle count.

      Serial.printf("[MESSAGE] Displaying timed short message: '%s' for %lu ms. Advancing mode.\n", customMessage, durationMs);

      P.setTextAlignment(PA_CENTER);
      P.setCharSpacing(1);
      P.print(msg.c_str());

      // Block execution for the specified duration (non-HA uses weatherDuration)
      unsigned long displayUntil = millis() + durationMs;
      while (millis() < displayUntil) {
        yield();
      }

      // --- CYCLE TRACKING FOR SCROLLTIMES ---
      // Increment the counter if the HA message is configured to clear by scroll count.
      if (messageScrollTimes > 0) {
        currentDisplayCycleCount++;
        Serial.printf("[MESSAGE] Short message cycle complete. Count: %d/%d\n", currentDisplayCycleCount, messageScrollTimes);
      }

      // After display, the message content must persist, but the display must cycle.
      Serial.println(F("[MESSAGE] Short message duration complete. Advancing display mode."));
      advanceDisplayMode();
      yield();
      return;
    }

    // ----------------------------------------------------------------------
    // BRANCH B: SCROLLING (Long Message: strlen > 8) - (Existing Logic)
    // ----------------------------------------------------------------------

    // --- Determine if we need left padding based on previous mode ---
    bool addPadding = false;
    bool humidityVisible = showHumidity && weatherAvailable && strlen(openWeatherApiKey) == 32 && strlen(openWeatherCity) > 0 && strlen(openWeatherCountry) > 0;

    // If coming from CLOCK mode
    if (prevDisplayMode == 0 && (showDayOfWeek || colonBlinkEnabled)) {
      addPadding = true;
    } else if (prevDisplayMode == 1 && humidityVisible) {
      addPadding = true;
    }
    // Apply padding (4 spaces) if needed
    if (addPadding) {
      msg = "    " + msg;
    }

    // --- Display scrolling message ---
    P.setTextAlignment(PA_LEFT);
    P.setCharSpacing(1);
    textEffect_t actualScrollDirection = getEffectiveScrollDirection(PA_SCROLL_LEFT, flipDisplay);
    extern int messageScrollSpeed;

    // START SCROLL CYCLE
    P.displayScroll(msg.c_str(), PA_LEFT, actualScrollDirection, messageScrollSpeed);

    // BLOCKING WAIT: Completes 1 full scroll
    while (!P.displayAnimate()) yield();

    // SCROLL COUNT INCREMENT
    if (messageScrollTimes > 0) {
      currentScrollCount++;
      Serial.printf("[MESSAGE] Scroll complete. Count: %d/%d\n", currentScrollCount, messageScrollTimes);
    }

    // If no HA parameters are set, this is a persistent/infinite scroll, so advance mode after 1 scroll cycle.
    // If HA parameters ARE set, the mode relies on the check at the top to break out.
    if (messageDisplaySeconds == 0 && messageScrollTimes == 0) {
      P.setTextAlignment(PA_CENTER);
      advanceDisplayMode();
    }

    yield();
    return;
  }

  unsigned long currentMillis = millis();
  unsigned long runtimeSeconds = (currentMillis - bootMillis) / 1000;
  unsigned long currentTotal = totalUptimeSeconds + runtimeSeconds;

  // --- Log and save uptime every 10 minutes ---
  const unsigned long uptimeLogInterval = 600000UL;  // 10 minutes in ms

  if (currentMillis - lastUptimeLog >= uptimeLogInterval) {
    lastUptimeLog = currentMillis;
    Serial.printf("[UPTIME] Runtime: %s (total %.2f hours)\n",
                  formatUptime(currentTotal).c_str(), currentTotal / 3600.0);
    saveUptime();  // Save accumulated uptime every 10 minutes
  }
  yield();
}