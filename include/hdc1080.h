/*
 * ESP32 HDC1080 COMPONENT DRIVER LIBRARY
 * Copyright 2023 Open grStat
 *
 * SPDX-FileCopyrightText: 2023 Open grStat https://github.com/grstat
 * SPDX-FileType: HEADER
 * SPDX-FileContributor: Created by Adrian Borchardt
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef __HDC1080_H__
#define __HDC1080_H__
#include <esp_err.h>
#include <esp_log.h>
#include <esp_event.h>
#include <math.h>

#define HDC1080_TEMPERATURE_REG     0x00    /* TEMPERATURE MEASUREMENT OUTPUT */
#define HDC1080_HUMIDITY_REG        0x01    /* RELATIVE HUMIDITY MEASUREMENT OUTPUT */
#define HDC1080_CONFIG_REG          0x02    /* HDC1080 CONFIGURATION DATA */
#define HDC1080_SERIALID2_REG       0xFB    /* FIRST 2 BYTES OF SERIAL ID */
#define HDC1080_SERIALID1_REG       0xFC    /* MID 2 BYTES OF THE SERIAL ID */
#define HDC1080_SERIALID0_REG       0xFD    /* LAST BYTE BIT OF THE SERIAL ID */
#define HDC1080_MANUFACTURER_ID_REG 0xFE    /* ID OF TEXAS INSTRUMENTS */
#define HDC1080_DEVICE_ID_REG       0xFF    /* REGISTER OF THE DEVICE ID */
#define HDC1080_DEVICE_ID           0x1050  /* HDC1080 UNIQUE ID */
#define HDC1080_MANUFACTURER_ID     0x5449  /* TI MANUFACTURER ID */
#define HDC1080_I2C_ADDRESS         0x40    /* I2C ADDRESS OF THE HDC1080 */

#define HDC1080_ACQUISITION_HUMIDITY_AND_TEMPERATURE  0x01
#define HDC1080_ACQUISITION_HUMIDITY_OR_TEMPERATURE   0x00
#define HDC1080_TEMPERATURE_RESOLUTION_11BIT          0x01
#define HDC1080_TEMPERATURE_RESOLUTION_14BIT          0x00
#define HDC1080_HUMIDITY_RESOLUTION_8BIT              0x02
#define HDC1080_HUMIDITY_RESOLUTION_11BIT             0x01
#define HDC1080_HUMIDITY_RESOLUTION_14BIT             0x00

#define HDC1080_HEATER_ENABLED      0x01
#define HDC1080_HEATER_DISABLED     0x00
#define HDC1080_BATTERY_STATUS_OK   0x00
#define HDC1080_BATTERY_STATUS_LOW  0x01
#define HDC1080_ERR_ID              0xFF
#define HDC1080_CONVERTING          0xFE
#define HDC1080_CONVERSION_WAIT_PERIOD   (500000) /* CONVERSION WAIT PERIOD */

/* CONVERT CELSIUS TO FAHRENHEIT */
#define CEL2FAH(CELSIUS) ((1.8 * CELSIUS) + 32)
/* CALCULATE DEWPOINT USING TEMPERATURE AND HUMIDITY */
#define DEWPOINT(CELSIUS, RH) (CELSIUS - (14.55 + 0.114 * CELSIUS) * \
         (1 - (0.01 * RH)) - pow(((2.5 + 0.007 * CELSIUS) * \
         (1 - (0.01 * RH))),3) - (15.9 + 0.117 * CELSIUS) * \
         pow((1 - (0.01 * RH)), 14))
/* CALCULATE AIR SATURATION VAPOR PRESSURE IN PASCALS */
#define SVP(CELSIUS) (610.78 * pow(2.71828, (CELSIUS / (CELSIUS+237.3) * 17.2694)))
/* AIR VAPOR PRESSURE DEFICIT IN kPa */
#define VPD(SVP, RH) ((SVP *(1 - RH/100))/1000)
/* CONVERT PASCALS TO kPa */
#define PAS2KPA(PASCALS) (PASCALS / 1000)

/* THE CONFIG REGISTER IS 16 BITS LONG BUT THE FIRST 8 
 * BITS ARE RESERVED, SO HERE JUST PACK IN THE LAST 8 BITS */
typedef union {
	unsigned char config_register;
	struct {
		unsigned char humidity_measurement_resolution : 2;
		unsigned char temperature_measurement_resolution : 1;
		unsigned char battery_status : 1;
		unsigned char mode_of_acquisition : 1;
		unsigned char heater : 1;
		unsigned char reserved_bit : 1;
		unsigned char software_reset : 1;
	};
} hdc1080_config_t;

/* SENSOR READINGS STRUCT VALUES IN CELSIUS */
typedef struct HDC1080_SENSOR_READINGS {
  float humidity;
  float temperature;
} hdc1080_sensor_readings_t;

/* CALLBACK FOR SENSOR READINGS */
typedef void(* hdc1080_sensor_callback)(hdc1080_sensor_readings_t);

/* PORT AND CALLBACK SETTINGS
 * i2c_address -> HDC1080 i2c ADDRESS
 * i2c_port_number -> THE CONFIGURED i2c PORT
 * timeout_length -> THE LENGTH TO WAIT FOR A READ/WRITE TIMEOUT
 * callback -> THE CALLBACK FUNCTION TO RETURN THE SENSOR DATA TO
 *             EXP: void temperature_readings_callback(hdc1080_sensor_readings_t sens_readings)
 */
typedef struct HDC1080_SETTINGS {
  unsigned char i2c_address;
  unsigned char i2c_port_number;
  TickType_t timeout_length;
  hdc1080_sensor_callback callback;
} hdc1080_settings_t;

esp_err_t hdc1080_configure(hdc1080_settings_t * hdc1080_settings, hdc1080_config_t hdc_cfg);
esp_err_t hdc1080_request_readings(void);
esp_err_t hdc1080_get_configuration(hdc1080_config_t * hdc_cfg);

#endif