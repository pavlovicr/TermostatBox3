
#include "sensor_manager.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "sensor_manager";

// I2C Konfiguracija GPIO za SCL in SDA
#define I2C_MASTER_SCL_IO           40      // GPIO40 for SCL
#define I2C_MASTER_SDA_IO           41      // GPIO41 for SDA
#define I2C_MASTER_FREQ_HZ          100000  // data sheet, 100kHz je max hitrost za standar mode
#define I2C_MASTER_TIMEOUT_MS       1000

// AHT21 Konfiguracija senzorja
#define AHT21_I2C_ADDR              0x38 // data sheet AHT21  
#define AHT21_CMD_INIT              0xBE // data sheet AHT21
#define AHT21_CMD_TRIGGER           0xAC // data sheet AHT21
#define AHT21_CMD_SOFT_RESET        0xBA //data sheet AH21 , posebni ukaz I2C, ki resetira napravo, brez ponovnega vklopa in izklopa
#define AHT21_MEASUREMENT_DELAY_MS  80      // Time to wait for measurement

// I2C Master Handle
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t aht21_dev_handle = NULL;
static bool initialized = false;

/*
Inicializacija  I2C master bus
 */
static esp_err_t init_i2c_bus(void) //napravimo inicializacijsko funkcijo 
{
    i2c_master_bus_config_t bus_config = {  //konfiguracija master bus
         .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,//omogoči notranji pullup 
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle); //inicializacija master bus
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri kreiranju I2C master bus: %s", esp_err_to_name(ret));
        return ret; //ret je return 
    }

    ESP_LOGI(TAG, "I2C master bus uspesno kreiran na portu 0 (SDA=GPIO%d, SCL=GPIO%d)", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

/*
 Dodajanje senzorja AHT21 na I2C bus
*/
static esp_err_t add_aht21_device(void)//napravimo funkcijo s katero bomo dodali napravo 
{
    i2c_device_config_t dev_config = { //konfiguracija
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, //iz data sheet AHT21
        .device_address = AHT21_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &aht21_dev_handle);//inicializacija AHT21
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri prikljucevanju AHT21 na bus: %s", esp_err_to_name(ret));
        return ret;//ret je return 
    }

    ESP_LOGI(TAG, "AHT21 senzor dodan na bus na adresi 0x%02X", AHT21_I2C_ADDR);
    return ESP_OK;
}

/*
Sedaj, ko je senzor na vodilu mu lahko pošljemo vrednosti, ki jih bo prebral, se resetiral in ponastavil. 
Iz data sheets izhaja , da je senzor sposoben sprejemat sporočila od masterja v 100-500 ms 
po priključitvi na napajanje 2.V-5.5V
*/
static esp_err_t aht21_soft_reset(void) //napravimo funkcijo za pošiljanje in resetiranje 
{
    uint8_t cmd = AHT21_CMD_SOFT_RESET;//mehki reset za ponastavitev
    esp_err_t ret = i2c_master_transmit(aht21_dev_handle, &cmd, 1, I2C_MASTER_TIMEOUT_MS); //write ukaz
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri posiljanju reset vrednosti: %s", esp_err_to_name(ret));
        return ret;//ret je return 
    }
    
    vTaskDelay(pdMS_TO_TICKS(20));  // Malo počaklaj , da bo reset končan.
    ESP_LOGI(TAG, "AHT21 soft reset zakljucen");
    return ESP_OK;
}

/*
Pošljimo senzorju AHT21 še paket z vrednostmi za inicializacijo senzorja. Podatki  
so navedeni v data sheet. 0xBE, 0x08, 0x00  
*/
static esp_err_t aht21_init(void) //napravimo še funkcijo za pošiljanje inicializacijskih podatkov v napravo
{
    // Send initialization command: 0xBE, 0x08, 0x00
    uint8_t init_cmd[3] = {AHT21_CMD_INIT, 0x08, 0x00};
    esp_err_t ret = i2c_master_transmit(aht21_dev_handle, init_cmd, 3, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Inicializacija AHT21 ni uspela: %s", esp_err_to_name(ret));
        return ret;//ret je return 
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));  // Počakaj na inicializacijo
    ESP_LOGI(TAG, "AHT21 inicializacija uspesna");
    return ESP_OK;
}

