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

char ssid[32] = "";
char password[32] = "";
char openWeatherApiKey[64] = "";
char openWeatherCity[64] = "";
char openWeatherCountry[64] = "";
char weatherUnits[12] = "metric";
char timeZone[64] = "";
char language[8] = "en";

unsigned long clockDuration = 10000;
unsigned long weatherDuration = 5000;

// ADVANCED SETTINGS
int brightness = 7;
bool flipDisplay = false;
bool twelveHourToggle = false;
bool showDayOfWeek = true;
bool showHumidity = false;
char ntpServer1[64] = "pool.ntp.org";
char ntpServer2[64] = "time.nist.gov";

// DIMMING SETTINGS
bool dimmingEnabled = false;
int dimStartHour = 18;  // 6pm default
int dimStartMinute = 0;
int dimEndHour = 8;  // 8am default
int dimEndMinute = 0;
int dimBrightness = 2;  // Dimming level (0-15)

bool weatherCycleStarted = false; 

WiFiClient client;

const byte DNS_PORT = 53;
DNSServer dnsServer;

String currentTemp = "";
String weatherDescription = "";
bool weatherAvailable = false;
bool weatherFetched = false;
bool weatherFetchInitiated = false;
bool isAPMode = false;
char tempSymbol = 'C';

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

// --- Globals for non-blocking IP display ---
bool showingIp = false;
int ipDisplayCount = 0;      // How many times IP has been shown
const int ipDisplayMax = 1;  // Number of repeats
String pendingIpToShow = "";

void loadConfig() {
  Serial.println(F("[CONFIG] Loading configuration..."));

  if (!LittleFS.exists("/config.json")) {
    Serial.println(F("[CONFIG] config.json not found, creating with defaults..."));
    DynamicJsonDocument doc(512);  // Sufficient for initial defaults
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

  DynamicJsonDocument doc(2048);                                  // Use 2048 to match the save handler's capacity
  DeserializationError error = deserializeJson(doc, configFile);  // Read directly from file
  configFile.close();                                             // Close after reading

  if (error) {
    Serial.print(F("[ERROR] JSON parse failed during load: "));
    Serial.println(error.f_str());
    return;
  }

  // Populate global variables from loaded JSON, using default values if keys are missing
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
    strlcpy(language, "en", sizeof(language));  // Fallback if key missing
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
    tempSymbol = 'F';
  else if (strcmp(weatherUnits, "standard") == 0)
    tempSymbol = 'K';
  else
    tempSymbol = 'C';
  Serial.println(F("[CONFIG] Configuration loaded."));
}

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

      // --- NON-BLOCKING: Schedule IP display in loop() for 1 repeat ---
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
        case 0: P.print(F("W @ F @ ©")); break;
        case 1: P.print(F("W @ F @ ª")); break;
        case 2: P.print(F("W @ F @ «")); break;
      }
      animFrame++;
    }
    yield();
  }
}

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
  ntpSyncSuccessful = false;  // Reset the flag
}

