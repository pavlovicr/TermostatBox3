
#include "sensor_manager.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "sensor_manager";

// I2C Configuration for ESP32-S3
#define I2C_MASTER_SCL_IO           40      // GPIO40 for SCL
#define I2C_MASTER_SDA_IO           41      // GPIO41 for SDA
#define I2C_MASTER_FREQ_HZ          100000  // 100kHz
#define I2C_MASTER_TIMEOUT_MS       1000

// AHT21 Configuration
#define AHT21_I2C_ADDR              0x38
#define AHT21_CMD_INIT              0xBE
#define AHT21_CMD_TRIGGER           0xAC
#define AHT21_CMD_SOFT_RESET        0xBA
#define AHT21_MEASUREMENT_DELAY_MS  80      // Time to wait for measurement

// I2C Master Handle
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t aht21_dev_handle = NULL;
static bool initialized = false;

/**
 * @brief Initialize I2C master bus
 */
static esp_err_t init_i2c_bus(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C master bus created on port 0 (SDA=GPIO%d, SCL=GPIO%d)", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

/**
 * @brief Add AHT21 device to I2C bus
 */
static esp_err_t add_aht21_device(void)
{
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AHT21_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &aht21_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add AHT21 device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "AHT21 device added at address 0x%02X", AHT21_I2C_ADDR);
    return ESP_OK;
}

/**
 * @brief Send soft reset command to AHT21
 */
static esp_err_t aht21_soft_reset(void)
{
    uint8_t cmd = AHT21_CMD_SOFT_RESET;
    esp_err_t ret = i2c_master_transmit(aht21_dev_handle, &cmd, 1, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send reset command: %s", esp_err_to_name(ret));
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(20));  // Wait for reset to complete
    ESP_LOGI(TAG, "AHT21 soft reset completed");
    return ESP_OK;
}

/**
 * @brief Initialize AHT21 sensor
 */
static esp_err_t aht21_init(void)
{
    // Send initialization command: 0xBE, 0x08, 0x00
    uint8_t init_cmd[3] = {AHT21_CMD_INIT, 0x08, 0x00};
    esp_err_t ret = i2c_master_transmit(aht21_dev_handle, init_cmd, 3, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize AHT21: %s", esp_err_to_name(ret));
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));  // Wait for initialization
    ESP_LOGI(TAG, "AHT21 initialized successfully");
    return ESP_OK;
}

/**
 * @brief Trigger measurement and read data from AHT21
 */
static esp_err_t aht21_read_raw(uint8_t *data, size_t len)
{
    // Trigger measurement: 0xAC, 0x33, 0x00
    uint8_t trigger_cmd[3] = {AHT21_CMD_TRIGGER, 0x33, 0x00};
    esp_err_t ret = i2c_master_transmit(aht21_dev_handle, trigger_cmd, 3, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to trigger measurement: %s", esp_err_to_name(ret));
        return ret;
    }

    // Wait for measurement to complete
    vTaskDelay(pdMS_TO_TICKS(AHT21_MEASUREMENT_DELAY_MS));

    // Read 7 bytes of data
    ret = i2c_master_receive(aht21_dev_handle, data, len, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read measurement data: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief Convert raw data to temperature and humidity
 */
static void aht21_convert_data(uint8_t *raw_data, sensor_data_t *data)
{
    // Check if sensor is busy (bit 7 of status byte)
    if (raw_data[0] & 0x80) {
        ESP_LOGW(TAG, "Sensor busy, data may not be valid");
        data->valid = false;
        return;
    }

    // Extract humidity (20 bits: bits 12-31 of the data)
    uint32_t humidity_raw = ((uint32_t)raw_data[1] << 12) | 
                            ((uint32_t)raw_data[2] << 4) | 
                            ((uint32_t)raw_data[3] >> 4);
    
    // Extract temperature (20 bits: bits 32-51 of the data)
    uint32_t temperature_raw = (((uint32_t)raw_data[3] & 0x0F) << 16) | 
                               ((uint32_t)raw_data[4] << 8) | 
                               (uint32_t)raw_data[5];

    // Convert to actual values
    data->humidity = ((float)humidity_raw / 1048576.0f) * 100.0f;
    data->temperature = ((float)temperature_raw / 1048576.0f) * 200.0f - 50.0f;
    data->valid = true;

    ESP_LOGD(TAG, "Temperature: %.2f°C, Humidity: %.2f%%", 
             data->temperature, data->humidity);
}

// Public API Implementation

esp_err_t sensor_manager_init(void)
{
    if (initialized) {
        ESP_LOGW(TAG, "Sensor manager already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing sensor manager for AHT21...");

    // Initialize I2C bus
    esp_err_t ret = init_i2c_bus();
    if (ret != ESP_OK) {
        return ret;
    }

    // Add AHT21 device
    ret = add_aht21_device();
    if (ret != ESP_OK) {
        i2c_del_master_bus(i2c_bus_handle);
        i2c_bus_handle = NULL;
        return ret;
    }

    // Soft reset AHT21
    ret = aht21_soft_reset();
    if (ret != ESP_OK) {
        i2c_master_bus_rm_device(aht21_dev_handle);
        i2c_del_master_bus(i2c_bus_handle);
        aht21_dev_handle = NULL;
        i2c_bus_handle = NULL;
        return ret;
    }

    // Initialize AHT21
    ret = aht21_init();
    if (ret != ESP_OK) {
        i2c_master_bus_rm_device(aht21_dev_handle);
        i2c_del_master_bus(i2c_bus_handle);
        aht21_dev_handle = NULL;
        i2c_bus_handle = NULL;
        return ret;
    }

    initialized = true;
    ESP_LOGI(TAG, "Sensor manager initialized successfully");
    return ESP_OK;
}

esp_err_t sensor_manager_read(sensor_data_t *data)
{
    if (!initialized) {
        ESP_LOGE(TAG, "Sensor manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL) {
        ESP_LOGE(TAG, "Invalid data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize data structure
    memset(data, 0, sizeof(sensor_data_t));
    data->valid = false;

    // Read raw data (7 bytes)
    uint8_t raw_data[7];
    esp_err_t ret = aht21_read_raw(raw_data, sizeof(raw_data));
    if (ret != ESP_OK) {
        return ret;
    }

    // Convert raw data to temperature and humidity
    aht21_convert_data(raw_data, data);

    if (!data->valid) {
        ESP_LOGW(TAG, "Sensor data not valid");
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t sensor_manager_deinit(void)
{
    if (!initialized) {
        ESP_LOGW(TAG, "Sensor manager not initialized");
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    // Remove device
    if (aht21_dev_handle != NULL) {
        ret = i2c_master_bus_rm_device(aht21_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remove AHT21 device: %s", esp_err_to_name(ret));
        }
        aht21_dev_handle = NULL;
    }

    // Delete bus
    if (i2c_bus_handle != NULL) {
        ret = i2c_del_master_bus(i2c_bus_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete I2C bus: %s", esp_err_to_name(ret));
        }
        i2c_bus_handle = NULL;
    }

    initialized = false;
    ESP_LOGI(TAG, "Sensor manager deinitialized");
    return ret;
}

bool sensor_manager_is_initialized(void)
{
    return initialized;
}
