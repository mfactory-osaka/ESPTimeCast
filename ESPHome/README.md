# ESPTimeCast for ESPHome

Based on @mfactory-osaka Arduino code I have created a version that will work with an ESP32 compiled through [ESPHome](https://esphome.io/)

Eventually there will be 2 versions:

1. A version that is more or less a 100% copy of the Arduino version
2. A version that will work with Home Assistant, and pull all data from there.

Currently only no 1. is available, but I am working on the version for Home Assistant.

## Hardware

I really like the case design done by @mfactory-osaka so I have made sure that boards I have tested fit in to the 3D printed casing, but if you design your own, you can use whatever ESP32 you like, as long as it can support the MAX7219 display.

For my testing, I have tried both with a **Wemos ESP32 D1 Mini** and a **ESP32 DEVKITV1**. The default PIN Layout in the YAML file is set for the Wemos ESP32 D1 Mini, but you can change these to the correct PIN's for the specific board you end up using,

For the Display, I have used the same *MAX7219* display as for the Arduino version, as this is also supported in ESPHome.

## Setup Device

You must have [ESPHome](https://esphome.io/) installed already and I will not here describe how to create a new Device as this is already covered in the ESPHome documentation.

* Once you have a new device, edit it, and paste the code from `esp-time-cast.yaml` in to the file, overwriting everything that is in there.
* Make sure you have the [MatrixLight8X Font File](https://github.com/trip5/Matrix-Fonts/blob/main/8-series/MatrixLight8X.ttf) downloaded and copied to a directory in your ESPHome directory. The default in the YAML file is a directory called `fonts` under the `esphome` directory.
* In the top of the YAML file you will find the below section in the top of the file:

  ```yaml
  ### CHANGE SETTINGS BELOW BEFORE COMPILE ###
  PIN_CLK: "18"                           # Enter GPIO Pin for CLK
  PIN_CS: "5"                             # Enter GPIO Pin for CS (DATA)
  PIN_MOSI: "23"                          # Enter GPIO Pin for MOSI

  flip_display: "false"                   # Set to true to flip the display 180°
  default_brightness: "7"                 # Enter the default brightness level of the display 0 -15

  ssid: !secret wifi_ssid                 # The name of the WiFi network. When using !secret that means that wifi_ssid needs to be defined in secrets.yaml
  password: !secret wifi_password         # The password for the WiFi Network

  font_path: "fonts/MatrixLight8X.ttf"    # Enter the path and filename of the font you use. I recommend the file entered.
                                          # Change the path if you have not placed it in a fonts directory under esphome
                                          # Find the font here: https://github.com/trip5/Matrix-Fonts/tree/main/8-series
                                          # The default font can be found in the Github Repository for this project.

  timezone: "Europe/Copenhagen"           # Enter your Timezone

  compass_N: "N"                          # Compass Direction North
  compass_NE: "NØ"                        # Compass Direction North East
  compass_E: "Ø"                          # Compass Direction East
  compass_SE: "SØ"                        # Compass Direction South East
  compass_S: "S"                          # Compass Direction South
  compass_SW: "SV"                        # Compass Direction South West
  compass_W: "V"                          # Compass Direction West
  compass_NW: "NV"                        # Compass Direction North West

  # weekDaysShort: "Søn,Man,Tir,Ons,Tor,Fre,Lør"  # Danish
  weekDaysShort: "Sun,Mon,Tue,Wed,Thu,Fri,Sat"    # English
  # weekDaysShort: "Son,Mon,Din,Mid,Don,Fre,Sam"  # German
  # weekDaysShort: "Dim,Lun,Mar,Mer,Jeu,Ven,Sam"  # French
  # weekDaysShort: "Dom,Lun,Mar,Mer,Gio,Ven,Sab"  # Italien
  ############ END OF SETTINGS ############## 
  ````

  The settings here need to be configured before compiling and uploading to the ESP32 as they cannot be set while running.
  * The top 3 are the PIN's used to communicate with the Display. These must reflect how you connected the cables between the display and the ESP32. The default PIN's work well with a Wemos ESP32 D1 Mini.
  * `flip_display` Set this to true if you need to rotate the screen 180 degree's
  * `default_brightness` is a number between 0 and 15. The higher the number, the brighter the display. This can be adjusted dynamically. See below.
  * `ssid` and `password` are the WiFi Network and passord for that network. You can either type it directly here or you can enter it in to the `secrets.yaml` file, so that it is not visible directly in the code.
  * The `font_path` variable is where ESPHome should look for the font file. (See above)
  * The `timezone` explains itself.
  * The `compass_X` string are used for displaying the Wind Cardinal on the Wind Screen. Localize the text here to your own language.
  * `weekDaysShort` Is used for displaying the weekday on the screen. You can localize the day names, and I added a few examples for other languages. Pick the one you want by uncommenting it, and remember to outcomment the ones not used. Or create your own language string.

* Save the file you just edited, and install the code to the device.

## Configuration

The above settings are all settings that need to be set when compiling and uploading the code and therefore they need to be set in the yaml file. But all the other settings can be set and dynamically changed from the Web Interface.
The Web Interface can be reached on http://esp32-time-cast.local or the IP Address of the device. The port used is port 80.

The interface in ESPHome cannot be made as nice as the one @mfactory-osaka made for the Arduino version, but it will do the trick of settings the parameters.

<img src="esp32_timecast_web.jpg?v2" alt="Web Interface" width="480">

| Setting | Required | Comment |
|:-------------:|:-------:|:-------|
| **Clock Settings** |  |  |
| *Show Weekday* | No | Flip this on if the clock should show the weekday |
| *Show Humidity* | No | Flip this on if the Humidity should be shown with the temperature |
| *Show Condition Screen* | No | If on, this screen will show the current condition in text |
| *Show Wind Screen* | No | If on, this screen will show wind speed and wind cardinal |
| *Clock Duration* | No | The number of seconds the Clock is displayed |
| *Weather Duration* | No | The number of seconds the Weather info is shown |
| *12-Hour Clock* | No | Set to On to show a 12-Hour clock instead of 24-Hour |
| *Brightness* | No | Set the brightness of the display. A number between 0 and 15, where 0 is low and 15 is high |
| **Weather Settings** |  |  |
| *OpenWeather API Key* | Yes | Type your personal [OpenWeather API Key](https://openweathermap.org/api). |
| *City* | Yes | The city to retrieve weather data for  |
| *Country Code* | Yes | The 2-letter country code where the city is located |
| *Language* | Yes | Select a language from the list and the Condition screen will be in that language. Default is english |
| *Weather Units* | Yes | Weather Units to display data for. Metric, Imperial or Standard (Kelvin Temperature)   |
| **Night Mode** |  |  |
| *Toggle Night Mode* | No | If switched on the Clock will go in to Night Mode. If off, the rest of this section will be ignored |
| *Turn Off Display at Night* | No | Night mode can be either dimming the light or turn off the display completely. If this switch is enabled the *Night Mode Brightness* will be ignored and the screen will turn off  |
| *Night Mode Brightness* | No | The intensity of the display when Night Mode is active |
| *Night Mode Start Hour and Minute* | No | The Time of day Night Mode will turn on |
| *Night Mode End Hour and Minute* | No | The Time of day Night Mode will turn off again |
| **Device** |  |  |
| *Toggle Display* | No | Turn the display on or off |
| *Restart Device* | No | Reboot the device |

