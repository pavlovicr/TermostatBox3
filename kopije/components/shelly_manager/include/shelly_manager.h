/**
 * @file shelly_manager.h
 * @brief Shelly 2PM Gen3 communication manager
 */
#ifndef SHELLY_MANAGER_H
#define SHELLY_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Shelly status struktura
 */
typedef struct {
    bool output_0;          // Relay 0 status (ON/OFF)
    bool output_1;          // Relay 1 status
    float power_0;          // Power consumption channel 0 (W)
    float power_1;          // Power consumption channel 1 (W)
    float temperature;      // Internal temperature (°C)
    bool online;            // Je Shelly dosegljiv
} shelly_status_t;

/**
 * @brief Inicializira Shelly manager
 * @param ip_address Shelly IP naslov (npr. "192.168.1.100")
 * @return ESP_OK če uspešno
 */
esp_err_t shelly_manager_init(const char *ip_address);

/**
 * @brief Vklopi/izklopi relay
 * @param channel Channel (0 ali 1)
 * @param on True za vklop, false za izklop
 * @return ESP_OK če uspešno
 */
esp_err_t shelly_manager_set_relay(uint8_t channel, bool on);

/**
 * @brief Preberi status Shelly naprave
 * @param status Output struktura za status
 * @return ESP_OK če uspešno
 */
esp_err_t shelly_manager_get_status(shelly_status_t *status);

/**
 * @brief Nastavi IP naslov Shelly naprave
 * @param ip_address Nova IP naslov
 */
void shelly_manager_set_ip(const char *ip_address);

#endif // SHELLY_MANAGER_H