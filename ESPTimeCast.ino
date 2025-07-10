#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <sntp.h>
#include <time.h>

#include "mfactoryfont.h"  // Custom font
#include "tz_lookup.h"     // Timezone lookup, do not duplicate mapping here!
#include "days_lookup.h"   // Languages for the Days of the Week

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 12
#define DATA_PIN 15
#define CS_PIN 13

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
AsyncWebServer server(80);

// WiFi and configuration globals
char ssid[32] = "";
char password[32] = "";
char openWeatherApiKey[64] = "";
char openWeatherCity[64] = "";
char openWeatherCountry[64] = "";
char weatherUnits[12] = "metric";
char timeZone[64] = "";
char language[8] = "en";

// Timing and display settings
unsigned long clockDuration = 10000;
unsigned long weatherDuration = 5000;
int brightness = 7;
bool flipDisplay = false;
bool twelveHourToggle = false;
bool showDayOfWeek = true;
bool showHumidity = false;
char ntpServer1[64] = "pool.ntp.org";
char ntpServer2[64] = "time.nist.gov";

// Dimming
bool dimmingEnabled = false;
int dimStartHour = 18;   // 6pm default
int dimStartMinute = 0;
int dimEndHour = 8;      // 8am default
int dimEndMinute = 0;
int dimBrightness = 2;   // Dimming level (0-15)

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
bool shouldFetchWeatherNow = false; // Flag to trigger immediate weather fetch

unsigned long lastSwitch = 0;
unsigned long lastColonBlink = 0;
int displayMode = 0;
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

// Non-blocking IP display globals
bool showingIp = false;
int ipDisplayCount = 0;
const int ipDisplayMax = 1;
String pendingIpToShow = "";


// -----------------------------------------------------------------------------
// Configuration Load & Save
// -----------------------------------------------------------------------------
void loadConfig() {
  Serial.println(F("[CONFIG] Loading configuration..."));

  if (!LittleFS.exists("/config.json")) {
    Serial.println(F("[CONFIG] config.json not found, creating with defaults..."));
    DynamicJsonDocument doc(512);
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
    doc[F("showHumidity")] = showHumidity;
    doc[F("ntpServer1")] = ntpServer1;
    doc[F("ntpServer2")] = ntpServer2;
    doc[F("dimmingEnabled")] = dimmingEnabled;
    doc[F("dimStartHour")] = dimStartHour;
    doc[F("dimEndHour")] = dimEndHour;
    doc[F("dimBrightness")] = dimBrightness;
    File f = LittleFS.open("/config.json", "w");
    if (f) {
      serializeJsonPretty(doc, f);
      f.close();
      Serial.println(F("[CONFIG] Default config.json created."));
    } else {
      Serial.println(F("[ERROR] Failed to create default config.json"));
    }
  }

  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[ERROR] Failed to open config.json for reading. Cannot load config."));
    return;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print(F("[ERROR] JSON parse failed during load: "));
    Serial.println(error.f_str());
    return;
  }

  strlcpy(ssid, doc["ssid"] | "", sizeof(ssid));
  strlcpy(password, doc["password"] | "", sizeof(password));
  strlcpy(openWeatherApiKey, doc["openWeatherApiKey"] | "", sizeof(openWeatherApiKey));
  strlcpy(openWeatherCity, doc["openWeatherCity"] | "", sizeof(openWeatherCity));
  strlcpy(openWeatherCountry, doc["openWeatherCountry"] | "", sizeof(openWeatherCountry));
  strlcpy(weatherUnits, doc["weatherUnits"] | "metric", sizeof(weatherUnits));
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
  showHumidity = doc["showHumidity"] | false;

  String de = doc["dimmingEnabled"].as<String>();
  dimmingEnabled = (de == "true" || de == "on" || de == "1");

  dimStartHour = doc["dimStartHour"] | 18;
  dimStartMinute = doc["dimStartMinute"] | 0;
  dimEndHour = doc["dimEndHour"] | 8;
  dimEndMinute = doc["dimEndMinute"] | 0;
  dimBrightness = doc["dimBrightness"] | 0;

  strlcpy(ntpServer1, doc["ntpServer1"] | "pool.ntp.org", sizeof(ntpServer1));
  strlcpy(ntpServer2, doc["ntpServer2"] | "time.nist.gov", sizeof(ntpServer2));

  if (strcmp(weatherUnits, "imperial") == 0)
    tempSymbol = ']';
  else
    tempSymbol = '[';
  Serial.println(F("[CONFIG] Configuration loaded."));

  if (doc.containsKey("showWeatherDescription"))
  showWeatherDescription = doc["showWeatherDescription"];
