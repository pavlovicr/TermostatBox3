/**
 * @file display_manager.h
 * @brief Display Manager - hardware abstraction za ESP-BOX-3 display
 */
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "esp_err.h"
#include "lvgl.h"

/**
 * @brief Inicializira display manager
 * @return ESP_OK če uspešno
 */
esp_err_t display_manager_init(void);

/**
 * @brief Dobi aktivni LVGL screen
 * @return Pointer na aktivni screen
 */
lv_obj_t* display_manager_get_screen(void);

/**
 * @brief Nastavi svetlost backlight-a
 * @param brightness Vrednost 0-100%
 * @return ESP_OK če uspešno
 */
esp_err_t display_manager_set_brightness(uint8_t brightness);

#endif // DISPLAY_MANAGER_H