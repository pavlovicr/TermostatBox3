/**
 * @file furnace_controller.h
 * @brief Furnace (heating) controller z Shelly 2PM
 */
#ifndef FURNACE_CONTROLLER_H
#define FURNACE_CONTROLLER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Furnace status
 */
typedef enum {
    FURNACE_OFF,       // Gretje izklopljeno
    FURNACE_HEATING,   // Aktivno greje
    FURNACE_IDLE,      // Gretje vklopljeno, ampak na target temp
    FURNACE_ERROR      // Napaka (Shelly offline, sensor fail...)
} furnace_state_t;

/**
 * @brief Furnace controller callback
 * @param state Nov state peči
 * @param power_w Trenutna poraba (W) - 0 če ni podatka
 */
typedef void (*furnace_state_callback_t)(furnace_state_t state, float power_w);

/**
 * @brief Inicializira furnace controller
 * @param shelly_ip Shelly IP naslov
 * @param relay_channel Shelly relay channel (0 ali 1)
 * @return ESP_OK če uspešno
 */
esp_err_t furnace_controller_init(const char *shelly_ip, uint8_t relay_channel);

/**
 * @brief Nastavi target temperaturo
 * @param target_temp Target temperatura v °C
 */
void furnace_controller_set_target(float target_temp);

/**
 * @brief Dobi target temperaturo
 * @return Target temperatura v °C
 */
float furnace_controller_get_target(void);

/**
 * @brief Posodobi trenutno temperaturo (kliče sensor task)
 * @param current_temp Trenutna temperatura v °C
 * @return ESP_OK če uspešno
 */
esp_err_t furnace_controller_update_temperature(float current_temp);

/**
 * @brief Dobi trenutni state peči
 * @return Furnace state
 */
furnace_state_t furnace_controller_get_state(void);

/**
 * @brief Registriraj callback za state spremembe
 * @param callback Callback funkcija
 */
void furnace_controller_register_callback(furnace_state_callback_t callback);

/**
 * @brief Ročno vklopi/izklopi peč (override)
 * @param on True za vklop
 * @return ESP_OK če uspešno
 */
esp_err_t furnace_controller_manual_override(bool on);

#endif // FURNACE_CONTROLLER_H