else
  showWeatherDescription = false;

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
    Serial.print(F("AP IP address: "));
    Serial.println(WiFi.softAPIP());
    isAPMode = true;
    Serial.println(F("[WIFI] AP Mode Started"));
    return;
  }

  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 25000;
  unsigned long animTimer = 0;
  int animFrame = 0;
  bool animating = true;

  while (animating) {
    unsigned long now = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("[WIFI] Connected: ") + WiFi.localIP().toString());
      isAPMode = false;
      animating = false;

      pendingIpToShow = WiFi.localIP().toString();
      showingIp = true;
      ipDisplayCount = 0;
      P.displayClear();
      P.setCharSpacing(1);
      P.displayScroll(pendingIpToShow.c_str(), PA_CENTER, PA_SCROLL_LEFT, 120);
      break;
    } else if (now - startAttemptTime >= timeout) {
      Serial.println(F("\r\n[WiFi] Failed. Starting AP mode..."));
      WiFi.softAP(AP_SSID, DEFAULT_AP_PASSWORD);
      Serial.print(F("AP IP address: "));
      Serial.println(WiFi.softAPIP());
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      isAPMode = true;
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
// Time/NTP Functions
// -----------------------------------------------------------------------------
void setupTime() {
  sntp_stop();
  if (!isAPMode) {
    Serial.println(F("[TIME] Starting NTP sync..."));
  }
  configTime(0, 0, ntpServer1, ntpServer2);
  setenv("TZ", ianaToPosix(timeZone), 1);
  tzset();
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
  Serial.print(F("WiFi SSID: ")); Serial.println(ssid);
  Serial.print(F("WiFi Password: ")); Serial.println(password);
  Serial.print(F("OpenWeather City: ")); Serial.println(openWeatherCity);
  Serial.print(F("OpenWeather Country: ")); Serial.println(openWeatherCountry);
  Serial.print(F("OpenWeather API Key: ")); Serial.println(openWeatherApiKey);
  Serial.print(F("Temperature Unit: ")); Serial.println(weatherUnits);
  Serial.print(F("Clock duration: ")); Serial.println(clockDuration);
  Serial.print(F("Weather duration: ")); Serial.println(weatherDuration);
  Serial.print(F("TimeZone (IANA): ")); Serial.println(timeZone);
  Serial.print(F("Days of the Week/Weather description language: ")); Serial.println(language);
  Serial.print(F("Brightness: ")); Serial.println(brightness);
  Serial.print(F("Flip Display: ")); Serial.println(flipDisplay ? "Yes" : "No");
  Serial.print(F("Show 12h Clock: ")); Serial.println(twelveHourToggle ? "Yes" : "No");
  Serial.print(F("Show Day of the Week: ")); Serial.println(showDayOfWeek ? "Yes" : "No");
  Serial.print(F("Show Weather Description: "));Serial.println(showWeatherDescription ? "Yes" : "No");
  Serial.print(F("Show Humidity ")); Serial.println(showHumidity ? "Yes" : "No");
  Serial.print(F("NTP Server 1: ")); Serial.println(ntpServer1);
  Serial.print(F("NTP Server 2: ")); Serial.println(ntpServer2);
  Serial.print(F("Dimming Enabled: ")); Serial.println(dimmingEnabled);
  Serial.print(F("Dimming Start Hour: ")); Serial.println(dimStartHour);
  Serial.print(F("Dimming Start Minute: ")); Serial.println(dimStartMinute);
  Serial.print(F("Dimming End Hour: ")); Serial.println(dimEndHour);
  Serial.print(F("Dimming End Minute: ")); Serial.println(dimEndMinute);
  Serial.print(F("Dimming Brightness: ")); Serial.println(dimBrightness);
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
    request->send(LittleFS, "/index.html", "text/html");
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
    doc[F("mode")] = isAPMode ? "ap" : "sta";
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // Save, restore, status and settings handlers grouped for clarity
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[WEBSERVER] Request: /save"));
    DynamicJsonDocument doc(2048);

    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      Serial.println(F("[WEBSERVER] Existing config.json found, loading..."));
      DeserializationError err = deserializeJson(doc, configFile);
      configFile.close();
      if (err) {
        Serial.print(F("[WEBSERVER] Error parsing existing config.json: "));
        Serial.println(err.f_str());
      }
    } else {
      Serial.println(F("[WEBSERVER] config.json not found, starting with empty doc."));
    }

    for (int i = 0; i < request->params(); i++) {
      const AsyncWebParameter *p = request->getParam(i);
      String n = p->name();
      String v = p->value();

      Serial.printf("[SAVE] Param: %s = %s\n", n.c_str(), v.c_str());

      if (n == "brightness") doc[n] = v.toInt();
      else if (n == "clockDuration") doc[n] = v.toInt();
      else if (n == "weatherDuration") doc[n] = v.toInt();
      else if (n == "flipDisplay") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "twelveHourToggle") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "showDayOfWeek") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "showHumidity") doc[n] = (v == "true" || v == "on" || v == "1");
      else if (n == "dimStartHour") doc[n] = v.toInt();
      else if (n == "dimStartMinute") doc[n] = v.toInt();
      else if (n == "dimEndHour") doc[n] = v.toInt();
      else if (n == "dimEndMinute") doc[n] = v.toInt();
      else if (n == "dimBrightness") doc[n] = v.toInt();
      else if (n == "showWeatherDescription") doc[n] = (v == "true" || v == "on" || v == "1");
      else doc[n] = v;
    }

    Serial.print(F("[SAVE] Document content before saving: "));
    serializeJson(doc, Serial);
    Serial.println();

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

    Serial.println(F("[SAVE] Attempting to open /config.json for verification."));
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

    Serial.println(F("[SAVE] Content of /config.json during verification read:"));
    while (verify.available()) {
      Serial.write(verify.read());
    }
    Serial.println();
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
    okDoc[F("message")] = "Saved successfully. Rebooting...";
    String response;
    serializeJson(okDoc, response);
    request->send(200, "application/json", response);
    Serial.println(F("[WEBSERVER] Sending success response and scheduling reboot..."));

    request->onDisconnect([]() {
      Serial.println(F("[WEBSERVER] Client disconnected, rebooting ESP..."));
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

  // Settings endpoints (brightness, flip, etc.)
  server.on("/set_brightness", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("value", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing value\"}");
      return;
    }
    int newBrightness = request->getParam("value", true)->value().toInt();
    if (newBrightness < 0) newBrightness = 0;
    if (newBrightness > 15) newBrightness = 15;
    brightness = newBrightness;
    P.setIntensity(brightness);
    Serial.printf("[WEBSERVER] Set brightness to %d\n", brightness);
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

  server.on("/set_language", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("value", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing value\"}");
      return;
    }
    String lang = request->getParam("value", true)->value();
    strlcpy(language, lang.c_str(), sizeof(language));
    Serial.printf("[WEBSERVER] Set language to %s\n", language);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/set_weatherdesc", HTTP_POST, [](AsyncWebServerRequest *request) {
  bool showDesc = false;
  if (request->hasParam("value", true)) {
    String v = request->getParam("value", true)->value();
    showDesc = (v == "1" || v == "true" || v == "on");
  }
  showWeatherDescription = showDesc;
  Serial.printf("[WEBSERVER] Set showWeatherDescription to %d\n", showWeatherDescription);
  request->send(200, "application/json", "{\"ok\":true}");
});

server.on("/set_units", HTTP_POST, [](AsyncWebServerRequest *request) {
  if (request->hasParam("value", true)) {
    String v = request->getParam("value", true)->value();
    if (v == "1" || v == "true" || v == "on") {
      strcpy(weatherUnits, "imperial");
      tempSymbol = ']'; // Fahrenheit symbol
    } else {
      strcpy(weatherUnits, "metric");
      tempSymbol = '['; // Celsius symbol
    }
    Serial.printf("[WEBSERVER] Set weatherUnits to %s\n", weatherUnits);
    shouldFetchWeatherNow = true;
    request->send(200, "application/json", "{\"ok\":true}");
  } else {
    request->send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
  }
});


  server.begin();
  Serial.println(F("[WEBSERVER] Web server started"));
}

void handleCaptivePortal(AsyncWebServerRequest *request) {
  Serial.print(F("[WEBSERVER] Captive Portal Redirecting: "));
  Serial.println(request->url());
  request->redirect(String("http://") + WiFi.softAPIP().toString() + "/");
}

// -----------------------------------------------------------------------------
// Weather Fetching and API settings
// -----------------------------------------------------------------------------
String getValidLang(String lang) {
  if (lang == "eo" || lang == "sw" || lang == "ja") {
    return "en";
  }
  return lang;
}

bool isNumber(const char* str) {
  for (int i = 0; str[i]; i++) {
    if (!isdigit(str[i]) && str[i] != '.' && str[i] != '-') return false;
  }
  return true;
}

bool isFiveDigitZip(const char* str) {
  if (strlen(str) != 5) return false;
  for (int i = 0; i < 5; i++) {
    if (!isdigit(str[i])) return false;
  }
  return true;
}

String buildWeatherURL() {
  String base = "http://api.openweathermap.org/data/2.5/weather?";

  float lat = atof(openWeatherCity);
  float lon = atof(openWeatherCountry);

  bool latValid = isNumber(openWeatherCity) && isNumber(openWeatherCountry) &&
                  lat >= -90.0 && lat <= 90.0 &&
                  lon >= -180.0 && lon <= 180.0;

  if (latValid) {
    base += "lat=" + String(lat, 8) + "&lon=" + String(lon, 8);
  } else if (isFiveDigitZip(openWeatherCity) &&
             String(openWeatherCountry).equalsIgnoreCase("US")) {
    base += "zip=" + String(openWeatherCity) + "," + String(openWeatherCountry);
  } else {
    base += "q=" + String(openWeatherCity) + "," + String(openWeatherCountry);
  }

  base += "&appid=" + String(openWeatherApiKey);
  base += "&units=" + String(weatherUnits);

  return base;
}

void fetchWeather() {
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
    return;
  }

  Serial.println(F("[WEATHER] Connecting to OpenWeatherMap..."));
  const char *host = "api.openweathermap.org";
  String url = buildWeatherURL();
  Serial.println(F("[WEATHER] URL: ") + url);

  IPAddress ip;
  if (!WiFi.hostByName(host, ip)) {
    Serial.println(F("[WEATHER] DNS lookup failed!"));
    weatherAvailable = false;
    return;
  }

  if (!client.connect(host, 80)) {
    Serial.println(F("[WEATHER] Connection failed"));
    weatherAvailable = false;
    return;
  }

  Serial.println(F("[WEATHER] Connected, sending request..."));
  String request = String("GET ") + url + " HTTP/1.1\r\n" + F("Host: ") + host + F("\r\n") + F("Connection: close\r\n\r\n");

  if (!client.print(request)) {
    Serial.println(F("[WEATHER] Failed to send request!"));
    client.stop();
    weatherAvailable = false;
    return;
  }

  unsigned long weatherStart = millis();
  const unsigned long weatherTimeout = 10000;

  bool isBody = false;
  String payload = "";
  String line = "";

  while ((client.connected() || client.available()) && millis() - weatherStart < weatherTimeout && WiFi.status() == WL_CONNECTED) {
    line = client.readStringUntil('\n');
    if (line.length() == 0) continue;

    if (line.startsWith(F("HTTP/1.1"))) {
      int statusCode = line.substring(9, 12).toInt();
      if (statusCode != 200) {
        Serial.print(F("[WEATHER] HTTP error: "));
        Serial.println(statusCode);
        client.stop();
        weatherAvailable = false;
        return;
      }
    }

    if (!isBody && line == F("\r")) {
      isBody = true;
      while (client.available()) {
        payload += (char)client.read();
      }
      break;
    }
    yield();
  }
  client.stop();

  if (millis() - weatherStart >= weatherTimeout) {
    Serial.println(F("[WEATHER] ERROR: Weather fetch timed out!"));
    weatherAvailable = false;
    return;
  }

  Serial.println(F("[WEATHER] Response received."));
  Serial.println(F("[WEATHER] Payload: ") + payload);

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("[WEATHER] JSON parse error: "));
    Serial.println(error.f_str());
    weatherAvailable = false;
    return;
  }

  if (doc.containsKey(F("main")) && doc[F("main")].containsKey(F("temp"))) {
    float temp = doc[F("main")][F("temp")];
    currentTemp = String((int)round(temp)) + "º";
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

  if (doc.containsKey(F("weather")) && doc[F("weather")].is<JsonArray>() && doc[F("weather")][0].containsKey(F("main"))) {
    const char *desc = doc[F("weather")][0][F("main")];
    Serial.printf("[WEATHER] Description: %s\n", desc);
    weatherDescription = String(desc);
  } else {
    Serial.println(F("[WEATHER] Weather description not found in JSON payload"));
  }
  weatherFetched = true;
}

// -----------------------------------------------------------------------------
// Main setup() and loop()
// -----------------------------------------------------------------------------

/*
DisplayMode key:
  0: Clock
  1: Weather
  2: Weather Description
*/
unsigned long descStartTime = 0;
bool descScrolling = false;
const unsigned long descriptionDuration = 3000; // 3s for short text

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[SETUP] Starting setup..."));

  if (!LittleFS.begin()) {
    Serial.println(F("[ERROR] LittleFS mount failed in setup! Halting."));
    while (true) {
      delay(1000);
    }
  }
  Serial.println(F("[SETUP] LittleFS file system mounted successfully."));

  P.begin();
  P.setCharSpacing(0);
  P.setFont(mFactory);
  loadConfig();
  P.setIntensity(brightness);
  P.setZoneEffect(0, flipDisplay, PA_FLIP_UD);
  P.setZoneEffect(0, flipDisplay, PA_FLIP_LR);
  Serial.println(F("[SETUP] Parola (LED Matrix) initialized"));
  connectWiFi();
  Serial.println(F("[SETUP] Wifi connected"));
  setupWebServer();
  Serial.println(F("[SETUP] Webserver setup complete"));
  Serial.println(F("[SETUP] Setup complete"));
  Serial.println();
  printConfigToSerial();
  setupTime();
  displayMode = 0;
  lastSwitch = millis();
  lastColonBlink = millis();
}

void advanceDisplayMode() {
  int oldMode = displayMode;
  if (displayMode == 0) {
    displayMode = 1; // clock -> weather
  } else if (displayMode == 1 && showWeatherDescription && weatherAvailable && weatherDescription.length() > 0) {
    displayMode = 2; // weather -> description
  } else {
    displayMode = 0; // description (or weather if no desc) -> clock
  }
  lastSwitch = millis();
  // Serial print for debugging
  const char* modeName = displayMode == 0 ? "CLOCK" :
                         displayMode == 1 ? "WEATHER" : "DESCRIPTION";
  Serial.printf("[LOOP] Switching to display mode: %s\n", modeName);
}

