
/**
 * @file display_manager.c
 * @brief Display Manager implementacija
 */
#include "display_manager.h" 
#include "bsp/esp-bsp.h"
#include "esp_log.h"

static const char *TAG = "display_mgr";

// File-scope spremenljivke
static lv_display_t *display_handle = NULL;

esp_err_t display_manager_init(void)
{
    ESP_LOGI(TAG, "Inicializiram display manager...");
    
    // BSP inicializacija (ustvari LVGL task + display)
    display_handle = bsp_display_start();
    if (display_handle == NULL) {
        ESP_LOGE(TAG, "Napaka pri startu displaya");
        return ESP_FAIL;
    }
    
    // Vklopi backlight
    bsp_display_backlight_on();
    
    ESP_LOGI(TAG, "Display je uspešno inicializiran");
    return ESP_OK;
}

lv_obj_t* display_manager_get_screen(void)
{
    return lv_scr_act();  // Direktno iz LVGL
}

esp_err_t display_manager_set_brightness(uint8_t brightness)
{
    if (brightness > 100) {
        ESP_LOGE(TAG, "Invalid brightness value: %d (must be 0-100)", brightness);
        return ESP_ERR_INVALID_ARG;
    }
    
    bsp_display_brightness_set(brightness);
    ESP_LOGI(TAG, "Display brightness set to %d%%", brightness);
    return ESP_OK;
}