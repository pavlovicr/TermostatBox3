
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief AHT21 sensor data structure
 */
typedef struct {
    float temperature;  // Temperature in Celsius
    float humidity;     // Relative humidity in %
    bool valid;         // Data validity flag
} sensor_data_t;

/**
 * @brief Initialize sensor manager and AHT21 sensor
 * 
 * Initializes I2C bus and AHT21 sensor on ESP32-S3
 * I2C configuration:
 * - SCL: GPIO40
 * - SDA: GPIO41
 * - I2C Address: 0x38
 * - Frequency: 100kHz
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_manager_init(void);

/**
 * @brief Read temperature and humidity from AHT21
 * 
 * Triggers measurement and reads data from AHT21 sensor
 * 
 * @param data Pointer to sensor_data_t structure to store results
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_manager_read(sensor_data_t *data);

/**
 * @brief Deinitialize sensor manager
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_manager_deinit(void);

/**
 * @brief Check if sensor is initialized
 * 
 * @return true if initialized, false otherwise
 */
bool sensor_manager_is_initialized(void);

#endif // SENSOR_MANAGER_H