String getValidLang(String lang) {
  // List of unsupported codes
  if (lang == "eo" || lang == "sw" || lang == "ja") {
    return "en";  // fallback to English
  }
  return lang;  // supported language, return as is
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
    // Latitude/Longitude query
    base += "lat=" + String(lat, 8) + "&lon=" + String(lon, 8);
  } else if (isFiveDigitZip(openWeatherCity) &&
             String(openWeatherCountry).equalsIgnoreCase("US")) {
    // US ZIP code query
    base += "zip=" + String(openWeatherCity) + "," + String(openWeatherCountry);
  } else {
    // City name and country code
    base += "q=" + String(openWeatherCity) + "," + String(openWeatherCountry);
  }

  base += "&appid=" + String(openWeatherApiKey);
  base += "&units=" + String(weatherUnits);
  base += "&lang=" + getValidLang(language);  // Optional, safe fallback

  return base;
}



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
  Serial.print(F("Show Humidity "));
  Serial.println(showHumidity ? "Yes" : "No");
  Serial.print(F("NTP Server 1: "));
  Serial.println(ntpServer1);
  Serial.print(F("NTP Server 2: "));
  Serial.println(ntpServer2);
  Serial.print(F("Dimming Enabled: "));
  Serial.println(dimmingEnabled);
  Serial.print(F("Dimming Start Hour: "));
  Serial.println(dimStartHour);
  Serial.print(F("Dimming Start Minute: "));
  Serial.println(dimStartMinute);
  Serial.print(F("Dimming End Hour: "));
  Serial.println(dimEndHour);
  Serial.print(F("Dimming End Minute: "));
  Serial.println(dimEndMinute);
  Serial.print(F("Dimming Brightness: "));
  Serial.println(dimBrightness);
  Serial.println(F("========================================"));
  Serial.println();
}



// This tells the compiler that handleCaptivePortal exists somewhere later in the code.
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

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println(F("[WEBSERVER] Request: /save"));

    DynamicJsonDocument doc(2048);

    // Load existing config.json into the document
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      Serial.println(F("[WEBSERVER] Existing config.json found, loading..."));
      DeserializationError err = deserializeJson(doc, configFile);
      configFile.close();
      if (err) {
        // Log the error but proceed, allowing the new config to potentially fix it
        Serial.print(F("[WEBSERVER] Error parsing existing config.json: "));
        Serial.println(err.f_str());
      }
    } else {
      Serial.println(F("[WEBSERVER] config.json not found, starting with empty doc."));
    }

    // Iterate through incoming parameters from the web form and update the document
    for (int i = 0; i < request->params(); i++) {
      const AsyncWebParameter *p = request->getParam(i);
      String n = p->name();
      String v = p->value();

      Serial.printf("[SAVE] Param: %s = %s\n", n.c_str(), v.c_str());

      // Specific type casting for known boolean/integer fields
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
      else doc[n] = v;  // Generic for all other string parameters
    }

    // --- DEBUGGING CODE ---
    Serial.print(F("[SAVE] Document content before saving: "));
    serializeJson(doc, Serial);  // Print the JSON document to Serial
    Serial.println();

    // Get file system info for ESP8266
    FSInfo fs_info;
    LittleFS.info(fs_info);
    Serial.printf("[SAVE] LittleFS total bytes: %u, used bytes: %u\n", fs_info.totalBytes, fs_info.usedBytes);
    // --- END DEBUGGING CODE ---

    // Save the updated doc
    if (LittleFS.exists("/config.json")) {
      Serial.println(F("[SAVE] Renaming /config.json to /config.bak"));
      LittleFS.rename("/config.json", "/config.bak");  // Create a backup
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
    f.close();  // Close the file to ensure data is flushed
    Serial.println(F("[SAVE] /config.json file closed."));

    // Verification step
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

    // --- DEBUGGING CODE ---
    Serial.println(F("[SAVE] Content of /config.json during verification read:"));
    // Read and print the content character by character
    while (verify.available()) {
      Serial.write(verify.read());
    }
    Serial.println();  // Newline after file content
    verify.seek(0);    // Reset file pointer to beginning for deserializeJson
    // --- END DEBUGGING CODE ---

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

  server.begin();
  Serial.println(F("[WEBSERVER] Web server started"));
}

