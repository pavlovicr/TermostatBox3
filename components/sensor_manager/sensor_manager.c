/**
 * \image html arlekino-joker.jpg "arlekino" height=200px   
 * @file sensor_manager.c
 * @brief Sensor Manager implementacija za AHT30 senzor
 * \image html AHT30.png "AHT30"
 *
 * Ta modul omogoča:
 * - inicializacijo I2C vodila
 * - komunikacijo z AHT30 senzorjem
 * - branje temperature in vlage
 */

#include "sensor_manager.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "sensor_manager";

/** @brief GPIO pin za I2C SCL */
#define I2C_MASTER_SCL_IO           40      
/** @brief GPIO pin za I2C SDA */
#define I2C_MASTER_SDA_IO           41  
/** @brief Frekvenca I2C vodila */
#define I2C_MASTER_FREQ_HZ          100000  
/** @brief Timeout za I2C operacije v milisekundah */
#define I2C_MASTER_TIMEOUT_MS       1000

/** @brief Adresa AHT30 senzorja na I2C vodilu */
#define AHT30_I2C_ADDR              0x38 
/** @brief Ukaz za inicializacijo AHT30 senzorja. V data sheetu ni naveden, ker ga verjetno ne potrebuje tako kot za AHT21 */  
#define AHT30_CMD_INIT              0xBE
/** @brief Ukaz za zagon meritve */
#define AHT30_CMD_TRIGGER           0xAC 
/** @brief Ukaz za mehki reset.Resetira napravo, brez ponovnega vklopa in izklopa */
#define AHT30_CMD_SOFT_RESET        0xBA 
/** @brief Čas, ki ga je potrebno počakati po sprožitvi merjenja, da so podatki pripravljeni */
#define AHT30_MEASUREMENT_DELAY_MS  80      

/** @brief Handle za I2C master bus */
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
/** @brief Handle za AHT30 senzor */
static i2c_master_dev_handle_t aht30_dev_handle = NULL;

/** @brief Flag, ki označuje ali je senzor manager inicializiran */
static bool initialized = false;
/** @brief Funkcija za inicializacijo I2C vodila */
static esp_err_t init_i2c_bus(void)  
{
    /** @brief Konfiguracija I2C master bus */
    i2c_master_bus_config_t bus_config = {  
         .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        /** @brief Število ignoriranih glitchev */
        .glitch_ignore_cnt = 7,
        /** @brief Flag za omogočanje notranjega pullup upora */
        .flags.enable_internal_pullup = true, 
    };
    /** @brief Kreiranje I2C master bus */
    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle); 
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri kreiranju I2C master bus: %s", esp_err_to_name(ret));
        /** @brief Vrne napako */
        return ret; 
    }
    /** @brief Uspešno kreiran I2C master bus */
    ESP_LOGI(TAG, "I2C master bus uspesno kreiran na portu 0 (SDA=GPIO%d, SCL=GPIO%d)", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

/** @brief Funkcija za dodajanje senzorja AHT30 na I2C bus */
static esp_err_t add_aht30_device(void)
{
        /** @brief Konfiguracija AHT30 senzorja */
    i2c_device_config_t dev_config = { 
        /** @brief Dolžina I2C naslova (7-bitni) iz data sheet AHT30*/
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, 
        .device_address = AHT30_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    /** @brief Dodajanje AHT30 senzorja na I2C bus */
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &aht30_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri prikljucevanju AHT30 na bus: %s", esp_err_to_name(ret));
        return ret;
    }
    /** @brief Uspešno dodan AHT30 senzor na I2C bus */
    ESP_LOGI(TAG, "AHT30 senzor dodan na bus na adresi 0x%02X", AHT30_I2C_ADDR);
    return ESP_OK;
}

/** @brief Sedaj, ko je senzor na vodilu mu lahko pošljemo vrednosti, ki jih bo prebral, se resetiral in ponastavil. 
Iz data sheets izhaja , da je senzor sposoben sprejemat sporočila od masterja v 100-500 ms 
po priključitvi na napajanje 2.V-5.5V  */
/** @brief Funkcija za mehki reset AHT30 senzorja       */
static esp_err_t aht30_soft_reset(void) 
{   /** @brief Pošlji ukaz za mehki reset AHT30 senzorju */
    uint8_t cmd = AHT30_CMD_SOFT_RESET;
    /** @brief Pošlji ukaz za reset in preveri rezultat */
    esp_err_t ret = i2c_master_transmit(aht30_dev_handle, &cmd, 1, I2C_MASTER_TIMEOUT_MS); //write ukaz
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri posiljanju reset vrednosti: %s", esp_err_to_name(ret));
        return ret; 
    }
    /** @brief Počakaj, da se reset zaključi */
    vTaskDelay(pdMS_TO_TICKS(20));  
    ESP_LOGI(TAG, "AHT30 soft reset zakljucen");
    return ESP_OK;
}

