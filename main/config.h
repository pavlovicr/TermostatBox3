/**
 * @file config.h
 * @brief Aplikacijske konstante in konfiguracija
 */
#ifndef CONFIG_H
#define CONFIG_H

// Temperature settings
#define DEFAULT_TARGET_TEMP         21.0f
#define TEMP_HYSTERESIS             0.5f    // ±0.5°C
#define MIN_TARGET_TEMP             15.0f
#define MAX_TARGET_TEMP             30.0f

// Display settings
#define DISPLAY_BRIGHTNESS_DEFAULT  30
#define DISPLAY_BRIGHTNESS_NIGHT    10

// Sensor settings
#define SENSOR_READ_INTERVAL_MS     2000
#define SENSOR_I2C_SCL_GPIO         40
#define SENSOR_I2C_SDA_GPIO         41

// Furnace control
#define FURNACE_RELAY_GPIO          12      // Primer GPIO za rele

// UI colors
#define COLOR_TEMP_NORMAL           0x00FF00
#define COLOR_TEMP_ERROR            0xFF0000
#define COLOR_HUMIDITY              0x00BFFF
#define COLOR_FURNACE_ON            0xFF0000
#define COLOR_FURNACE_OFF           0x808080
#define COLOR_TARGET_TEMP           0xFFAA00

#endif // CONFIG_H

// ════════════════════════════════════════════
// NETWORK CONFIGURATION
// ════════════════════════════════════════════
#define WIFI_SSID                   "YOUR_WIFI_SSID"
#define WIFI_PASSWORD               "YOUR_WIFI_PASSWORD"

// ════════════════════════════════════════════
// SHELLY 2PM CONFIGURATION
// ════════════════════════════════════════════
#define SHELLY_IP_ADDRESS           "192.168.1.100"
#define SHELLY_FURNACE_CHANNEL      0    // Kateri relay (0 ali 1)
#define SHELLY_STATUS_INTERVAL_MS   10000 // Vsakih 10s preveri status