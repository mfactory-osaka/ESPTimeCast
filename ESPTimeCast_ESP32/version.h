/*
ESPTimeCast™

Copyright (c) 2026 M-Factory

This software is source-available for personal, non-commercial use only.
It is not open source.

See LICENSE.txt for full terms.
*/

#pragma once
#define FIRMWARE_VERSION "2.2.1"

// Auto-detect the specific chip family
#if defined(WIFI_TX_POWER_CAP)
  #define BOARD_TYPE "esp32c3alt"
#elif defined(ESP8266)
  #define BOARD_TYPE "esp8266"
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  #define BOARD_TYPE "esp32s2"
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  #define BOARD_TYPE "esp32s3"
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  #define BOARD_TYPE "esp32c3"
#elif defined(ESP32)
  #define BOARD_TYPE "esp32"
#else
  #define BOARD_TYPE "unknown"
#endif