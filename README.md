![ESPTimeCast](assets/logo.svg)

![GitHub stars](https://img.shields.io/github/stars/mfactory-osaka/ESPTimeCast?style=social)
![GitHub forks](https://img.shields.io/github/forks/mfactory-osaka/ESPTimeCast?style=social)
![Last Commit](https://img.shields.io/github/last-commit/mfactory-osaka/ESPTimeCast)

**ESPTimeCast** is a WiFi-connected LED matrix clock and weather station based on ESP8266/ESP32 and MAX7219.  
It displays the current time, day of the week, and local weather (temp/humidity/weather description) fetched from OpenWeatherMap.  
Setup and configuration are fully managed via a built-in web interface.  

![clock - weather](assets/demo.gif) 


## 📦 3D Printable Case

Want to give your ESPTimeCast a home? You can 3D print a custom case for it!  
A styilish version (V2) of the case has just been released!   
The case front panel (3mm) can be laser cut!

<img src="assets/image01.png" alt="3D Printable Case V1" width="320" />
<img src="assets/image02.png" alt="3D Printable Case V2" width="320" />

[![Printables Downloads](https://img.shields.io/badge/Printables-132%20Downloads-orange?logo=prusa)](https://www.printables.com/model/1344276-esptimecast-wi-fi-clock-weather-display)
[![Cults3D Downloads](https://img.shields.io/badge/Cults3D-66%20Downloads-blue?logo=cults3d)](https://cults3d.com/en/3d-model/gadget/wifi-connected-led-matrix-clock-and-weather-station-esp8266-and-max7219)

---

## ✨ Features

- **LED Matrix Display (8x32)** powered by MAX7219, with custom font support
- **Simple Web Interface** for all configuration (WiFi, weather, time zone, display durations, and more)
- **Automatic NTP Sync** with robust status feedback and retries
- **Weather Fetching** from OpenWeatherMap (every 5 minutes, temp/humidity/description)
- **Fallback AP Mode** for easy first-time setup or configuration
- **Timezone Selection** from IANA names (DST integrated on backend)
- **Get My Location** button to get your approximate Lat/Long.
- **Week Day and Weather Description display** in multiple languages
- **Persistent Config** stored in LittleFS, with backup/restore system
- **Status Animations** for WiFi connection, AP mode, time syncing.
- **Advanced Settings** panel with:
  - Custom **Primary/Secondary NTP server** input
  - Display **Day of the Week** toggle (default is on)
  - Display **Blinking Colon** toggle (default is on)
  - Show **Date** toggle (default is off)
  - **24/12h clock mode** toggle (24-hour default)
  - **Imperial Units (°F)** toggle (metric °C defaults)
  - Show **Humidity** toggle (display Humidity besides Temperature)
  - **Weather description** toggle (displays: heavy rain, scattered clouds, thunderstorm etc.)
  - **Flip display** (180 degrees)
  - Adjustable display **brightness**
  - Dimming Hours **Scheduling**
  - **Countdown** function (Scroll / Dramatic)
  - Optional **glucose + trend** display (Nightscout-compatible, set via ntpserver2)
    
---

## 🪛 Wiring



**Power Supply Change** Switching from 3.3V to 5V for Display

 Note: although the pins are labeled differently in the V4 and the S2, the positions are the same as the V3.x

**Wemos D1 Mini (ESP8266) → MAX7219**  
**Wemos S2 Mini (ESP32) → MAX7219**

| Wemos D1 Mini (v3.x) | Wemos D1 Mini (v4.0) | Wemos S2 Mini | MAX7219 |
|:------:|:------:|:------:|:------|
|  GND  |  GND  |  GND  |   GND  |
|  D6   |  12  |  9  |   CLK  |
|  D7   |  13  |  11  |   CS   |
|  D8   |  15  |  12  |   DIN  |
|  5V  |  5V  |  5V  |   VCC  | 

<img src="assets/wiring2.png" alt="Wiring" width="800" />

**Important hardware update:**  
After observing overheating issues and unstable behavior when powering the MAX7219 matrix from the Mini D1’s 3.3V pin, we’re officially switching to powering the display via the 5V USB rail instead.  

**What’s changing:**
- Before: Display VCC was connected to 3.3V pin on the ESP board.
- Now: Display VCC will be connected to the board’s 5V pin (which comes directly from USB power).
  
**Why this change is needed:**
- The MAX7219 LED matrix is designed for 5V operation.  
- The onboard 3.3V regulator (usually an AMS1117) on the Mini D1 is very limited in current output (~800mA max, often much less in practice).  
- High-brightness matrix modules — especially green/yellow displays — can draw enough current to overload the regulator, causing:
  - Overheating
  - Voltage drop
  - Complete regulator failure (some users reported only 2.4V output after damage)

**Benefits of using 5V:**
- Higher brightness and more stable matrix performance
- Reduced heat load on the ESP8266 board
- Avoid long-term damage to the onboard regulator
- The MAX7219 works fine with 3.3V logic signals from the ESP (no need for level shifters)

Note: Thanks to @Wood578Guy for the info on V4

---

## 🌐 Web UI & Configuration

The built-in web interface provides full configuration for:

- **WiFi settings** (SSID & Password)
- **Weather settings** (OpenWeatherMap API key, City, Country, Coordinates)
- **Time zone** (will auto-populate if TZ is found)
- **Day of the Week and Weather Description** languages
- **Display durations** for clock and weather (milliseconds)
- **Advanced Settings** (see below)

### First-time Setup / AP Mode

1. Power on the device. If WiFi fails, it auto-starts in AP mode:
   - **SSID:** `ESPTimeCast`
   - **Password:** `12345678`
   - Open `http://192.168.4.1` or `http://setup.esp` in your browser.
2. Set your WiFi and all other options.
3. Click **Save Setting** – the device saves config, reboots, and connects.
4. The device shows its local IP address after boot so you can login again for setting changes

*External links and the "Get My Location" button require internet access.  
They won't work while the device is in AP Mode - connect to Wi-Fi first.

### UI Example:
<img src="assets/webui6.png" alt="Web Interface" width="320">

---

## ⚙️ Advanced Settings

Click the **cog icon** next to “Advanced Settings” in the web UI to reveal extra configuration options.  

**Available advanced settings:**

- **Primary NTP Server**: Override the default NTP server (e.g. `pool.ntp.org`)
- **Secondary NTP Server**: Fallback NTP server (e.g. `time.nist.gov`)
- **Day of the Week**: Display Day of the Week in the desired language
- **Blinking Colon** toggle (default is on)
- **Show Date** (default is off, duration is the same as weather duration)
- **24/12h Clock**: Switch between 24-hour and 12-hour time formats (24-hour default)
- **Imperial Units (°F)** toggle (metric °C defaults)
- **Humidity**: Display Humidity besides Temperature
- **Weather description** toggle (display weather description in the selected language* for 3 seconds or scrolls once if description is too long)
- **Flip Display**: Invert the display vertically/horizontally
- **Brightness**: 0 (dim) to 15 (bright)
- **Dimming Feature**: Start time, end time and desired brightness selection
- **Countdown** function, set a countdown to your favorit/next event, 2 modes: Scroll/Dramatic! 

*Non-English characters converted to their closest English alphabet.   
*For Esperanto, Irish, and Swahili, weather description translations are not available. Japanese translations exist, but since the device cannot display all Japanese characters, English will be used in all these cases.  

Tip: Don't forget to press the save button to keep your settings

---

## 📝 Configuration Notes

- **OpenWeatherMap API Key:**
   - [Make an account here](https://home.openweathermap.org/users/sign_up)
   - [Check your API key here](https://home.openweathermap.org/api_keys)
- **City Name:** e.g. `Tokyo`, `London`, etc.
- **Country Code:** 2-letter code (e.g., `JP`, `GB`)
- **ZIP Code:** Enter your ZIP code in the city field and US in the country field (US only)
- **Latitude and Longitude** You can enter coordinates in the city field (lat.) and country field (long.)
- **Time Zone:** Select from IANA zones (e.g., `America/New_York`, handles DST automatically)

---

## 🚀 Getting Started

This guide will walk you through setting up your environment and uploading the **ESPTimeCast** project to your **ESP8266** or **ESP32** board. Please follow the instructions carefully for your specific board type.

---

### ⚙️ ESP8266 Setup

Follow these steps to prepare your Arduino IDE for ESP8266 development:

1.  **Install ESP8266 Board Package:**
    * Open `File > Preferences` in Arduino IDE.
    * Add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to "Additional Boards Manager URLs."
    * Go to `Tools > Board > Boards Manager...`. Search for `esp8266` by `ESP8266 Community` and click "Install".
2.  **Select Your Board:**
    * Go to `Tools > Board` and select your specific board, e.g., **Wemos D1 Mini** (or your ESP8266 variant).
3.  **Configure Flash Size:**
    * Under `Tools`, select `Flash Size "4MB FS:2MB OTA:~1019KB"`. This ensures enough space for the sketch and LittleFS data.
4.  **Install Libraries:**
    * Go to `Sketch > Include Library > Manage Libraries...` and install the following:
        * `ArduinoJson` by Benoit Blanchon
        * `MD_Parola` by majicDesigns (this will typically also install its dependency: `MD_MAX72xx`)
        * `ESPAsyncTCP` by ESP32Async
        * `ESPAsyncWebServer` by ESP32Async

---

### ⚙️ ESP32 Setup

Follow these steps to prepare your Arduino IDE for ESP32 development:

1.  **Install ESP32 Board Package:**
    * Go to `Tools > Board > Boards Manager...`. Search for `esp32` by `Espressif Systems` and click "Install".
2.  **Select Your Board:**
    * Go to `Tools > Board` and select your specific board, e.g., **LOLIN S2 Mini** (or your ESP32 variant).
3.  **Configure Partition Scheme:**
    * Under `Tools`, select `Partition Scheme "Default 4MB with spiffs"`. This ensures enough space for the sketch and LittleFS data.
4.  **Install Libraries:**
    * Go to `Sketch > Include Library > Manage Libraries...` and install the following:
        * `ArduinoJson` by Benoit Blanchon
        * `MD_Parola` by majicDesigns (this will typically also install its dependency: `MD_MAX72xx`)
        * `AsyncTCP` by ESP32Async
        * `ESPAsyncWebServer` by ESP32Async

---

### ⬆️ Uploading the Code and Data

Once your Arduino IDE is set up for your board (as described above):

1.  **Open the Project Folder:**
    * For ESP8266: Navigate to and open the `ESPTimceCast_ESP8266` project folder. Inside, you'll find the main sketch file, typically named `ESPTimceCast_ESP8266.ino`. Open this `.ino` file in the Arduino IDE.
    * For ESP32: Navigate to and open the `ESPTimceCast_ESP32` project folder. Inside, you'll find the main sketch file, typically named `ESPTimceCast_ESP32.ino`. Open this `.ino` file in the Arduino IDE.
2. **Upload the Sketch:**
    * With the main sketch file open, click the "Upload" button (the right arrow icon) in the Arduino IDE toolbar. This will compile the entire project and upload it to your board.
3.  **Upload `/data` folder (LittleFS):**
    * This project uses LittleFS for storing web interface files and other assets. You'll need the LittleFS Uploader plugin.
    * [**Install the LittleFS Uploader Plugin**](https://randomnerdtutorials.com/arduino-ide-2-install-esp8266-littlefs/) 
    * **Before uploading, ensure the Serial Monitor is closed.**
    * Open the Command Palette (`Ctrl+Shift+P` on Windows, `Cmd+Shift+P` on macOS).
    * Search for and run: `Upload Little FS to Pico/ESP8266/ESP32` (the exact command name might vary).
    * **Important for ESP32:** If the upload fails, you might need to manually put your ESP32 into "Download Mode." While holding down the **Boot button** (often labeled 'BOOT' or 'IO0' or 'IO9'), briefly press and release the **RST button**, then release the Boot button.

---


## 📺 Display Behavior

**ESPTimeCast** automatically switches between two display modes: Clock and Weather.
If "Show Weather Description" is enabled a third mode (Description) will display with a duration of 3 seconds, if the description is too long to fit on the display the description will scroll from right to left once.

What you see on the LED matrix depends on whether the device has successfully fetched the current time (via NTP) and weather (via OpenWeatherMap).  
The following table summarizes what will appear on the display in each scenario:

| Display Mode | 🕒 NTP Time | 🌦️ Weather Data | 📺 Display Output                              |
|:------------:|:----------:|:--------------:|:--------------------------------------------|
| **Clock**    | ✅ Yes      | —              | 🗓️ Day Icon + ⏰ Time (e.g. `@ 14:53`)           |
| **Clock**    | ❌ No       | —              |  `! NTP` (NTP sync failed)               |
| **Weather**  | —          | ✅ Yes         | 🌡️ Temperature (e.g. `23ºC`)                |
| **Weather**  | ✅ Yes      | ❌ No          | 🗓️ Day Icon + ⏰ Time (e.g. `@ 14:53`)           |
| **Weather**  | ❌ No       | ❌ No          |  `! TEMP` (no weather or time data)       |

### **How it works:**

- The display automatically alternates between **Clock** and **Weather** modes (the duration for each is configurable).
- If "Show Weather Description" is enabled a third mode **Description** will display after the **Weather** display with a duration of 3 seconds.
- In **Clock** mode, if NTP time is available, you’ll see the current time plus a unique day-of-week icon. If NTP is not available, you'll see `! NTP`.
- In **Weather** mode, if weather is available, you’ll see the temperature (like `23ºC`). If weather is not available but time is, it falls back to showing the clock. If neither is available, you’ll see `! TEMP`.
- All status/error messages (`! NTP`, `! TEMP`) are big icons shown on the display.

**Legend:**
- 🗓️ **Day Icon**: Custom symbol for day of week (`@`, `=`, etc.)
- ⏰ **Time**: Current time (HH:MM)
- 🌡️ **Temperature**: Weather from OpenWeatherMap
- ✅ **Yes**: Data available
- ❌ **No**: Data not available
- — : Value does not affect this mode

---

## ☕ Support this project

If you enjoy this project, please consider supporting my work:

[![Donate via PayPal](https://img.shields.io/badge/Donate-PayPal-blue.svg?logo=paypal)](https://www.paypal.me/officialuphoto)
[![GitHub Sponsors](https://img.shields.io/badge/GitHub-Sponsor-fafbfc?logo=github&logoColor=ea4aaa)](https://github.com/sponsors/mfactory-osaka) 
























