/**
 * @file display_manager.h
 * @brief Display Manager for ESP-BOX-3 with LVGL
 * 
 * Ta komponenta inicializira in upravlja zaslon ESP-BOX-3.
 * Poskrbi za:
 * - Inicializacijo LVGL sistema
 * - Nastavitev ESP-BOX-3 driverjev
 * - Kreiranje osnovnega UI kontejnerja
 */

#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializira display manager in LVGL
 * 
 * Ta funkcija:
 * - Inicializira LVGL knjižnico
 * - Nastavi ESP-BOX-3 display driver
 * - Kreira task za LVGL timer handling
 * 
 * @return 
 *    - ESP_OK: Uspešna inicializacija
 *    - ESP_FAIL: Napaka pri inicializaciji
 */
esp_err_t display_manager_init(void);

/**
 * @brief Dobi glavni LVGL screen objekt
 * 
 * @return lv_obj_t* pointer na aktivni screen
 */
lv_obj_t* display_manager_get_screen(void);

/**
 * @brief Nastavi brightness zaslona (0-100%)
 * 
 * @param brightness Svetlost od 0 do 100
 * @return 
 *    - ESP_OK: Uspešno nastavljeno
 *    - ESP_ERR_INVALID_ARG: Napačna vrednost
 */
esp_err_t display_manager_set_brightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif
