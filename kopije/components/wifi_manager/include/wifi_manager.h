/**
 * @file wifi_manager.h
 * @brief WiFi connection manager
 */
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief WiFi event callback
 * @param connected True če je WiFi povezan
 * @param ip_address IP naslov (če je povezan)
 */
typedef void (*wifi_event_callback_t)(bool connected, const char *ip_address);

/**
 * @brief Inicializira WiFi manager
 * @return ESP_OK če uspešno
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Poveži se na WiFi (blocking)
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @param timeout_ms Timeout v ms (0 = neskončno)
 * @return ESP_OK če uspešno povezan
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password, uint32_t timeout_ms);

/**
 * @brief Preveri ali je WiFi povezan
 * @return True če je povezan
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Dobi IP naslov (string format)
 * @param ip_str Buffer za IP (min 16 bytes)
 */
void wifi_manager_get_ip(char *ip_str, size_t max_len);

/**
 * @brief Registriraj callback za WiFi events
 * @param callback Callback funkcija
 */
void wifi_manager_register_callback(wifi_event_callback_t callback);

/**
 * @brief Dobi RSSI (signal strength)
 * @return RSSI v dBm (-100 do 0)
 */
int8_t wifi_manager_get_rssi(void);

#endif // WIFI_MANAGER_H