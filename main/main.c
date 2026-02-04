

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "display_manager.h"
#include "sensor_manager.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "main";

// UI elementi
static lv_obj_t *temp_label = NULL;
static lv_obj_t *hum_label = NULL;

/**
 * @brief Kreira UI za prikaz temperature in vlažnosti
 */
static void create_thermostat_ui(void)
{
    lv_obj_t *screen = display_manager_get_screen();
    
    bsp_display_lock(0);
    
    // Naslov
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Termostat Box 3");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Temperature display
    temp_label = lv_label_create(screen);
    lv_label_set_text(temp_label, "--.-°C");
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -30);
    
    // Humidity display
    hum_label = lv_label_create(screen);
    lv_label_set_text(hum_label, "--.-%");
    lv_obj_set_style_text_font(hum_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(hum_label, lv_color_hex(0x00BFFF), 0);
    lv_obj_align(hum_label, LV_ALIGN_CENTER, 0, 40);
    
    bsp_display_unlock();
    
    ESP_LOGI(TAG, "Thermostat UI created");
}

/**
 * @brief Posodobi prikaz temperature in vlažnosti
 */
static void update_sensor_display(void)
{
    sensor_data_t data;
    
    // Preberi podatke iz senzorja
    esp_err_t ret = sensor_manager_read(&data);
    
    if (ret == ESP_OK && data.valid) {
        char temp_str[32];
        char hum_str[32];
        
        snprintf(temp_str, sizeof(temp_str), "%.1f°C", data.temperature);
        snprintf(hum_str, sizeof(hum_str), "%.1f%%", data.humidity);
        
        bsp_display_lock(0);
        lv_label_set_text(temp_label, temp_str);
        lv_label_set_text(hum_label, hum_str);
        bsp_display_unlock();
        
        ESP_LOGI(TAG, "Temperature: %.1f°C, Humidity: %.1f%%", 
                 data.temperature, data.humidity);
    } else {
        ESP_LOGW(TAG, "Failed to read sensor data");
        
        bsp_display_lock(0);
        lv_label_set_text(temp_label, "ERROR");
        lv_label_set_text(hum_label, "ERROR");
        bsp_display_unlock();
    }
}

/**
 * @brief app_main - entry point aplikacije
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "ESP32-BOX-3 Thermostat Starting");
    ESP_LOGI(TAG, "=================================");
    
    // 1. Inicializacija display sistema
    ESP_LOGI(TAG, "Initializing display...");
    ESP_ERROR_CHECK(display_manager_init());
    
    // 2. Inicializacija senzorja (AHT21 na GPIO40/41)
    ESP_LOGI(TAG, "Initializing AHT21 sensor...");
    esp_err_t ret = sensor_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize sensor manager: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check I2C connections: SCL=GPIO40, SDA=GPIO41");
        // Nastavi display, ampak prikaži napako
        display_manager_set_brightness(100);
        create_thermostat_ui();
        
        bsp_display_lock(0);
        lv_label_set_text(temp_label, "SENSOR");
        lv_label_set_text(hum_label, "ERROR");
        bsp_display_unlock();
        
        // Ne prekini programa - lahko še vedno testiraš display
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 3. Kreiranje UI
    ESP_LOGI(TAG, "Creating UI...");
    create_thermostat_ui();
    
    // 4. Nastavitev svetlosti
    display_manager_set_brightness(100);
    
    ESP_LOGI(TAG, "Initialization complete!");
    ESP_LOGI(TAG, "Reading sensor every 2 seconds...");
    
    // 5. Main loop - periodično branje temperature in vlažnosti
    while (1) {
        update_sensor_display();
        vTaskDelay(pdMS_TO_TICKS(2000)); // Vsake 2 sekundi
    }
}
