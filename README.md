# ESP32 HDC1080 LIBRARY
This is an HDC1080 Hardware abstraction library that has been created for Open grStat using esp-idf.
Feel free to use it in your projects, esp32-hdc1080.c provides you with an example on how to setup and use the library

# Requirements

- An ESP32 Dev Kit
- An HDC1080 i2c Sensor from Texas Instruments
- esp-idf and tools -> the vscode Espressif IDF extension makes it easy

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