/*
Sprožimo(trigger) merjenje (measurement) in branje data iz AHT21 tako , da mu pošljemo
vrednosti iz data sheet  0xAC, 0x33, 0x00
*/
static esp_err_t aht21_read_raw(uint8_t *data, size_t len)
{
    // Trigger measurement: 0xAC, 0x33, 0x00  
    uint8_t trigger_cmd[3] = {AHT21_CMD_TRIGGER, 0x33, 0x00};
    esp_err_t ret = i2c_master_transmit(aht21_dev_handle, trigger_cmd, 3, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri prozenju merjenj: %s", esp_err_to_name(ret));
        return ret;
    }

    // Počakamo , da se merjenje zaključi
    vTaskDelay(pdMS_TO_TICKS(AHT21_MEASUREMENT_DELAY_MS));

    // Read 7 bytes of data
    ret = i2c_master_receive(aht21_dev_handle, data, len, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Napaka pri branju measurement data: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/*
Konvertiranje raw(surovih) data v temperaturo in vlago
*/
static void aht21_convert_data(uint8_t *raw_data, sensor_data_t *data)
{
    // Preveri ali je senzor zaseden (bit 7 od statusnega byte)
    if (raw_data[0] & 0x80) {
        ESP_LOGW(TAG, "Senzor je  zaseden, meritve so nezanesljive ");
        data->valid = false;
        return;
    }

    // Extract vlage (20 bits: bits 12-31 od data)
    uint32_t humidity_raw = ((uint32_t)raw_data[1] << 12) | 
                            ((uint32_t)raw_data[2] << 4) | 
                            ((uint32_t)raw_data[3] >> 4);
    
    // Extract temperature (20 bits: bits 32-51 od data)
    uint32_t temperature_raw = (((uint32_t)raw_data[3] & 0x0F) << 16) | 
                               ((uint32_t)raw_data[4] << 8) | 
                               (uint32_t)raw_data[5];

    // Konvertiraj aktualne vrednosti
    data->humidity = ((float)humidity_raw / 1048576.0f) * 100.0f;
    data->temperature = ((float)temperature_raw / 1048576.0f) * 200.0f - 51.30f;//tovarniško je 200.0f-50.0f 
    data->valid = true;

    ESP_LOGD(TAG, "Temperatura: %.2f°C, Vlaga: %.2f%%", 
             data->temperature, data->humidity);
}

// -----------------------------------------Objavi API Implementacijo--------------------------------------------------------

//INICIALIZACIJA

esp_err_t sensor_manager_init(void)
{
    if (initialized) {
        ESP_LOGW(TAG, "Senzor je ze inicializiran");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Inicializiram sensor manager za AHT21...");

    // Inicializacija I2C bus
    esp_err_t ret = init_i2c_bus();
    if (ret != ESP_OK) {
        return ret;
    }

    // Dodajanje AHT21 senzorja
    ret = add_aht21_device();
    if (ret != ESP_OK) {
        i2c_del_master_bus(i2c_bus_handle);
        i2c_bus_handle = NULL;
        return ret;
    }

    // Soft reset AHT21
    ret = aht21_soft_reset();
    if (ret != ESP_OK) {
        i2c_master_bus_rm_device(aht21_dev_handle);
        i2c_del_master_bus(i2c_bus_handle);
        aht21_dev_handle = NULL;
        i2c_bus_handle = NULL;
        return ret;
    }

    // Inicializacija AHT21
    ret = aht21_init();
    if (ret != ESP_OK) {
        i2c_master_bus_rm_device(aht21_dev_handle);
        i2c_del_master_bus(i2c_bus_handle);
        aht21_dev_handle = NULL;
        i2c_bus_handle = NULL;
        return ret;
    }

    initialized = true;
    ESP_LOGI(TAG, "Sensor manager je uspesno inicializiran");
    return ESP_OK;
}
// BRANJE IN MERJENJE 
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

    // Inicializacija data strukture 
    memset(data, 0, sizeof(sensor_data_t));
    data->valid = false;

    // Read raw data (7 bytes)
    uint8_t raw_data[7];
    esp_err_t ret = aht21_read_raw(raw_data, sizeof(raw_data));
    if (ret != ESP_OK) {
        return ret;
    }

    // Konvertiraj raw data v temperaturo in vlago
    aht21_convert_data(raw_data, data);

    if (!data->valid) {
        ESP_LOGW(TAG, "Sensor data not valid");
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

// ODMONTIRANJE 

esp_err_t sensor_manager_deinit(void)  
{
    if (!initialized) {
        ESP_LOGW(TAG, "Sensor manager not initialized");
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    // Remove device
    if (aht21_dev_handle != NULL) {
        ret = i2c_master_bus_rm_device(aht21_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remove AHT21 device: %s", esp_err_to_name(ret));
        }
        aht21_dev_handle = NULL;
    }

    // Delete bus
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

//UGOTOVITEV

bool sensor_manager_is_initialized(void)
{
    return initialized;
}
