
/**
 * @file display_manager.c
 * @brief Display Manager implementacija
 */

#include "display_manager.h" 
#include "bsp/esp-bsp.h"           // ESP-BOX-3 BSP
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "display_mgr";

// Globalne spremenljivke
static lv_obj_t *main_screen = NULL;
static esp_timer_handle_t lvgl_timer = NULL;

// Forward deklaracije
static void lvgl_timer_callback(void *arg);

/**
 * @brief LVGL timer callback - klican periodično za LVGL processing
 */
static void lvgl_timer_callback(void *arg)
{
    // lv_timer_handler procesira vse pending LVGL taske
    // Ta funkcija MORA biti klicana periodično!
    lv_timer_handler();
}

esp_err_t display_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing display manager...");

    // 1. Inicializacija ESP-BOX-3 Board Support Package
    //    To nastavi vse peripherals (display, touch, audio, itd.)
   
    lv_display_t *display = bsp_display_start();
    if (display == NULL) {
    ESP_LOGE(TAG, "Failed to start display");
    return ESP_FAIL;
    }

    // 2. Konfiguracija display brightness
    bsp_display_backlight_on();
    
    // 3. LVGL lock (pomembno za thread safety!)
    //    ESP-BOX-3 BSP ustvari mutex za LVGL operacije
    bsp_display_lock(0);
    
    // 4. Kreiranje glavnega screena
    main_screen = lv_obj_create(lv_screen_active());
    
    lv_obj_set_size(main_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x003a57), 0);
    
    bsp_display_unlock();

    // 5. Kreiranje periodic timerja za LVGL
    //    LVGL potrebuje periodične klice lv_timer_handler()
    const esp_timer_create_args_t timer_args = {
        .callback = lvgl_timer_callback,
        .name = "lvgl_timer",
        .skip_unhandled_events = true,
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &lvgl_timer));
    
    // Štartaj timer - klic na vsakih 10ms
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_timer, 10 * 1000)); // 10ms v mikrosekundah

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
    
    // ESP-BOX-3 BSP ima funkcijo za brightness control
    bsp_display_brightness_set(brightness);
    
    ESP_LOGI(TAG, "Display brightness set to %d%%", brightness);
    return ESP_OK;
}
