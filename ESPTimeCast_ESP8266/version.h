#pragma once
#define FIRMWARE_VERSION "1.3.0"

// Auto-detect the specific chip family
#if defined(ESP8266)
  #define BOARD_TYPE "esp8266"
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  #define BOARD_TYPE "esp32s2"
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  #define BOARD_TYPE "esp32s3"
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  #define BOARD_TYPE "esp32c3"
#elif defined(ESP32)
  #define BOARD_TYPE "esp32" // Standard ESP32 (Original)
#else
  #define BOARD_TYPE "unknown"
#endif