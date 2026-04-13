/**
 * @file ui_manager.c
 * @brief UI Manager implementation
 */
#include "ui_manager.h"
#include "display_manager.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>


static const char *TAG = "ui_mgr";

// File-scope spremenljivke - UI elementi
static lv_obj_t *temp_label = NULL;
static lv_obj_t *hum_label = NULL;
static lv_obj_t *target_temp_label = NULL;
static lv_obj_t *furnace_status_label = NULL;
static lv_obj_t *wifi_status_label = NULL;
static lv_obj_t *power_label = NULL;

//==============NOVO=================

static lv_obj_t *btn_minus = NULL;
static lv_obj_t *btn_plus = NULL;

//Dodaj button callback funkcije:








//====================================================================
/**
 * @brief WiFi ikona (text-based)
 */
static const char* get_wifi_icon(int8_t rssi, bool connected)
{
    if (!connected) return "📡❌";
    if (rssi > -50) return "📡▂▄▆█";  // Excellent
    if (rssi > -60) return "📡▂▄▆";   // Good
    if (rssi > -70) return "📡▂▄";    // Fair
    return "📡▂";                      // Weak
}

/**
 * @brief Kreira glavni thermostat screen
 */
static void create_main_screen(void)
{
    lv_obj_t *screen = display_manager_get_screen();

    // ═══════════════════════════════════════════════
    // BACKGROUND COLOR 
    // ═══════════════════════════════════════════════
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x003a57), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    // ═══════════════════════════════════════════════
    // HEADER - WiFi status
    // ═══════════════════════════════════════════════
    wifi_status_label = lv_label_create(screen);
    lv_label_set_text(wifi_status_label, "📡 Connecting...");
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_LEFT, 5, 5);
    
    // ═══════════════════════════════════════════════
    // TITLE
    // ═══════════════════════════════════════════════
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Termostat Box 3");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    
    // ═══════════════════════════════════════════════
    // TEMPERATURE (large display)
    // ═══════════════════════════════════════════════
    temp_label = lv_label_create(screen);
    lv_label_set_text(temp_label, "--.-°C");
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -50);
    
    // ═══════════════════════════════════════════════
    // HUMIDITY
    // ═══════════════════════════════════════════════
    hum_label = lv_label_create(screen);
    lv_label_set_text(hum_label, "--.-%");
    lv_obj_set_style_text_font(hum_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hum_label, lv_color_hex(0x00BFFF), 0);
    lv_obj_align(hum_label, LV_ALIGN_CENTER, 0, 10);
    
    // ═══════════════════════════════════════════════
    // TARGET TEMPERATURE
    // ═══════════════════════════════════════════════
    target_temp_label = lv_label_create(screen);
    lv_label_set_text(target_temp_label, "Target: 21.0°C");
    lv_obj_set_style_text_font(target_temp_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(target_temp_label, lv_color_hex(0xFFAA00), 0);
    lv_obj_align(target_temp_label, LV_ALIGN_CENTER, 0, 50);
    
    // ═══════════════════════════════════════════════
    // FURNACE STATUS
    // ═══════════════════════════════════════════════
    furnace_status_label = lv_label_create(screen);
    lv_label_set_text(furnace_status_label, "🔥 OFF");
    lv_obj_set_style_text_font(furnace_status_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(furnace_status_label, lv_color_hex(0x808080), 0);
    lv_obj_align(furnace_status_label, LV_ALIGN_BOTTOM_MID, 0, -50);
    
    // ═══════════════════════════════════════════════
    // POWER MONITORING
    // ═══════════════════════════════════════════════
    power_label = lv_label_create(screen);
    lv_label_set_text(power_label, "⚡ --- W");
    lv_obj_set_style_text_font(power_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(power_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(power_label, LV_ALIGN_BOTTOM_MID, 0, -25);

    // ═══════════════════════════════════════════════
    // MINUS BUTTON
    // ═══════════════════════════════════════════════
    btn_minus = lv_btn_create(screen);
    lv_obj_set_size(btn_minus, 80, 60);
    lv_obj_align(btn_minus, LV_ALIGN_CENTER, -50, 80);  // Levo od centra
    
    // Button style
    lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x404040), 0);
    lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x606060), LV_STATE_PRESSED);
    
    // Label "-" v gumbu
    lv_obj_t *label_minus = lv_label_create(btn_minus);
    lv_label_set_text(label_minus, "-");
    lv_obj_set_style_text_font(label_minus, &lv_font_montserrat_32, 0);
    lv_obj_center(label_minus);
    
    // Registriraj event
    //lv_obj_add_event_cb(btn_minus, btn_minus_cb, LV_EVENT_CLICKED, NULL);
    
    // ═══════════════════════════════════════════════
    // PLUS BUTTON
    // ═══════════════════════════════════════════════
    btn_plus = lv_btn_create(screen);
    lv_obj_set_size(btn_plus, 80, 60);
    lv_obj_align(btn_plus, LV_ALIGN_CENTER, 50, 80);  // Desno od centra
    
    // Button style
    lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x404040), 0);
    lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x606060), LV_STATE_PRESSED);
    
    // Label "+" v gumbu
    lv_obj_t *label_plus = lv_label_create(btn_plus);
    lv_label_set_text(label_plus, "+");
    lv_obj_set_style_text_font(label_plus, &lv_font_montserrat_32, 0);
    lv_obj_center(label_plus);
    
    // Registriraj event
    //lv_obj_add_event_cb(btn_plus, btn_plus_cb, LV_EVENT_CLICKED, NULL);
    
   
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
        snprintf(hum_str, sizeof(hum_str), "💧%.1f%%", humidity);
        
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
    
    // Dodaj ikono glede na status
    if (strcmp(status, "HEATING") == 0) {
        snprintf(status_str, sizeof(status_str), "🔥 %s", status);
    } else if (strcmp(status, "OFF") == 0) {
        snprintf(status_str, sizeof(status_str), "❄️ %s", status);
    } else {
        snprintf(status_str, sizeof(status_str), "⚠️ %s", status);
    }
    
    bsp_display_lock(0);
    lv_label_set_text(furnace_status_label, status_str);
    lv_obj_set_style_text_color(furnace_status_label, lv_color_hex(color), 0);
    bsp_display_unlock();
}

