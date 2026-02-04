# Termostat Box 3

A thermostat application for the ESP32-BOX-3 development board that displays temperature and humidity readings from an AHT21 sensor on an LCD screen.

## Features

- Real-time temperature and humidity display
- AHT21 sensor integration via I2C
- LVGL-based user interface
- ESP32-BOX-3 hardware support

## Hardware Requirements

- ESP32-BOX-3 development board
- AHT21 temperature and humidity sensor connected to:
  - SCL: GPIO40
  - SDA: GPIO41

## Software Requirements

- ESP-IDF framework
- LVGL graphics library
- ESP-BSP (Board Support Package)

## Building and Flashing

1. Set up ESP-IDF environment
2. Navigate to the project directory
3. Configure the project: `idf.py menuconfig`
4. Build the project: `idf.py build`
5. Flash to device: `idf.py flash`
6. Monitor output: `idf.py monitor`

## Usage

The application initializes the display and sensor, then continuously reads and displays temperature and humidity every 2 seconds. If sensor initialization fails, it displays an error message on the screen.

## Components

- `display_manager`: Handles LCD display initialization and brightness control
- `sensor_manager`: Manages AHT21 sensor communication and data reading

## License

[Add license information here]