void loop() {
  if (isAPMode) {
    dnsServer.processNextRequest();
  }

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

  // Dimming
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  int curHour = timeinfo.tm_hour;
  int curMinute = timeinfo.tm_min;
  int curTotal = curHour * 60 + curMinute;
  int startTotal = dimStartHour * 60 + dimStartMinute;
  int endTotal = dimEndHour * 60 + dimEndMinute;
  bool isDimming = false;

  if (dimmingEnabled) {
    if (startTotal < endTotal) {
      isDimming = (curTotal >= startTotal && curTotal < endTotal);
    } else {
      isDimming = (curTotal >= startTotal || curTotal < endTotal);
    }
    if (isDimming) {
      P.setIntensity(dimBrightness);
    } else {
      P.setIntensity(brightness);
    }
  } else {
    P.setIntensity(brightness);
  }

  // Show IP after WiFi connect
  if (showingIp) {
    if (P.displayAnimate()) {
      ipDisplayCount++;
      if (ipDisplayCount < ipDisplayMax) {
        P.displayScroll(pendingIpToShow.c_str(), PA_CENTER, PA_SCROLL_LEFT, 120);
      } else {
        showingIp = false;
        P.displayClear();
        delay(500);
        displayMode = 0;
        lastSwitch = millis();
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
  const unsigned long fetchInterval = 300000; // 5 minutes

  switch (ntpState) {
    case NTP_IDLE: break;
    case NTP_SYNCING: {
      time_t now = time(nullptr);
      if (now > 1000) {
        Serial.println(F("\n[TIME] NTP sync successful."));
        ntpSyncSuccessful = true;
        ntpState = NTP_SUCCESS;
      } else if (millis() - ntpStartTime > ntpTimeout || ntpRetryCount > maxNtpRetries) {
        Serial.println(F("\n[TIME] NTP sync failed."));
        ntpSyncSuccessful = false;
        ntpState = NTP_FAILED;
      } else {
        if (millis() - ntpStartTime > ((unsigned long)ntpRetryCount * 1000)) {
          Serial.print(F("."));
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
      break;
  }

  // --- MODIFIED WEATHER FETCHING LOGIC ---
  if (WiFi.status() == WL_CONNECTED) {
    // Check if an immediate fetch is requested OR if the regular interval has passed
    if (!weatherFetchInitiated || shouldFetchWeatherNow || (millis() - lastFetch > fetchInterval)) {
      if (shouldFetchWeatherNow) {
        Serial.println(F("[LOOP] Immediate weather fetch requested by web server."));
        shouldFetchWeatherNow = false; // Reset the flag after handling
      } else if (!weatherFetchInitiated) {
        Serial.println(F("[LOOP] Initial weather fetch."));
      } else {
        Serial.println(F("[LOOP] Regular interval weather fetch."));
      }
      
      weatherFetchInitiated = true;
      weatherFetched = false; // Mark as not yet fetched
      fetchWeather();
      lastFetch = millis();
    }
  } else {
    weatherFetchInitiated = false;
    // It's good practice to reset the flag if WiFi disconnects to avoid stale requests
    shouldFetchWeatherNow = false; 
  }
  // --- END MODIFIED WEATHER FETCHING LOGIC ---


  const char *const *daysOfTheWeek = getDaysOfWeek(language);
  const char *daySymbol = daysOfTheWeek[timeinfo.tm_wday];

  char timeStr[9];
  if (twelveHourToggle) {
    int hour12 = timeinfo.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    sprintf(timeStr, " %d:%02d", hour12, timeinfo.tm_min);
  } else {
    sprintf(timeStr, " %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }

  char timeSpacedStr[20];
  int j = 0;
  for (int i = 0; timeStr[i] != '\0'; i++) {
    timeSpacedStr[j++] = timeStr[i];
    if (timeStr[i + 1] != '\0') {
      timeSpacedStr[j++] = ' ';
    }
  }
  timeSpacedStr[j] = '\0';

  String formattedTime;
  if (showDayOfWeek) {
    formattedTime = String(daySymbol) + " " + String(timeSpacedStr);
  } else {
    formattedTime = String(timeSpacedStr);
  }

  // --- Weather Description Mode handling ---
  static unsigned long descStartTime = 0;
  static bool descScrolling = false;
  static unsigned long descScrollEndTime = 0;  // for post-scroll delay
  const unsigned long descriptionDuration = 3000; // 3s for short text
  const unsigned long descriptionScrollPause = 300; // 300ms pause after scroll

  // Only advance mode by timer for clock/weather, not description!
  unsigned long displayDuration = (displayMode == 0) ? clockDuration : weatherDuration;
  if ((displayMode == 0 || displayMode == 1) && millis() - lastSwitch > displayDuration) {
    advanceDisplayMode();
  }

  // --- WEATHER DESCRIPTION Display Mode ---
  if (displayMode == 2 && showWeatherDescription && weatherAvailable && weatherDescription.length() > 0) {
    String desc = weatherDescription;
    desc.toUpperCase();

    if (desc.length() > 8) {
      if (!descScrolling) {
        P.displayClear();
        P.displayScroll(desc.c_str(), PA_CENTER, PA_SCROLL_LEFT, 100);
        descScrolling = true;
        descScrollEndTime = 0; // reset end time at start
      }
      if (P.displayAnimate()) {
        if (descScrollEndTime == 0) {
          descScrollEndTime = millis(); // mark the time when scroll finishes
        }
        // wait small pause after scroll stops
        if (millis() - descScrollEndTime > descriptionScrollPause) {
          descScrolling = false;
          descScrollEndTime = 0;
          advanceDisplayMode();
        }
      } else {
        descScrollEndTime = 0; // reset if not finished
      }
      yield();
      return;
    } else {
      if (descStartTime == 0) {
        P.setTextAlignment(PA_CENTER);
        P.setCharSpacing(1);
        P.print(desc.c_str());
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

  static bool weatherWasAvailable = false;
  // --- CLOCK Display Mode ---
  if (displayMode == 0) {
    P.setCharSpacing(0);
    if (ntpState == NTP_SYNCING) {
      if (millis() - ntpAnimTimer > 750) {
        ntpAnimTimer = millis();
        switch (ntpAnimFrame % 3) {
          case 0: P.print(F("S Y N C ®")); break;
          case 1: P.print(F("S Y N C ¯")); break;
          case 2: P.print(F("S Y N C °")); break;
        }
        ntpAnimFrame++;
      }
    } else if (!ntpSyncSuccessful) {
      P.setTextAlignment(PA_CENTER);
      P.print(F("?/"));
    } else {
      String timeString = formattedTime;
      if (!colonVisible) timeString.replace(":", " ");
      P.print(timeString);
    }
    yield();
    return;
  }

  // --- WEATHER Display Mode ---
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
        P.print(F("?*"));
      }
    }
    yield();
    return;
  }

  yield();
}