/**
 * @file shelly_manager.c
 * @brief Shelly 2PM Gen3 HTTP API implementation (simplified, no JSON parsing)
 */
#include "shelly_manager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "shelly_mgr";

static char shelly_ip[32] = {0};

esp_err_t shelly_manager_init(const char *ip_address)
{
    if (ip_address == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(shelly_ip, ip_address, sizeof(shelly_ip) - 1);
    ESP_LOGI(TAG, "Shelly manager initialized for IP: %s", shelly_ip);
    
    return ESP_OK;
}

void shelly_manager_set_ip(const char *ip_address)
{
    if (ip_address) {
        strncpy(shelly_ip, ip_address, sizeof(shelly_ip) - 1);
    }
}

esp_err_t shelly_manager_set_relay(uint8_t channel, bool on)
{
    if (channel > 1) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char url[128];
    snprintf(url, sizeof(url), 
             "http://%s/relay/%d?turn=%s", 
             shelly_ip, channel, on ? "on" : "off");
    
    ESP_LOGI(TAG, "Setting relay %d to %s", channel, on ? "ON" : "OFF");
    ESP_LOGI(TAG, "URL: %s", url);
    
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d", status_code);
        
        if (status_code == 200) {
            ESP_LOGI(TAG, "Relay command successful");
        } else {
            ESP_LOGW(TAG, "Unexpected status code: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err;
}

/**
 * @brief Simple string search helper
 */
static bool str_contains(const char *haystack, const char *needle)
{
    return strstr(haystack, needle) != NULL;
}

/**
 * @brief Extract float value after key in JSON-like string
 * Example: "power":123.45  -> returns 123.45
 */
static float extract_float_value(const char *str, const char *key)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char *pos = strstr(str, search);
    if (pos) {
        pos += strlen(search);
        return atof(pos);
    }
    return 0.0f;
}

esp_err_t shelly_manager_get_status(shelly_status_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(status, 0, sizeof(shelly_status_t));
    
    char url[128];
    snprintf(url, sizeof(url), "http://%s/status", shelly_ip);
    
    char response_buffer[1024] = {0};
    
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_open(client, 0);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        status->online = false;
        return err;
    }
    
    int content_length = esp_http_client_fetch_headers(client);//če rabiš.
    int data_read = esp_http_client_read_response(client, response_buffer, sizeof(response_buffer) - 1);
    
    if (data_read >= 0) {
        response_buffer[data_read] = '\0';
        
        ESP_LOGI(TAG, "Received %d bytes", data_read);
        ESP_LOGD(TAG, "Response: %s", response_buffer);
        
        // Simple string parsing (no JSON library needed)
        // Shelly Gen3 response format: {"relays":[{"ison":true,...},...],...}
        
        // Check relay 0 status
        // Look for first "ison" in relays array
        const char *relay0_pos = strstr(response_buffer, "\"relays\":[{");
        if (relay0_pos) {
            const char *ison_pos = strstr(relay0_pos, "\"ison\":");
            if (ison_pos) {
                status->output_0 = str_contains(ison_pos, "true");
            }
            
            // Look for second relay (after first closing brace)
            const char *relay1_pos = strstr(ison_pos, "},{");
            if (relay1_pos) {
                const char *ison2_pos = strstr(relay1_pos, "\"ison\":");
                if (ison2_pos) {
                    status->output_1 = str_contains(ison2_pos, "true");
                }
            }
        }
        
        // Extract power values
        const char *meters_pos = strstr(response_buffer, "\"meters\":[{");
        if (meters_pos) {
            status->power_0 = extract_float_value(meters_pos, "power");
            
            const char *meter1_pos = strstr(meters_pos, "},{");
            if (meter1_pos) {
                status->power_1 = extract_float_value(meter1_pos, "power");
            }
        }
        
        // Extract temperature
        status->temperature = extract_float_value(response_buffer, "tC");
        
        status->online = true;
        
        ESP_LOGI(TAG, "Status: Relay0=%s, Relay1=%s, Power0=%.1fW, Power1=%.1fW, Temp=%.1f°C",
                 status->output_0 ? "ON" : "OFF",
                 status->output_1 ? "ON" : "OFF",
                 status->power_0,
                 status->power_1,
                 status->temperature);
    } else {
        ESP_LOGE(TAG, "Failed to read response");
        status->online = false;
        err = ESP_FAIL;
    }
    
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    return err;
}