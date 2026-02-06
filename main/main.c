#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "config.h"
#include "display_manager.h"
#include "sensor_manager.h"
#include "ui_manager.h"

static const char *TAG = "main";

static float target_temperature = DEFAULT_TARGET_TEMP;

/**
 * @brief Task za periodično branje senzorjev
 */
static void sensor_update_task(void *arg)
{
    sensor_data_t data;  // ← sensor_manager struktura
    
    while (1) {
        esp_err_t ret = sensor_manager_read(&data);
        
        if (ret == ESP_OK && data.valid) {
            // Posodobi UI - ločeni klici za temp in humidity
            ui_manager_update_temperature(data.temperature, true);
            ui_manager_update_humidity(data.humidity, true);
            
            // Thermostat logika
            if (data.temperature < target_temperature - TEMP_HYSTERESIS) {
                ui_manager_update_furnace_status("HEATING", COLOR_FURNACE_ON);
                // TODO: gpio_set_level(FURNACE_RELAY_GPIO, 1);
            } else if (data.temperature > target_temperature + TEMP_HYSTERESIS) {
                ui_manager_update_furnace_status("OFF", COLOR_FURNACE_OFF);
                // TODO: gpio_set_level(FURNACE_RELAY_GPIO, 0);
            }
            
            ESP_LOGI(TAG, "T: %.1f°C, H: %.1f%%, Target: %.1f°C", 
                     data.temperature, data.humidity, target_temperature);
        } else {
            // Napaka pri branju senzorja
            ui_manager_show_sensor_error();
            ui_manager_update_furnace_status("ERROR", COLOR_TEMP_ERROR);
            ESP_LOGW(TAG, "Sensor read failed");
        }
        
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "ESP32-BOX-3 Thermostat v2.0");
    ESP_LOGI(TAG, "=================================");
    
    // ═══════════════════════════════════════
    // INICIALIZACIJE
    // ═══════════════════════════════════════
    
    // 1. Display HW
    ESP_LOGI(TAG, "[1/3] Initializing display...");
    ESP_ERROR_CHECK(display_manager_init());
    display_manager_set_brightness(DISPLAY_BRIGHTNESS_DEFAULT);
    
    // 2. UI rendering
    ESP_LOGI(TAG, "[2/3] Creating UI...");
    ESP_ERROR_CHECK(ui_manager_init());
    ui_manager_set_target_temperature(target_temperature);
    
    // 3. Sensor HW
    ESP_LOGI(TAG, "[3/3] Initializing sensor...");
    esp_err_t ret = sensor_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sensor init failed: %s", esp_err_to_name(ret));
        ui_manager_show_sensor_error();
        // Nadaljuj brez senzorja (za debug display-a)
    }
    
    ESP_LOGI(TAG, "Initialization complete!");
    
    // ═══════════════════════════════════════
    // ŠTART TASKOV
    // ═══════════════════════════════════════
    
    xTaskCreate(sensor_update_task, "sensor_update", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "System running...");
}
