/**
 * @file ui_manager.h
 * @brief UI Manager - samo rendering UI elementov
 */
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializira UI manager (kreira screen elemente)
 * @return ESP_OK če uspešno
 */
esp_err_t ui_manager_init(void);

/**
 * @brief Posodobi prikaz temperature
 * @param temperature Temperatura v °C
 * @param valid Ali so podatki veljavni
 */
void ui_manager_update_temperature(float temperature, bool valid);

/**
 * @brief Posodobi prikaz vlažnosti
 * @param humidity Vlažnost v %
 * @param valid Ali so podatki veljavni
 */
void ui_manager_update_humidity(float humidity, bool valid);

/**
 * @brief Prikaži napako na senzorju
 */
void ui_manager_show_sensor_error(void);

/**
 * @brief Posodobi status peči
 * @param status Tekst statusa (npr. "HEATING", "OFF")
 * @param color Barva teksta (RGB hex)
 */
void ui_manager_update_furnace_status(const char *status, uint32_t color);

/**
 * @brief Nastavi target temperaturo (prikaz)
 * @param target_temp Target temperatura v °C
 */
void ui_manager_set_target_temperature(float target_temp);

#endif // UI_MANAGER_H