/** @brief Funkcija za inicializacijo AHT30 senzorja. Senzorju AHT30 pošljemo paket z vrednostmi za inicializacijo senzorja. Podatki  
so navedeni v data sheet. 0xBE, 0x08, 0x00 */
static esp_err_t aht30_init(void) 
{
    /** @brief 0xBE, 0x08, 0x00 */
    uint8_t init_cmd[3] = {AHT30_CMD_INIT, 0x08, 0x00};
    esp_err_t ret = i2c_master_transmit(aht30_dev_handle, init_cmd, 3, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Inicializacija AHT30 ni uspela: %s", esp_err_to_name(ret));
        return ret;
    }
    /** @brief Počakaj, da se inicializacija zaključi */
    vTaskDelay(pdMS_TO_TICKS(10));  
    ESP_LOGI(TAG, "AHT30 inicializacija uspesna");
    return ESP_OK;
}

/** @brief Funkcija za branje surovih podatkov iz AHT30 senzorja. Sprožimo(trigger) merjenje (measurement) in branje data iz AHT30 tako , da mu pošljemo
vrednosti iz data sheet  0xAC, 0x33, 0x00
*/
static esp_err_t aht30_read_raw(uint8_t *data, size_t len)
{
    /** @brief Trigger measurement: 0xAC, 0x33, 0x00 */
    uint8_t trigger_cmd[3] = {AHT30_CMD_TRIGGER, 0x33, 0x00};
    /** @brief Pošlji ukaz za sprožitev merjenja in preveri rezultat */
    esp_err_t ret = i2c_master_transmit(aht30_dev_handle, trigger_cmd, 3, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri prozenju merjenj: %s", esp_err_to_name(ret));
        return ret;
    }

    /** @brief Počakamo , da se merjenje zaključi */    
    vTaskDelay(pdMS_TO_TICKS(AHT30_MEASUREMENT_DELAY_MS));

    /** @brief Read 7 bytes of data */
    ret = i2c_master_receive(aht30_dev_handle, data, len, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri branju measurement data: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/** @brief Funkcija za konvertiranje surovih (raw) podatkov v temperaturo in vlago */
static void aht30_convert_data(uint8_t *raw_data, sensor_data_t *data)
{
    /** @brief Preveri status bit (bit 7 prvega bajta). Če je nastavljen, so meritve nezanesljive */
    if (raw_data[0] & 0x80) {
        ESP_LOGW(TAG, "Senzor je  zaseden, meritve so nezanesljive ");
        data->valid = false;
        return;
    }

    /** @brief Extract vlage (20 bits: bits 12-31 od data) */
    uint32_t humidity_raw = ((uint32_t)raw_data[1] << 12) | 
                            ((uint32_t)raw_data[2] << 4) | 
                            ((uint32_t)raw_data[3] >> 4);
    
    /** @brief Extract temperature (20 bits: bits 32-51 od data) */
    uint32_t temperature_raw = (((uint32_t)raw_data[3] & 0x0F) << 16) | 
                               ((uint32_t)raw_data[4] << 8) | 
                               (uint32_t)raw_data[5];

    /** @brief Konvertiraj aktualne vrednosti */
    data->humidity = ((float)humidity_raw / 1048576.0f) * 100.0f;
    /** @brief Tovarniška vrednost je 200.0f-50.0f */
    data->temperature = ((float)temperature_raw / 1048576.0f) * 200.0f - 50.0f;
    data->valid = true;

    ESP_LOGD(TAG, "Temperatura: %.2f°C, Vlaga: %.2f%%", 
             data->temperature, data->humidity);
}

/** @brief Funkcija za inicializacijo sensor managerja, ki vključuje inicializacijo I2C vodila in dodajanje AHT30 senzorja. Prav tako se izvede mehki reset in inicializacija senzorja. V primeru napake se ustrezno očistijo viri. */

/** @brief Inicializacija sensor managerja */

esp_err_t sensor_manager_init(void)
{
    if (initialized) {
        ESP_LOGW(TAG, "Senzor je ze inicializiran");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Inicializiram sensor manager za AHT30...");

    /** @brief Inicializacija I2C bus */    
    esp_err_t ret = init_i2c_bus();
    if (ret != ESP_OK) {
        return ret;
    }

    /** @brief Dodajanje AHT30 senzorja */
    ret = add_aht30_device();
    if (ret != ESP_OK) {
        i2c_del_master_bus(i2c_bus_handle);
        i2c_bus_handle = NULL;
        return ret;
    }

    /** @brief Soft reset AHT30 */
    ret = aht30_soft_reset();
    if (ret != ESP_OK) {
        i2c_master_bus_rm_device(aht30_dev_handle);
        i2c_del_master_bus(i2c_bus_handle);
        aht30_dev_handle = NULL;
        i2c_bus_handle = NULL;
        return ret;
    }

    /** @brief Inicializacija AHT30 */
    ret = aht30_init();
    if (ret != ESP_OK) {
        i2c_master_bus_rm_device(aht30_dev_handle);
        i2c_del_master_bus(i2c_bus_handle);
        aht30_dev_handle = NULL;
        i2c_bus_handle = NULL;
        return ret;
    }

    initialized = true;
    ESP_LOGI(TAG, "Sensor manager je uspesno inicializiran");
    return ESP_OK;
}


/** @brief BRANJE IN MERJENJE */
esp_err_t sensor_manager_read(sensor_data_t *data)
{
    if (!initialized) {
        ESP_LOGE(TAG, "Sensor manager ni inicializiran");
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL) {
        ESP_LOGE(TAG, "Napacen data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    /** @brief Inicializacija data strukture */
    memset(data, 0, sizeof(sensor_data_t));
    data->valid = false;

    /** @brief Read raw data (7 bytes) */
    uint8_t raw_data[7];
    esp_err_t ret = aht30_read_raw(raw_data, sizeof(raw_data));
    if (ret != ESP_OK) {
        return ret;
    }

    /** @brief Konvertiraj raw data v temperaturo in vlago */
    aht30_convert_data(raw_data, data);

    if (!data->valid) {
        ESP_LOGW(TAG, "Sensor data not valid");
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

/** @brief ODMONTIRANJE */

esp_err_t sensor_manager_deinit(void)  
{
    if (!initialized) {
        ESP_LOGW(TAG, "Sensor manager not initialized");
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    /** @brief Remove device */
    if (aht30_dev_handle != NULL) {
        ret = i2c_master_bus_rm_device(aht30_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remove AHT30 device: %s", esp_err_to_name(ret));
        }
        aht30_dev_handle = NULL;
    }

    /** @brief Delete bus */
    if (i2c_bus_handle != NULL) {
        ret = i2c_del_master_bus(i2c_bus_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete I2C bus: %s", esp_err_to_name(ret));
        }
        i2c_bus_handle = NULL;
    }

    initialized = false;
    ESP_LOGI(TAG, "Sensor manager deinitialized");
    return ret;
}

/** @brief UGOTOVITEV */

bool sensor_manager_is_initialized(void)
{
    return initialized;
}