// --- handleCaptivePortal FUNCTION DEFINITION ---
void handleCaptivePortal(AsyncWebServerRequest *request) {
  Serial.print(F("[WEBSERVER] Captive Portal Redirecting: "));
  Serial.println(request->url());
  request->redirect(String("http://") + WiFi.softAPIP().toString() + "/");
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
      // Read the entire body at once
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
    const char *desc = doc[F("weather")][0][F("description")];
    Serial.printf("[WEATHER] Description: %s\n", desc);
  } else {
    Serial.println(F("[WEATHER] Weather description not found in JSON payload"));
  }
  weatherFetched = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[SETUP] Starting setup..."));

  if (!LittleFS.begin()) {
    Serial.println(F("[ERROR] LittleFS mount failed in setup! Halting."));
    while (true) {  // Halt execution if file system cannot be mounted
      delay(1000);
    }
  }
  Serial.println(F("[SETUP] LittleFS file system mounted successfully."));

  P.begin();
  P.setCharSpacing(0);
  P.setFont(mFactory);  // Custom font
  loadConfig();         // Load config before setting intensity & flip
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
  setupTime();  // Start NTP sync process
  displayMode = 0;
  lastSwitch = millis();
  lastColonBlink = millis();
}

void loop() {
  if (isAPMode) {
    dnsServer.processNextRequest();
  }

  // --- AP Mode Animation remains unchanged ---
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
      case 0: P.print(F("A P ©")); break;
      case 1: P.print(F("A P ª")); break;
      case 2: P.print(F("A P «")); break;
    }
    yield();
    return;
  }

  // --- Dimming logic with hour and minute ---
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
      // Dimming in same day (e.g. 18:45 to 23:00)
      isDimming = (curTotal >= startTotal && curTotal < endTotal);
    } else {
      // Dimming overnight (e.g. 18:45 to 08:30)
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

  // --- NON-BLOCKING: Show IP after WiFi connect for 1 scrolls, then resume normal display ---
  if (showingIp) {
    if (P.displayAnimate()) {
      ipDisplayCount++;
      if (ipDisplayCount < ipDisplayMax) {
        // Scroll again
        P.displayScroll(pendingIpToShow.c_str(), PA_CENTER, PA_SCROLL_LEFT, 120);
      } else {
        // Done showing IP, resume normal display
        showingIp = false;        
        P.displayClear();
        delay(500);
        displayMode = 0;        // Force clock mode
        lastSwitch = millis();  // Reset timer so clock mode gets full duration
      }
    }
    yield();
    return;  // Skip normal display logic while showing IP
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

  switch (ntpState) {
    case NTP_IDLE: break;
    case NTP_SYNCING:
      {
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

  if (WiFi.status() == WL_CONNECTED) {
    if (!weatherFetchInitiated) {
      weatherFetchInitiated = true;
      fetchWeather();
      lastFetch = millis();
    }
    if (millis() - lastFetch > fetchInterval) {
      Serial.println(F("[LOOP] Fetching weather data..."));
      weatherFetched = false;
      fetchWeather();
      lastFetch = millis();
    }
  } else {
    weatherFetchInitiated = false;
  }

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

  unsigned long displayDuration = (displayMode == 0) ? clockDuration : weatherDuration;
  if (millis() - lastSwitch > displayDuration) {
    displayMode = (displayMode + 1) % 2;
    lastSwitch = millis();
    Serial.printf("[LOOP] Switching to display mode: %s\n", displayMode == 0 ? "CLOCK" : "WEATHER");
  }

  P.setTextAlignment(PA_CENTER);
  static bool weatherWasAvailable = false;

  if (displayMode == 0) {
    P.setCharSpacing(0);
    if (ntpState == NTP_SYNCING) {
      if (millis() - ntpAnimTimer > 750) {
        ntpAnimTimer = millis();
        switch (ntpAnimFrame % 3) {
          case 0: P.print(F("$ ®")); break;
          case 1: P.print(F("$ ¯")); break;
          case 2: P.print(F("$ °")); break;
        }
        ntpAnimFrame++;
      }
    } else if (!ntpSyncSuccessful) {
      P.print(F("? /"));
    } else {
      String timeString = formattedTime;
      if (!colonVisible) timeString.replace(":", " ");
      P.print(timeString);
    }
  } else {
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
        P.print(F("? *"));
      }
    }
  }

  yield();
}
