/**
 * @file main.c
 * @brief ESP32-S3-BOX-3 Thermostat z WiFi + Shelly 2PM kontrolo
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Config
#include "config.h"

// Managerji
#include "display_manager.h"
#include "sensor_manager.h"
#include "ui_manager.h"
#include "wifi_manager.h"
#include "shelly_manager.h"
#include "furnace_controller.h"

static const char *TAG = "main";

// Globalne spremenljivke
static float target_temperature = DEFAULT_TARGET_TEMP;

// ═══════════════════════════════════════════════════════════
// WiFi Event Callback
// ═══════════════════════════════════════════════════════════
static void wifi_event_cb(bool connected, const char *ip_address)
{
    if (connected) {
        ESP_LOGI(TAG, "WiFi connected! IP: %s", ip_address);
        
        int8_t rssi = wifi_manager_get_rssi();
        ui_manager_update_wifi_status(true, ip_address, rssi);
    } else {
        ESP_LOGW(TAG, "WiFi disconnected!");
        ui_manager_update_wifi_status(false, NULL, -100);
    }
}

// ═══════════════════════════════════════════════════════════
// Furnace State Callback
// ═══════════════════════════════════════════════════════════
static void furnace_state_cb(furnace_state_t state, float power_w)
{
    const char *status_text;
    uint32_t status_color;
    
    switch (state) {
        case FURNACE_HEATING:
            status_text = "HEATING";
            status_color = COLOR_FURNACE_HEATING;
            break;
        case FURNACE_OFF:
            status_text = "OFF";
            status_color = COLOR_FURNACE_OFF;
            break;
        case FURNACE_ERROR:
            status_text = "ERROR";
            status_color = COLOR_FURNACE_ERROR;
            break;
        default:
            status_text = "UNKNOWN";
            status_color = COLOR_FURNACE_OFF;
            break;
    }
    
    ui_manager_update_furnace_status(status_text, status_color);
    ui_manager_update_power(power_w, state != FURNACE_ERROR);
    
    ESP_LOGI(TAG, "Furnace: %s, Power: %.1fW", status_text, power_w);
}

// ═══════════════════════════════════════════════════════════
// Sensor Update Task
// ═══════════════════════════════════════════════════════════
static void sensor_update_task(void *arg)
{
    sensor_data_t data;
    
    ESP_LOGI(TAG, "Sensor task started");
    
    while (1) {
        esp_err_t ret = sensor_manager_read(&data);
        
        if (ret == ESP_OK && data.valid) {
            // Update UI
            ui_manager_update_temperature(data.temperature, true);
            ui_manager_update_humidity(data.humidity, true);
            
            // Update furnace controller (Shelly control logic)
            if (wifi_manager_is_connected()) {
                furnace_controller_update_temperature(data.temperature);
            } else {
                ESP_LOGW(TAG, "WiFi not connected, skipping Shelly control");
            }
            
            ESP_LOGI(TAG, "Sensor: T=%.1f°C, H=%.1f%%, Target=%.1f°C", 
                     data.temperature, data.humidity, target_temperature);
        } else {
            ESP_LOGW(TAG, "Sensor read failed");
            ui_manager_show_sensor_error();
        }
        
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

// ═══════════════════════════════════════════════════════════
// WiFi Status Monitor Task
// ═══════════════════════════════════════════════════════════
static void wifi_monitor_task(void *arg)
{
    char ip[16];
    
    while (1) {
        if (wifi_manager_is_connected()) {
            wifi_manager_get_ip(ip, sizeof(ip));
            int8_t rssi = wifi_manager_get_rssi();
            ui_manager_update_wifi_status(true, ip, rssi);
        } else {
            ui_manager_update_wifi_status(false, NULL, -100);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Vsake 5 sekund
    }
}

// ═══════════════════════════════════════════════════════════
// app_main - Entry Point
// ═══════════════════════════════════════════════════════════
void app_main(void)
{
    ESP_LOGI(TAG, "╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ESP32-S3-BOX-3 Smart Thermostat     ║");
    ESP_LOGI(TAG, "║  WiFi + Shelly 2PM Control           ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
    
    // ═══════════════════════════════════════════════════════
    // FAZA 1: Display inicializacija
    // ═══════════════════════════════════════════════════════
    ESP_LOGI(TAG, "[1/6] Initializing display...");
    ESP_ERROR_CHECK(display_manager_init());
    display_manager_set_brightness(DISPLAY_BRIGHTNESS_DEFAULT);
    
    // ═══════════════════════════════════════════════════════
    // FAZA 2: UI kreacija
    // ═══════════════════════════════════════════════════════
    ESP_LOGI(TAG, "[2/6] Creating UI...");
    ESP_ERROR_CHECK(ui_manager_init());
    ui_manager_set_target_temperature(target_temperature);
    
    // ═══════════════════════════════════════════════════════
    // FAZA 3: WiFi povezava
    // ═══════════════════════════════════════════════════════
    ESP_LOGI(TAG, "[3/6] Connecting to WiFi...");
    ESP_ERROR_CHECK(wifi_manager_init());
    wifi_manager_register_callback(wifi_event_cb);
    
    esp_err_t wifi_ret = wifi_manager_connect(WIFI_SSID, WIFI_PASSWORD, WIFI_CONNECT_TIMEOUT_MS);
    if (wifi_ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connection failed! Shelly control disabled.");
        ui_manager_update_wifi_status(false, NULL, -100);
        // Nadaljuj brez WiFi (samo local sensor + display)
    }
    
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // ═══════════════════════════════════════════════════════
    // FAZA 4: Shelly & Furnace controller
    // ═══════════════════════════════════════════════════════
    ESP_LOGI(TAG, "[4/6] Initializing Shelly & Furnace controller...");
    if (wifi_manager_is_connected()) {
        esp_err_t furnace_ret = furnace_controller_init(SHELLY_IP_ADDRESS, SHELLY_FURNACE_CHANNEL);
        if (furnace_ret == ESP_OK) {
            furnace_controller_set_target(target_temperature);
            furnace_controller_register_callback(furnace_state_cb);
            ESP_LOGI(TAG, "Furnace controller ready");
        } else {
            ESP_LOGE(TAG, "Furnace controller init failed (Shelly offline?)");
            ui_manager_update_furnace_status("ERROR", COLOR_FURNACE_ERROR);
        }
    } else {
        ESP_LOGW(TAG, "WiFi not connected - skipping Shelly init");
    }
    
    // ═══════════════════════════════════════════════════════
    // FAZA 5: Sensor inicializacija
    // ═══════════════════════════════════════════════════════
    ESP_LOGI(TAG, "[5/6] Initializing sensor...");
    esp_err_t sensor_ret = sensor_manager_init();
    if (sensor_ret != ESP_OK) {
        ESP_LOGE(TAG, "Sensor init failed!");
        ESP_LOGE(TAG, "Check I2C connections: SCL=GPIO%d, SDA=GPIO%d", 
                 SENSOR_I2C_SCL_GPIO, SENSOR_I2C_SDA_GPIO);
        ui_manager_show_sensor_error();
        // Nadaljuj brez senzorja (za debug UI + Shelly)
    }
    
    // ═══════════════════════════════════════════════════════
    // FAZA 6: Štart FreeRTOS taskov
    // ═══════════════════════════════════════════════════════
    ESP_LOGI(TAG, "[6/6] Starting tasks...");
    
    // Sensor reading task
    xTaskCreate(sensor_update_task, "sensor", 4096, NULL, 5, NULL);
    
    // WiFi monitor task
    xTaskCreate(wifi_monitor_task, "wifi_monitor", 2048, NULL, 4, NULL);
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║      System Running Successfully!    ║");
    ESP_LOGI(TAG, "╠═══════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  Target: %.1f°C                       ║", target_temperature);
    ESP_LOGI(TAG, "║  Shelly: %-20s        ║", SHELLY_IP_ADDRESS);
    ESP_LOGI(TAG, "║  WiFi:   %-20s        ║", WIFI_SSID);
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
}