/**
 * @file furnace_controller.c
 * @brief Furnace controller implementation
 */
#include "furnace_controller.h"
#include "shelly_manager.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "furnace_ctrl";

// Hysteresis nastavitve
#define TEMP_HYSTERESIS_HIGH  0.3f   // °C nad target → izklopi
#define TEMP_HYSTERESIS_LOW   0.5f   // °C pod target → vklopi

static float s_target_temp = 21.0f;
static float s_current_temp = 20.0f;
static furnace_state_t s_state = FURNACE_OFF;
static furnace_state_callback_t s_callback = NULL;
static uint8_t s_relay_channel = 0;

/**
 * @brief Posodobi state in obvesti callback
 */
static void update_state(furnace_state_t new_state, float power)
{
    if (new_state != s_state) {
        ESP_LOGI(TAG, "State change: %d → %d", s_state, new_state);
        s_state = new_state;
        
        if (s_callback) {
            s_callback(new_state, power);
        }
    }
}

esp_err_t furnace_controller_init(const char *shelly_ip, uint8_t relay_channel)
{
    ESP_LOGI(TAG, "Initializing furnace controller...");
    ESP_LOGI(TAG, "Shelly IP: %s, Relay: %d", shelly_ip, relay_channel);
    
    s_relay_channel = relay_channel;
    
    esp_err_t ret = shelly_manager_init(shelly_ip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Shelly manager");
        return ret;
    }
    
    // Prvo branje statusa
    shelly_status_t status;
    ret = shelly_manager_get_status(&status);
    if (ret == ESP_OK && status.online) {
        ESP_LOGI(TAG, "Shelly is online, relay %d is %s", 
                 relay_channel,
                 (relay_channel == 0 ? status.output_0 : status.output_1) ? "ON" : "OFF");
    } else {
        ESP_LOGW(TAG, "Shelly is offline or unreachable");
        update_state(FURNACE_ERROR, 0.0f);
    }
    
    ESP_LOGI(TAG, "Furnace controller initialized");
    return ESP_OK;
}

void furnace_controller_set_target(float target_temp)
{
    if (target_temp < 10.0f || target_temp > 35.0f) {
        ESP_LOGW(TAG, "Invalid target temperature: %.1f°C (ignoring)", target_temp);
        return;
    }
    
    ESP_LOGI(TAG, "Target temperature changed: %.1f°C → %.1f°C", s_target_temp, target_temp);
    s_target_temp = target_temp;
}

float furnace_controller_get_target(void)
{
    return s_target_temp;
}

esp_err_t furnace_controller_update_temperature(float current_temp)
{
    s_current_temp = current_temp;
    
    float delta = s_target_temp - current_temp;
    
    ESP_LOGD(TAG, "Temp update: Current=%.1f°C, Target=%.1f°C, Delta=%.2f°C", 
             current_temp, s_target_temp, delta);
    
    // Thermostat logika z histerezom
    bool should_heat = false;
    
    if (delta > TEMP_HYSTERESIS_LOW) {
        // Precej hladneje → greje
        should_heat = true;
    } else if (delta < -TEMP_HYSTERESIS_HIGH) {
        // Precej toplejše → ne greje
        should_heat = false;
    } else {
        // V "gray zone" → ohrani trenutni state
        should_heat = (s_state == FURNACE_HEATING);
    }
    
    // Pošlji ukaz na Shelly
    esp_err_t ret = shelly_manager_set_relay(s_relay_channel, should_heat);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to control Shelly relay");
        update_state(FURNACE_ERROR, 0.0f);
        return ret;
    }
    
    // Preberi status za power monitoring
    shelly_status_t status;
    ret = shelly_manager_get_status(&status);
    
    float power = 0.0f;
    if (ret == ESP_OK && status.online) {
        power = (s_relay_channel == 0) ? status.power_0 : status.power_1;
        
        furnace_state_t new_state = should_heat ? FURNACE_HEATING : FURNACE_OFF;
        update_state(new_state, power);
        
        ESP_LOGI(TAG, "Furnace %s, Power: %.1fW, Temp: %.1f/%.1f°C", 
                 should_heat ? "HEATING" : "OFF", power, current_temp, s_target_temp);
    } else {
        update_state(FURNACE_ERROR, 0.0f);
    }
    
    return ESP_OK;
}

furnace_state_t furnace_controller_get_state(void)
{
    return s_state;
}

void furnace_controller_register_callback(furnace_state_callback_t callback)
{
    s_callback = callback;
}

esp_err_t furnace_controller_manual_override(bool on)
{
    ESP_LOGI(TAG, "Manual override: %s", on ? "ON" : "OFF");
    
    esp_err_t ret = shelly_manager_set_relay(s_relay_channel, on);
    if (ret == ESP_OK) {
        update_state(on ? FURNACE_HEATING : FURNACE_OFF, 0.0f);
    }
    
    return ret;
}