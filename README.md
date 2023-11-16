# ESP32 HDC1080 HAL LIBRARY
This is an HDC1080 Hardware abstraction library that has been created for Open grStat using esp-idf.
The HDC1080 is a temperature and humidity sensor made by Texas Instruments.

The datasheet and other information can be found here: https://www.ti.com/product/HDC1080

This code should give a good understanding of how to access, configure and read the sensor data.
Please feel free to leave a comment/request a feature or report any bugs you may run into.

# Requirements

- An ESP32 Dev Kit
- An HDC1080 i2c Sensor from Texas Instruments
- esp-idf and tools

Set your variables at the top of esp32-hdc1080.c

## Standard esp-idf configuration

### From the command line
Open the project configuration menu (`idf.py menuconfig`)
### Using vscode espressif IDF extension
Command pallet -> `ESP-IDF: SDK Configuration editor`

### From the command line
Use `idf.py -p PORT flash monitor` to build and flash to the device
### Using vscode espressif IDF extension
Build the project
Command pallet -> `ESP-IDF: Build your project`
Flash it
Command pallet -> `ESP-IDF: Flash your project`
  Choose UART or JTAG depenting on your setup
  Choose the COM_PORT to use
Monitor the device
Command pallet -> `ESP-IDF: Monitor your device`