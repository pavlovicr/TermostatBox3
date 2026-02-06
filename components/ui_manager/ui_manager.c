/**
 * @file ui_manager.c
 * @brief UI Manager implementacija
 */
#include "ui_manager.h"
#include "display_manager.h"   // ← DODAJ TA INCLUDE!
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "ui_mgr";

// UI elementi (privatno)
static lv_obj_t *temp_label = NULL;
static lv_obj_t *hum_label = NULL;
static lv_obj_t *target_temp_label = NULL;
static lv_obj_t *furnace_status_label = NULL;

/**
 * @brief Kreira glavni thermostat screen
 */
static void create_main_screen(void)
{
    lv_obj_t *screen = display_manager_get_screen();
    
    // Naslov
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Termostat Box 3");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Trenutna temperatura (veliko)
    temp_label = lv_label_create(screen);
    lv_label_set_text(temp_label, "--.-°C");
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -40);
    
    // Vlažnost
    hum_label = lv_label_create(screen);
    lv_label_set_text(hum_label, "--.-%");
    lv_obj_set_style_text_font(hum_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(hum_label, lv_color_hex(0x00BFFF), 0);
    lv_obj_align(hum_label, LV_ALIGN_CENTER, 0, 20);
    
    // Target temperatura
    target_temp_label = lv_label_create(screen);
    lv_label_set_text(target_temp_label, "Target: 21.0°C");
    lv_obj_set_style_text_font(target_temp_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(target_temp_label, lv_color_hex(0xFFAA00), 0);
    lv_obj_align(target_temp_label, LV_ALIGN_BOTTOM_MID, 0, -60);
    
    // Status peči
    furnace_status_label = lv_label_create(screen);
    lv_label_set_text(furnace_status_label, "● OFF");
    lv_obj_set_style_text_font(furnace_status_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(furnace_status_label, lv_color_hex(0x808080), 0);
    lv_obj_align(furnace_status_label, LV_ALIGN_BOTTOM_MID, 0, -30);
    
    ESP_LOGI(TAG, "Main screen created");
}

esp_err_t ui_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing UI manager...");
    
    bsp_display_lock(0);
    create_main_screen();
    bsp_display_unlock();
    
    ESP_LOGI(TAG, "UI manager initialized");
    return ESP_OK;
}

void ui_manager_update_temperature(float temperature, bool valid)
{
    char temp_str[32];
    
    if (valid) {
        snprintf(temp_str, sizeof(temp_str), "%.1f°C", temperature);
        
        bsp_display_lock(0);
        lv_label_set_text(temp_label, temp_str);
        lv_obj_set_style_text_color(temp_label, lv_color_hex(0x00FF00), 0);
        bsp_display_unlock();
    } else {
        bsp_display_lock(0);
        lv_label_set_text(temp_label, "ERROR");
        lv_obj_set_style_text_color(temp_label, lv_color_hex(0xFF0000), 0);
        bsp_display_unlock();
    }
}

void ui_manager_update_humidity(float humidity, bool valid)
{
    char hum_str[32];
    
    if (valid) {
        snprintf(hum_str, sizeof(hum_str), "%.1f%%", humidity);
        
        bsp_display_lock(0);
        lv_label_set_text(hum_label, hum_str);
        lv_obj_set_style_text_color(hum_label, lv_color_hex(0x00BFFF), 0);
        bsp_display_unlock();
    } else {
        bsp_display_lock(0);
        lv_label_set_text(hum_label, "ERROR");
        lv_obj_set_style_text_color(hum_label, lv_color_hex(0xFF0000), 0);
        bsp_display_unlock();
    }
}

void ui_manager_show_sensor_error(void)
{
    bsp_display_lock(0);
    lv_label_set_text(temp_label, "SENSOR");
    lv_label_set_text(hum_label, "ERROR");
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_color(hum_label, lv_color_hex(0xFF0000), 0);
    bsp_display_unlock();
}

void ui_manager_update_furnace_status(const char *status, uint32_t color)
{
    if (status == NULL) return;
    
    char status_str[64];
    snprintf(status_str, sizeof(status_str), "● %s", status);
    
    bsp_display_lock(0);
    lv_label_set_text(furnace_status_label, status_str);
    lv_obj_set_style_text_color(furnace_status_label, lv_color_hex(color), 0);
    bsp_display_unlock();
}

void ui_manager_set_target_temperature(float target_temp)
{
    char target_str[32];
    snprintf(target_str, sizeof(target_str), "Target: %.1f°C", target_temp);
    
    bsp_display_lock(0);
    lv_label_set_text(target_temp_label, target_str);
    bsp_display_unlock();
}