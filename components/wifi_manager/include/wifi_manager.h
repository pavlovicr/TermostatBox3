/**
 * @file wifi_manager.h
 * @brief WiFi connection manager
 */
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief WiFi status callback
 * @param connected True če je povezan, false če je odpojen
 */
typedef void (*wifi_status_callback_t)(bool connected);

/**
 * @brief Inicializira WiFi manager
 * @return ESP_OK če uspešno
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Poveži se na WiFi
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return ESP_OK če uspešno
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief Preveri ali je WiFi povezan
 * @return True če je povezan
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Registriraj callback za spremembo statusa
 * @param callback Callback funkcija
 */
void wifi_manager_register_status_callback(wifi_status_callback_t callback);

/**
 * @brief Dobi IP naslov
 * @param ip_str Buffer za IP string (min 16 bytes)
 */
void wifi_manager_get_ip(char *ip_str);

#endif // WIFI_MANAGER_H