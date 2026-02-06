
/**
 * @file display_manager.c
 * @brief Display Manager implementacija
 */
#include "display_manager.h" 
#include "bsp/esp-bsp.h"
#include "esp_log.h"

static const char *TAG = "display_mgr";

// Globalne spremenljivke
static lv_obj_t *main_screen = NULL;
static lv_display_t *display_handle = NULL;

esp_err_t display_manager_init(void) //vse spravimo pod eno kapo 
{
    ESP_LOGI(TAG, "Initializing display manager...");
    
    // 1. BSP inicializacija
    //    To že ustvari LVGL task, ki kliče lv_timer_handler() periodično!
    display_handle = bsp_display_start();
    if (display_handle == NULL) {
        ESP_LOGE(TAG, "Failed to start display");
        return ESP_FAIL;
    }
    
    // 2. Vklopi backlight
    bsp_display_backlight_on();
    
    // 3. Thread-safe kreiranje UI elementov
    bsp_display_lock(0);
    
    // Kreiranje glavnega screena
    main_screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(main_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x003a57), 0);
    
    bsp_display_unlock();
    
    ESP_LOGI(TAG, "Display manager initialized successfully");
    return ESP_OK;
}

lv_obj_t* display_manager_get_screen(void)
{
    return main_screen;
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

lv_display_t* display_manager_get_display(void)
{
    return display_handle;
}