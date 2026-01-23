![ESPTimeCast](assets/logo.svg)

![GitHub stars](https://img.shields.io/github/stars/mfactory-osaka/ESPTimeCast?style=social)
![GitHub forks](https://img.shields.io/github/forks/mfactory-osaka/ESPTimeCast?style=social)
![Last Commit](https://img.shields.io/github/last-commit/mfactory-osaka/ESPTimeCast)  
[![Hackaday](https://img.shields.io/badge/Featured%20on-Hackaday-black?logo=hackaday&logoColor=white)](https://hackaday.com/2025/10/02/building-a-desk-display-for-time-and-weather-data)
[![XDA Developers](https://img.shields.io/badge/Featured%20on-XDA%20Developers-blueviolet?logo=android&logoColor=white)](https://www.xda-developers.com/super-sleek-esp32-weather-station)
[![Hackster](https://img.shields.io/badge/Featured%20on-Hackster-orange?logo=hackster&logoColor=white)](https://www.hackster.io/news/the-perfect-minimalist-led-clock-49a4e4440518)

🎉 **1,000+ GitHub stars - thank you to the community!**  

**ESPTimeCast** is a sleek, WiFi-connected LED matrix clock and weather display built on **ESP8266/ESP32** and **MAX7219**.
It combines real-time NTP time sync, live OpenWeatherMap updates, and a modern web-based configuration interface — all in one compact design.


<video src="https://github.com/user-attachments/assets/78b6525d-8dcd-43fc-875e-28805e0f4fab"></video>

&nbsp;
## 📦 3D Printable Case

To help support the project’s development, the official **ESPTimeCast** case design is available as a **paid STL download** (see links below).  

If you prefer a free option, there are many compatible **MAX7219 LED matrix enclosures** shared by the community - you can find plenty by searching for “MAX7219 case” on Printables, Cults3D, or similar sites.

<img src="assets/image01.png" alt="3D Printable Case V1" width="640" />
<img src="assets/image02.png" alt="3D Printable Case V2" width="640" />

<p align="left">
  <a href="https://www.printables.com/model/1344276-esptimecast-wi-fi-clock-weather-display">
    <img src="https://img.shields.io/badge/Printables-384%20Downloads-orange?logo=prusa" width="210">
  </a>
  <br>
  <a href="https://cults3d.com/en/3d-model/gadget/wifi-connected-led-matrix-clock-and-weather-station-esp8266-and-max7219">
    <img src="https://img.shields.io/badge/Cults3D-117%20Downloads-blue?logo=cults3d" width="180">
  </a>
</p>

&nbsp;
## 📰 Press Mentions

ESPTimeCast has been featured on major maker and tech platforms highlighting its design, usability, and open-source community. 
- [Hackaday](https://hackaday.com/2025/10/02/building-a-desk-display-for-time-and-weather-data)  
- [XDA Developers](https://www.xda-developers.com/super-sleek-esp32-weather-station)
- [Hackster.io](https://www.hackster.io/news/the-perfect-minimalist-led-clock-49a4e4440518)

&nbsp;
## ✨ Features

- **LED Matrix Display (8x32)** powered by MAX7219, with custom font support
- **Simple Web Interface** for all configuration (WiFi, weather, time zone, display durations, and more)
- **Automatic NTP Sync** with robust status feedback and retries
- **Weather Fetching** from OpenWeatherMap (every 5 minutes, temp/humidity/description)
- **Custom Scroll Messages** - fully persistent until manually cleared via the Web UI
- **Fallback AP Mode** for easy first-time setup or configuration
- **Timezone Selection** from IANA names (DST integrated on backend)
- **Get My Location** button to get your approximate Lat/Long
- **Week Day and Weather Description display** in multiple languages
- **Persistent Config** stored in LittleFS, with backup/restore system
- **Status Animations** for WiFi connection, AP mode, time syncing
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
  - **Automatic Dimming** based on Sunrise/Sunset from weather API
  - **Custom Dimming** select custom dimming hours
  - **Countdown** function (Scroll / Dramatic)
  - **Optional:** ESPTimeCast supports displaying glucose data from **Nightscout** servers every 5 minutes, alternating with weather information
  - **Optional:** Export and Upload settings via `device-ip/export` and `device-ip/upload` endpoints

&nbsp;
## 🪛 Wiring - OCT 17 - Important Hardware Update ⚠️ ⚠️ ⚠️

**Pin assignments have been standardized across all Wemos D1 Mini and S2 Mini boards!**  
Check your board version and make sure **your sketch matches the new pin layout**, especially note that **`CLK` is now on `D5` (was `D6`)**.

After testing, we found that powering the MAX7219 matrix from the **D1 Mini’s 3.3V pin** can cause **overheating and unstable display behavior**, so we’re now officially powering the display from the **5V USB rail** instead.  
This change improves **brightness**, **stability**, and protects the **onboard voltage regulator** from long-term damage.

> **Note:** Pin positions on the header remain the same, but ensure your sketch uses the correct pin definitions.

#### What’s Changing

- **Before:** Display VCC was connected to the **3.3V** pin on the ESP board  
- **Now:** Display VCC connects to the **5V** pin (direct from USB power)  
- **CLK:** moved from **D6 → D5**

#### Why This Change Is Needed

- Standardized pin assignment across all Wemos D1 Mini and S2 Mini boards  
- The **MAX7219** LED matrix is designed for **5V operation**  
- The **AMS1117 3.3V regulator** on most D1 Minis has limited current (~800 mA max, often less)  
- High-brightness displays (especially green/yellow) can overload it, causing:
  - Overheating  
  - Voltage drop  
  - Regulator failure (some users saw only **2.4 V** output after damage)

#### Benefits of Using 5V

- Brighter and more stable display output  
- Reduced heat load on the ESP board  
- Prevents long-term regulator damage  
- No level shifters required — **MAX7219 works fine with 3.3V logic signals**


#### Next Steps

- Double-check your board wiring before powering on  
- Update your sketch pin definitions if you’re upgrading from an older build  
- Reference the diagram below for correct wiring (coming next)

#### 🧩 Board → MAX7219 

| D1 Mini (Micro USB) | D1 Mini (USB C) | D1 Mini (ESP 32) | S2 Mini | C3 Supermini |  MAX7219 |
|:-----:|:-----:|:-----:|:-----:|:------:|:------|
|  GND  |  GND  |  GND  |  GND  |  GND  | GND |
|  5V   |  5V/VBUS   |  5V/VBUS   |  5V/VBUS  | 5V  |  VCC  | 
|  D5   |  14   |  18   |  7    | 7    |  CLK  | 
|  D7   |  13   |  23   |  11   | 20   | CS   |
|  D8   |  15   |  5    |  12   | 8    | DIN  |


<img src="assets/wiring3.png" alt="Wiring" width="800" />

> **Tip:** Always double-check that VCC (5V), GND, and DIN/CS/CLK match your MAX7219 module’s pin order — different modules sometimes label them differently.


&nbsp;
## First-time Setup / AP Mode

1. Power on the device. If WiFi fails, it auto-starts in AP mode:
   - **SSID:** `ESPTimeCast`
   - **Password:** `12345678`
   - Captive portal should open automatically, if it doesn't open `http://192.168.4.1` or `http://setup.esp` in your browser.
2. Set your WiFi and all other options.
3. Click **Save Setting** – the device saves config, reboots, and connects.
4. The device shows its local IP address after boot so you can login again for setting changes

> External links and the "Get My Location" button require internet access.  
They won't work while the device is in AP Mode - connect to WiFi first.

&nbsp;
## 🌐 Web UI & Configuration

ESPTimeCast includes a built-in Web UI that lets you fully configure the device from any browser — no apps required.

#### You can open the Web UI using either:

- http://esptimecast.local  
mDNS / Bonjour - Works on macOS, iOS, Windows with Bonjour, and most modern browsers.

- The device’s **local IP address**  
→ On every reboot, ESPTimeCast shows its IP on the LED display so you can easily connect.

#### The Web UI gives you control over:
- **WiFi settings** (SSID & Password)
- **Weather settings** (OpenWeatherMap API key, City, Country, Coordinates)
- **Time zone** (will auto-populate if TZ is found)
- **Day of the Week and Weather Description** languages
- **Display durations** for clock and weather (milliseconds)
- **Custom Scroll Text** - set a persistent scrolling message on the display directly from the Web UI
- **Advanced Settings** (see below)

&nbsp;
## UI Example:
<img src="assets/webui8.png" alt="Web Interface" width="640">

&nbsp;
## ⚙️ Advanced Settings

Click the **cog icon** next to “Advanced Settings” in the Web UI to reveal extra configuration options.  

**Available advanced settings:**

- **Primary NTP Server**: Override the default NTP server (e.g. `pool.ntp.org`)
- **Secondary NTP Server**: Fallback NTP server (e.g. `time.nist.gov`)
- **Day of the Week**: Display Day of the Week in the desired language
- **Blinking Colon** toggle (default is on)
- **Show Date** (default is off, duration is the same as weather duration)
- **24/12h Clock**: Switch between 24-hour and 12-hour time formats (24-hour default)
- **Imperial Units (°F)** toggle (metric °C defaults)
- **Humidity**: Display Humidity besides Temperature
- **Weather description** toggle (display weather description in the selected language for 3 seconds or scrolls once if description is too long)
- **Flip Display**: Invert the display vertically/horizontally
- **Brightness**: Off - 0 (dim) to 15 (bright)
- **Automatic Dimming Feature** base on Sunrise/Sunset from weather API
- **Custom Dimming Feature**: Start time, end time and desired brightness selection
- **Countdown** function, set a countdown to your favorite/next event, 2 modes: Scroll/Dramatic! 

>Non-English characters converted to their closest English alphabet.   
>For Esperanto, Irish, and Swahili, weather description translations are not available. Japanese translations exist, but since the device cannot display all Japanese characters, English will be used in all these cases.  

> **Tip:** Don't forget to press the save button to keep your settings

&nbsp;
## 📝 Configuration Notes

- **OpenWeatherMap API Key:**
   - [Make an account here](https://home.openweathermap.org/users/sign_up)
   - [Check your API key here](https://home.openweathermap.org/api_keys)
- **City Name:** e.g. `Tokyo`, `London`, etc.
- **Country Code:** 2-letter code (e.g., `JP`, `GB`)
- **ZIP Code:** Enter your ZIP code in the city field and US in the country field (US only)
- **Latitude and Longitude** You can enter coordinates in the city field (lat.) and country field (long.)
- **Time Zone:** Select from IANA zones (e.g., `America/New_York`, handles DST automatically)


&nbsp;
## 🚀 Getting Started

This guide will walk you through setting up your environment and uploading the **ESPTimeCast** project to your **ESP8266** or **ESP32** board. Please follow the instructions carefully for your specific board type.

#### ⚙️ ESP8266 Setup

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
        * `ESPAsyncWebServer` by ESP32Async (3.9.1 or above)  
&nbsp;
#### ⚙️ ESP32 Setup

Follow these steps to prepare your Arduino IDE for ESP32 development:

1.  **Install ESP32 Board Package:**
    * Go to `Tools > Board > Boards Manager...`. Search for `esp32` by `Espressif Systems` and click "Install".
2.  **Select Your Board:**
    * Go to `Tools > Board` and select your specific board, e.g., **LOLIN S2 Mini** (or your ESP32 variant).
3.  **Configure Partition Scheme:**
    * Under `Tools`, select `Partition Scheme "No OTA (2MB APP/2MB SPIFFS) or No OTA (LARGE APP)"`. This ensures enough space for the sketch and LittleFS data.
4.  **Install Libraries:**
    * Go to `Sketch > Include Library > Manage Libraries...` and install the following:
        * `ArduinoJson` by Benoit Blanchon
        * `MD_Parola` by majicDesigns (this will typically also install its dependency: `MD_MAX72xx`)
        * `AsyncTCP` by ESP32Async
        * `ESPAsyncWebServer` by ESP32Async
    
&nbsp;
#### ⬆️ Uploading the Code and Data

Once your IDE is ready:

1. **Open the Project Folder**
   * **ESP8266:** Open the `ESPTimeCast_ESP8266` folder and open `ESPTimeCast_ESP8266.ino`.
   * **ESP32:** Open the `ESPTimeCast_ESP32` folder and open `ESPTimeCast_ESP32.ino`.

2. **Upload the Sketch**
   * Click the **Upload** button (right arrow icon) in the Arduino IDE toolbar. This will compile and upload the sketch to your board.
   * **No separate LittleFS upload is needed.** All web UI files are embedded in the sketch.
  
**⚠️ Note for existing users:** If you have previously uploaded /data via LittleFS, you can safely skip that step now — the device will manage config files internally.

&nbsp;
## 🏠 ESPTimeCast Home Assistant Integration

This guide explains how to integrate **ESPTimeCast** with **Home Assistant** to send custom messages to your LED display.

#### 🧠 Overview

ESPTimeCast exposes a REST API endpoint that lets you send **scrolling messages** to the display from either **Home Assistant** or the built-in **Web UI**.

#### Web UI messages
- Act as **persistent** messages
- Remain active (even through reboots) until replaced or cleared in the Web UI
- Short messages (up to 8 characters) display **static & centered**, using the Web UI’s `Weather Duration` before the display rotates to the next mode

#### Home Assistant messages
- Are **temporary overrides**
- Do **not** overwrite the persistent Web UI message
- Can automatically expire using:
  - `scrolltimes` → number of scroll cycles
- If neither parameter is sent:
  - Short messages (up to 8 characters) use `Weather Duration`
  - Long messages scroll **once per display cycle** (then the display advances to the next mode, e.g., clock → weather → …)

>**New:** Home Assistant messages can now expire automatically after a set number of **seconds** or **scroll cycles** and the last Web UI message (if any) will be restored.
#### 🔗 Endpoint

```
POST http://<device_ip>/set_custom_message
```


#### 📝 Parameters

| Parameter | Type | Required | Description |
|------------|------|-----------|-------------|
| `message` | string | Yes | Message text to display. Send an empty string (`""`) to clear messages. |
| `speed` | integer | Optional | Scrolling speed (range **10–200**). Lower values = **faster** scroll. |
| `seconds` | integer | Optional | Maximum display duration in seconds (range **0–3600**). If set to **0**, `Weather Duration` will be used. |
| `scrolltimes` | integer | Optional | Maximum number of full scroll cycles (**range 0–100**). Set to **0** for infinite scrolls. |


#### 💡 Message Behavior Overview

| Source | Behavior | Notes |
|---------|-----------|-------|
| **Home Assistant** | Displays message temporarily (until next mode rotation or clear). | Restores any saved Web UI message afterward. |
| **Web UI** | Displays message persistently until manually cleared. | Acts as a permanent banner or ticker. |
| **Clear command from Web UI** | Clears *all* messages (HA + UI). | Use this to reset the display completely. |
| **Clear command from Home Assistant** | Clears only the temporary HA message. | UI message will reappear if one was saved. |
| **Scrolltimes expires (HA only)** | **Automatic clear.** The temporary message is removed when the limit is reached.| Automatically restores the saved UI message. |

**Short messages (up to 8 characters):**  
- Display static & centered (no scrolling).  
- **Home Assistant:** uses `seconds` if provided, otherwise the Web UI **Weather Duration**.  
- **Web UI:** always uses **Weather Duration**.

**Long messages (8 characters or more):**  
- Always scroll.
- If sent from HA, scrolling stops when **scrolltimes** limit is reached or manually clered when sent without parameter.

#### ⚙️ Example Automations

#### 1. Send a Temporary HA Message with Duration

```yaml
alias: Notify Door Open on ESPTimeCast
trigger:
  - platform: state
    entity_id: binary_sensor.front_door
    to: "on"
action:
  - service: rest_command.esptimecast_message
    data:
      message: "DOOR OPEN"
      speed: 60
      seconds: 15 # Message will automatically clear after 15 seconds
```

#### 2. Send a Temporary HA Message with Scroll Count

```yaml
alias: Notify Mail Delivered Three Times
action:
  - service: rest_command.esptimecast_message
    data:
      message: "MAIL DELIVERED"
      scrolltimes: 3 # Message will clear after 3 complete scroll cycles
```

#### 3. Manually Clear the Temporary Message

```yaml
alias: Clear ESPTimeCast Message
trigger:
  - platform: state
    entity_id: binary_sensor.front_door
    to: "off"
action:
  - service: rest_command.esptimecast_message
    data:
      message: "" # Sends an empty message to trigger the clear logic
```


#### 🧩 Example `rest_command` Configuration

Add this to your `configuration.yaml` This configuration uses default values for the new parameters (`seconds` and `scrolltimes`) set to `0` (infinite) if they are not passed in the service call.


```yaml
rest_command:
  esptimecast_message:
    url: "http://<device_ip>/set_custom_message"
    method: POST
    content_type: "application/x-www-form-urlencoded"
    payload: "message={{ message }}&speed={{ speed | default(85) }}&seconds={{ seconds | default(0) }}&scrolltimes={{ scrolltimes | default(0) }}"
```

Then restart Home Assistant.

#### ⚡ Quick Test via curl
You can quickly test sending a message to your ESPTimeCast display using `curl` from any computer on the same network:

```
curl -X POST -d "message=HA TEST&speed=40&seconds=10&scrolltimes=2" "http://<device_ip>/set_custom_message"
```
> Replace <device_ip> with the IP of your ESPTimeCast device.  
> The message parameter is your text to display.  
> The optional speed parameter controls the scroll speed (10–200, lower = faster).
> The message will clear after **10 seconds** OR **2 scrolls**, whichever comes first.

#### 🧾 Notes

- Only **A–Z, 0–9**, spaces, and simple punctuation (`: ! ' - . , _ + % / ?`) are allowed.  
- All text is automatically converted to **uppercase**.
- Lower scroll speed values make the message **scroll faster**.
- Custom Message scroll speed can be changed via this endpoint.
- If both seconds and scrolltimes are set to non-zero values, the message is removed when the **first condition is met**.

#### ✅ Example Use Cases

- Temporary alerts like **DOOR OPEN**, **RAIN STARTING**, or **MAIL DELIVERED**.  
- Persistent ticker messages from the Web UI like **WELCOME HOME** or **ESPTIMECAST LIVE**.  
- Combine both: Web UI for a base banner, and HA for transient automation messages.

&nbsp;
#### 🔆 Brightness Control (Home Assistant)

ESPTimeCast provides an endpoint that allows Home Assistant to remotely control the LED matrix brightness — including turning the display completely off.

#### 🔗 Endpoint
```
POST http://<device_ip>/set_brightness
```

#### 📝 Parameters

| Parameter | Type | Required | Description |
|-----------|-------|----------|-------------|
| `value` | integer | Yes | Brightness level **0–15**, or **-1** to turn the display **off**. |

- Values **0–15** set the LED matrix brightness normally.  
- Value **-1** turns the display off entirely (LEDs disabled) until brightness is set again.
- When brightness is set back to 0–15, the display immediately resumes showing the current message or mode.


#### 🧩 Example Home Assistant `rest_command`

```
rest_command:
  esptimecast_brightness:
    url: "http://<device_ip>/set_brightness"
    method: POST
    content_type: "application/x-www-form-urlencoded"
    payload: "value={{ brightness }}"
```

#### ⚡ Example Automation

```
alias: Dim ESPTimeCast at Night
trigger:
  - platform: time
    at: "23:00"
action:
  - service: rest_command.esptimecast_brightness
    data:
      brightness: -1   # Turns the display off
```

#### ⚡ Quick Test via curl

You can quickly test changing the brightness of your ESPTimeCast display using `curl` from any computer on the same network:

```
curl -X POST -d "value=10" "http://<device_ip>/set_brightness"
```

> Replace <device_ip> with the IP address of your ESPTimeCast device.  
> Use a brightness value between **0–15**, or **-1** to turn the display off.

&nbsp;
## 🧩 Hidden & Advanced Features

ESPTimeCast includes a few optional “power-user” features that aren’t visible in the main interface but can be accessed directly from your browser. These are intended for advanced users who want more control or integration.

#### ⚙️ /factory_reset
Erases all saved configuration data, Wi-Fi credentials, and uptime history.
Used to restore the device to its original state. Only available in **AP mode**.

**Example:**  
```
http://192.168.4.1/factory_reset
```

#### 💾 /export
Downloads your current configuration (`config.json`) directly from the device.  
This is useful for creating backups or migrating settings between devices.

**Example:**  
```
http://your-device-ip/export
```
The file will download automatically with your saved WiFi credentials (safely masked for security) and all other settings.

#### 📂 /upload
Lets you manually upload a configuration file (`config.json`) to the device.  
Perfect for restoring a backup or quickly switching between setups.

**Usage:**
1. Go to  
   ```
   http://your-device-ip/upload
   ```
2. Select your edited or backup `config.json` file.  
3. The device will confirm the upload and automatically reboot with the new configuration.

> *Tip:* You can export → edit the file on your computer → re-upload to test new settings without using the web interface.


#### ⚕️ Nightscout Integration
ESPTimeCast supports displaying glucose data from **Nightscout** servers alongside weather information.

When the secondary NTP/URL field (`ntpServer2`) contains a valid Nightscout API endpoint for example:  
```
https://your-cgm-server/api/v1/entries/current.json?token=xxxxxxxxxxxxx
```
the device automatically enables **Glucose Display Mode**.

In this mode:
- The device fetches glucose data every 5 minutes.
- Glucose value and trend direction are displayed alternately with time and weather.
- The display duration for Nightscout data is the same as the weather display duration.
- Weather data continues to display normally.
- Debug logs confirm updates and Nightscout responses in the Serial Monitor.

#### ⚠️ Notes
- These features are optional and hidden from the main interface to avoid clutter.  
- `/upload` and `/export` are intentionally unlinked from the UI to prevent accidental access.  
- Always verify your WiFi credentials and tokens before uploading edited configurations.

&nbsp;
## 📺 Display Behavior

**ESPTimeCast** automatically switches between two display modes: Clock and Weather.
If "Show Weather Description" is enabled, a third mode (Description) will display with a duration of 3 seconds, if the description is too long to fit on the display the description will scroll from right to left once.

What you see on the LED matrix depends on whether the device has successfully fetched the current time (via NTP) and weather (via OpenWeatherMap).  
The following table summarizes what will appear on the display in each scenario:

| Display Mode | 🕒 NTP Time | 🌦️ Weather Data | 📺 Display Output                              |
|:------------:|:----------:|:--------------:|:--------------------------------------------|
| **Clock**    | ✅ Yes      | —              | 🗓️ Day Icon + ⏰ Time (e.g. `@ 14:53`)           |
| **Clock**    | ❌ No       | —              |  `! NTP` (NTP sync failed)               |
| **Weather**  | —          | ✅ Yes         | 🌡️ Temperature (e.g. `23ºC`)                |
| **Weather**  | ✅ Yes      | ❌ No          | 🗓️ Day Icon + ⏰ Time (e.g. `@ 14:53`)           |
| **Weather**  | ❌ No       | ❌ No          |  `! TEMP` (no weather or time data)       |

#### How it works:

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


&nbsp;
## 📣 Community & Help
If you need assistance, want to share your build, or discuss new features:  
👉 Join the ESPTimeCast Community on Reddit: [r/ESPTimeCast](https://www.reddit.com/r/ESPTimeCast/)
&nbsp;  
&nbsp;
## 🤝 Contributing
ESPTimeCast is a personal project, and to keep the codebase simple and focused, I’m not accepting pull requests.  
However, **forks, custom additions, and experiments are always welcome**. Feel free to build on it in your own fork ;)
&nbsp;  
&nbsp;
## ❤️ Support this project
ESPTimeCast is an open-source passion project that blends art, engineering, and design.  
If you enjoy it, you can help keep the project growing - even something as simple as leaving a ⭐ on GitHub goes a long way.  

If you'd like to go further, you can also support development through the options below:

[![Donate via PayPal](https://img.shields.io/badge/Donate-PayPal-blue.svg?logo=paypal)](https://www.paypal.me/officialuphoto)
[![GitHub Sponsors](https://img.shields.io/badge/GitHub-Sponsor-fafbfc?logo=github&logoColor=ea4aaa)](https://github.com/sponsors/mfactory-osaka)   
&nbsp;
&nbsp;


      
































































