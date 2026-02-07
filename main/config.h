/**
 * @file config.h
 * @brief Aplikacijske konstante - Thermostat projekt
 */
#ifndef CONFIG_H
#define CONFIG_H

// ════════════════════════════════════════════
// NETWORK CONFIGURATION
// ════════════════════════════════════════════
#define WIFI_SSID                   "TP-Link"      // ← SPREMENI TO!
#define WIFI_PASSWORD               "rolu9255"  // ← SPREMENI TO!
#define WIFI_CONNECT_TIMEOUT_MS     30000  // 30 sekund

// ════════════════════════════════════════════
// SHELLY 2PM CONFIGURATION
// ════════════════════════════════════════════
#define SHELLY_IP_ADDRESS           "192.168.0.111"  // ← SPREMENI TO!
#define SHELLY_FURNACE_CHANNEL      0    // Kateri relay (0 ali 1)
#define SHELLY_STATUS_INTERVAL_MS   10000 // Vsakih 10s preveri status

// ════════════════════════════════════════════
// TEMPERATURE SETTINGS
// ════════════════════════════════════════════
#define DEFAULT_TARGET_TEMP         20.0f
#define MIN_TARGET_TEMP             15.0f
#define MAX_TARGET_TEMP             30.0f

// ════════════════════════════════════════════
// HARDWARE CONFIGURATION
// ════════════════════════════════════════════
// Sensor I2C pins (AHT21 na ESP32-S3-BOX-3)
#define SENSOR_I2C_SCL_GPIO         40
#define SENSOR_I2C_SDA_GPIO         41
#define SENSOR_I2C_FREQ_HZ          100000

// ════════════════════════════════════════════
// DISPLAY SETTINGS
// ════════════════════════════════════════════
#define DISPLAY_BRIGHTNESS_DEFAULT  30
#define DISPLAY_BRIGHTNESS_NIGHT    10
#define DISPLAY_BRIGHTNESS_MAX      100

// ════════════════════════════════════════════
// TIMING CONFIGURATION
// ════════════════════════════════════════════
#define SENSOR_READ_INTERVAL_MS     2000   // Vsake 2 sekundi
#define UI_UPDATE_INTERVAL_MS       100

// ════════════════════════════════════════════
// UI COLORS (RGB HEX)
// ════════════════════════════════════════════
#define COLOR_TEMP_NORMAL           0x00FF00
#define COLOR_TEMP_ERROR            0xFF0000
#define COLOR_HUMIDITY              0x00BFFF
#define COLOR_FURNACE_HEATING       0xFF0000
#define COLOR_FURNACE_OFF           0x808080
#define COLOR_FURNACE_ERROR         0xFF6600
#define COLOR_TARGET_TEMP           0xFFAA00
#define COLOR_BACKGROUND            0x003a57
#define COLOR_WIFI_CONNECTED        0x00FF00
#define COLOR_WIFI_DISCONNECTED     0xFF0000

#endif // CONFIG_H