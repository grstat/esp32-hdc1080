# ESP32 HDC1080 COMPONENT DRIVER LIBRARY EXAMPLE

The example code should give a good understanding of how to configure and use the hdc1080 component driver.

It includes how to use the aspects of the driver as well as the available macros for calculating and converting 
the data into various formats. Including: 
- Celsius -> Fahrenheit
- Temperature & humidity -> Dewpoint
- Temperature -> Air saturation vapor pressure
- Air saturation vapor pressure & humidity -> Air vapor pressure deficit

## Example workflow

- I2C is initalized, included is an I2C scan event that will display current devices on the bus for troubleshooting
- If the I2C initalized successfully the following occurs
- hdc1080_settings_t is filled with the I2C configuration and callback procedure
- hdc1080_config_t is filled with the defined register values
- The hdc1080 sensor is configured with the settings and configuration defined
- If the sensor is configured without error and inital sensor read is started; upon completion the hdc1080_settings_t.callback is called
- The callback procedure performs various conversions and prints the current sensor data.
- The current hdc1080 configuration is captured

## Pinout

| ESP32 | HDC1080 |
|-------|---------|
| 22    | SCL     |
| 21    | SDA     |
| 3.3v  | VCC     |
| GND   | GND     |

## Requirements

- An ESP32 Dev Kit
- An HDC1080 i2c Sensor module.
- esp-idf and tools (Developed and tested on v5.1.1)

### Project configuration

* Open the project configuration menu (`idf.py menuconfig`)
* Go to the HDC1080 Example config section
* Set the I2C SCL, SDA, PORT NUMBER & FREQUENCY if they are not the default
* If you do not wish to perform the I2C device scan you can disable the option 

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```