void ui_manager_set_target_temperature(float target_temp)
{
    char target_str[32];
    snprintf(target_str, sizeof(target_str), "🎯 Target: %.1f°C", target_temp);
    
    bsp_display_lock(0);
    lv_label_set_text(target_temp_label, target_str);
    bsp_display_unlock();
}

void ui_manager_update_wifi_status(bool connected, const char *ip_address, int8_t rssi)
{
    char wifi_str[64];
    
    if (connected && ip_address) {
        const char *icon = get_wifi_icon(rssi, true);
        snprintf(wifi_str, sizeof(wifi_str), "%s %s", icon, ip_address);
    } else {
        snprintf(wifi_str, sizeof(wifi_str), "📡 Disconnected");
    }
    
    bsp_display_lock(0);
    lv_label_set_text(wifi_status_label, wifi_str);
    lv_obj_set_style_text_color(wifi_status_label, 
                                  connected ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    bsp_display_unlock();
}

void ui_manager_update_power(float power_w, bool online)
{
    char power_str[32];
    
    if (online) {
        if (power_w > 0) {
            snprintf(power_str, sizeof(power_str), "⚡ %.0f W", power_w);
        } else {
            snprintf(power_str, sizeof(power_str), "⚡ 0 W");
        }
    } else {
        snprintf(power_str, sizeof(power_str), "⚡ Offline");
    }
    
    bsp_display_lock(0);
    lv_label_set_text(power_label, power_str);
    lv_obj_set_style_text_color(power_label, 
                                  online ? lv_color_hex(0xFFFF00) : lv_color_hex(0xFF0000), 0);
    bsp_display_unlock();
}
