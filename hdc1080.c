/*
 * ESP32 HDC1080 COMPONENT DRIVER LIBRARY
 * Copyright 2023 Open grStat
 *
 * SPDX-FileCopyrightText: 2023 Open grStat https://github.com/grstat
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: Created by Adrian Borchardt
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <string.h>
#include <esp_timer.h>
#include <driver/i2c.h>
#include "hdc1080.h"

static esp_err_t read_hdc100_data(unsigned char i2c_register, unsigned char * read_buff, size_t read_len);
static esp_err_t write_hdc100_data(unsigned char i2c_register, unsigned char * write_buff, size_t write_len);
static esp_err_t check_hdc1080_error(esp_err_t hdc_err);
static void hdc1080_conversion_completed(void* arg);
static hdc1080_settings_t hdc1080_set = {0};
static esp_timer_handle_t hdc1080_conversion_timer_h;
static bool awaiting_conversion = false;

/* -------------------------------------------------------------
 * @name void hdc1080_conversion_completed(void* arg)
 * -------------------------------------------------------------
 * @brief callback from the conversion timer. Gets the current
 * sensor readings and callsback to the original request set
 * during configuration
 */
static void hdc1080_conversion_completed(void* arg){
  hdc1080_sensor_readings_t sens_readings = {0};
  unsigned char read_buff[4];
  // READ IN THE DATA
  esp_err_t err_ck = check_hdc1080_error(i2c_master_read_from_device(hdc1080_set.i2c_port_number, hdc1080_set.i2c_address, (unsigned char *)read_buff, sizeof(read_buff), hdc1080_set.timeout_length));
  if(err_ck == ESP_OK){
    // IF NO ERROR OCCURED THEN DO THE FLOAT CONVERSION 
    // OTHERWISE 0 WILL BE RETURNED FOR BOTH VALUES TO SIGNAL AND ISSUE
    sens_readings.temperature = ((((float)((read_buff[0] << 8) | read_buff[1])/65536) * 165) - 40);   /* pow(2, 16) ==  65536 */
    sens_readings.humidity = (((float)((read_buff[2] << 8) | read_buff[3])/65536) * 100);
  }
  awaiting_conversion = false;  // MARK THE FINISHED STATE
  hdc1080_set.callback(sens_readings);  // RUN THE CONFIGURED CALLBACK
}

/* -------------------------------------------------------------
 * @name esp_err_t hdc1080_request_readings(void)
 * -------------------------------------------------------------
 * @brief begin the read request for sensor data.
 * Sets the register to kickoff the conversion and starts a timer.
 * When the conversion timer completes it calls back to
 * hdc1080_conversion_completed
 * @returns ESP_OK on success
 */
esp_err_t hdc1080_request_readings(void){
  if(awaiting_conversion){ return HDC1080_CONVERTING; }
  /* HDC1080 -> START CONVERSION -> WAIT FOR CONVERSION -> READ SENSOR DATA */
  ESP_LOGD("HDC1080", "STARTING CONVERSION");
  i2c_cmd_handle_t cmdlnk = i2c_cmd_link_create();
  check_hdc1080_error(i2c_master_start(cmdlnk));
  check_hdc1080_error(i2c_master_write_byte(cmdlnk, (hdc1080_set.i2c_address << 1) | I2C_MASTER_WRITE, true));
  check_hdc1080_error(i2c_master_write_byte(cmdlnk, HDC1080_TEMPERATURE_REG, true));
  check_hdc1080_error(i2c_master_stop(cmdlnk));
  esp_err_t err_ck = check_hdc1080_error(i2c_master_cmd_begin(hdc1080_set.i2c_port_number, cmdlnk, hdc1080_set.timeout_length));
  i2c_cmd_link_delete(cmdlnk);
  if(err_ck != ESP_OK){ return err_ck; }
  awaiting_conversion = true;
  /* START CONVERSION WAIT TIMER */
  err_ck = esp_timer_start_once(hdc1080_conversion_timer_h, HDC1080_CONVERSION_WAIT_PERIOD);
  return err_ck;
}

/* --------------------------------------------------------------------------------------------------
 * @name esp_err_t hdc1080_configure(hdc1080_settings_t * hdc1080_settings, hdc1080_config_t hdc_cfg)
 * --------------------------------------------------------------------------------------------------
 * @brief Set HDC1080 config registers
 * @param hdc1080_settings -> Pointer to the hdc1080_settings_t struct
 * @param hdc_cfg - Device configuration from the hdc1080_config_t struct
 * @return ESP_OK on success* 
 * @note The i2c bus must be setup and configured before
 *       calling this routine
 */
esp_err_t hdc1080_configure(hdc1080_settings_t * hdc1080_settings, hdc1080_config_t hdc_cfg){
  if(awaiting_conversion){ return HDC1080_CONVERTING; }
  unsigned char hdc_buff[2] = {0};
  unsigned short cfg_s = 0;
  cfg_s = (hdc_cfg.config_register << 8);
  // CAPTURE THE SETTINGS TO THE FILE GLOBAL SCOPE
  memmove(&hdc1080_set, hdc1080_settings, sizeof(hdc1080_settings_t));
  // GET MANUFACTURER ID AND ENSURE A MATCH
  esp_err_t err_ck = check_hdc1080_error(read_hdc100_data(HDC1080_MANUFACTURER_ID_REG, hdc_buff, 2));
  if(err_ck != ESP_OK){ return err_ck; }
  if((unsigned short)((hdc_buff[0] << 8) | hdc_buff[1]) != HDC1080_MANUFACTURER_ID){
    // NOT A TI CHIP
    ESP_LOGE("HDC1080", "EXPECTED TI ID 0x%04X BUT GOT 0x%04X", HDC1080_MANUFACTURER_ID, (unsigned short)((hdc_buff[0] << 8) | hdc_buff[1]));
    return HDC1080_ERR_ID;
  }
  // GET THE DEVICE ID AND MAKE SURE IT'S AN HDC1080
  err_ck = check_hdc1080_error(read_hdc100_data(HDC1080_DEVICE_ID_REG, hdc_buff, 2));
  if(err_ck != ESP_OK){ return err_ck; }
  if((unsigned short)((hdc_buff[0] << 8) | hdc_buff[1]) != HDC1080_DEVICE_ID){
    // NOT AND HDC1080
    ESP_LOGE("HDC1080", "EXPECTED DEVICE ID 0x%04X BUT GOT 0x%04X", HDC1080_DEVICE_ID, (unsigned short)((hdc_buff[0] << 8) | hdc_buff[1]));
    return HDC1080_ERR_ID;  
  }
  // GET THE CURRENT CONFIGURATION AND IF IT DOESN'T MATCH, UPDATE IT
  err_ck = check_hdc1080_error(read_hdc100_data(HDC1080_CONFIG_REG, hdc_buff, 2));
  if(err_ck != ESP_OK){ return err_ck; }
  ESP_LOGD("HDC1080", "CURRENT CONFIGURATION 0x%04X", (unsigned short)((hdc_buff[0] << 8) | hdc_buff[1]));
  if((unsigned short)((hdc_buff[0] << 8) | hdc_buff[1]) != cfg_s){
    ESP_LOGD("HDC1080", "UPDATING CONFIGURATION FROM 0x%04X TO 0x%04X", (unsigned short)((hdc_buff[0] << 8) | hdc_buff[1]), cfg_s);
    hdc_buff[0] = hdc_cfg.config_register;
    hdc_buff[1] = 0;
    err_ck = check_hdc1080_error(write_hdc100_data(HDC1080_CONFIG_REG, hdc_buff, 2));
    if(err_ck != ESP_OK){ return err_ck; }
  }
  /* HDC1080 REQUIRES A SHORT DELAY TO PERFORM CONVERSION
   * BEFORE SENSOR DATA CAN BE READ. THE TIMER BELOW IS USED
   * WHEN TEMP READINGS ARE REQUESTED */
  const esp_timer_create_args_t hdc1080_conversion_timer_args = {
    .callback = &hdc1080_conversion_completed,
    .name = "hdc1080_conversion_timer"
  };
  /* CREATE THE TEMPERATURE TRIGGER TIMER */
  err_ck = esp_timer_create(&hdc1080_conversion_timer_args, &hdc1080_conversion_timer_h);
  return err_ck;
}

/* ----------------------------------------------------------------------
 * @name esp_err_t hdc1080_get_configuration(hdc1080_config_t * hdc_cfg)
 * ----------------------------------------------------------------------
 * @brief Write to the i2c bus
 * @param hdc_cfg -> pointer to an hdc1080_config_t to fill
 * @return ESP_OK on success* 
 */
esp_err_t hdc1080_get_configuration(hdc1080_config_t * hdc_cfg){
  if(awaiting_conversion){ return HDC1080_CONVERTING; }
  unsigned char hdc_buff[2] = {0};
  esp_err_t err_ck = check_hdc1080_error(read_hdc100_data(HDC1080_CONFIG_REG, hdc_buff, 2));
  if(err_ck != ESP_OK){ return err_ck; }
  hdc_cfg->config_register = hdc_buff[0];
  return err_ck;
}

/* --------------------------------------------------------------------------------------------------
 * @name static esp_err_t write_hdc100_data(unsigned char i2c_register, unsigned char * write_buff, size_t write_len)
 * --------------------------------------------------------------------------------------------------
 * @brief Write to the i2c bus
 * @param i2c_register -> register to write to
 * @param write_buff -> pointer to the buffer with the data
 * @param write_len -> length of the write
 * @return ESP_OK on success* 
 */
static esp_err_t write_hdc100_data(unsigned char i2c_register, unsigned char * write_buff, size_t write_len){
  /* CHANGE TO THE CORRECT REGISTER BEFORE THE READ */
  i2c_cmd_handle_t cmdlnk = i2c_cmd_link_create();
  check_hdc1080_error(i2c_master_start(cmdlnk));
  check_hdc1080_error(i2c_master_write_byte(cmdlnk, (hdc1080_set.i2c_address << 1) | I2C_MASTER_WRITE, true));
  check_hdc1080_error(i2c_master_write_byte(cmdlnk, i2c_register, true));
  check_hdc1080_error(i2c_master_write(cmdlnk, write_buff, write_len, I2C_MASTER_LAST_NACK));
  check_hdc1080_error(i2c_master_stop(cmdlnk));
  esp_err_t err_ck = check_hdc1080_error(i2c_master_cmd_begin(hdc1080_set.i2c_port_number, cmdlnk, hdc1080_set.timeout_length));
  i2c_cmd_link_delete(cmdlnk);
  return err_ck;
}

/* --------------------------------------------------------------------------------------------------
 * @name static esp_err_t read_hdc100_data(unsigned char i2c_register, unsigned char * read_buff, size_t read_len)
 * --------------------------------------------------------------------------------------------------
 * @brief Read from the i2c bus
 * @param i2c_register -> register to read from
 * @param read_buff -> pointer to the buffer where the data will be stored
 * @param read_len -> length of the read
 * @return ESP_OK on success* 
 */
static esp_err_t read_hdc100_data(unsigned char i2c_register, unsigned char * read_buff, size_t read_len){
  /* CHANGE TO THE CORRECT REGISTER BEFORE THE READ */
  i2c_cmd_handle_t cmdlnk = i2c_cmd_link_create();
  check_hdc1080_error(i2c_master_start(cmdlnk));
  check_hdc1080_error(i2c_master_write_byte(cmdlnk, (hdc1080_set.i2c_address << 1) | I2C_MASTER_WRITE, true));
  check_hdc1080_error(i2c_master_write_byte(cmdlnk, i2c_register, true));
  check_hdc1080_error(i2c_master_stop(cmdlnk));
  esp_err_t err_ck = check_hdc1080_error(i2c_master_cmd_begin(hdc1080_set.i2c_port_number, cmdlnk, hdc1080_set.timeout_length));
  i2c_cmd_link_delete(cmdlnk);
  if(err_ck != ESP_OK){ return err_ck; }
  /* BEGIN THE READ */
  cmdlnk = i2c_cmd_link_create();
  check_hdc1080_error(i2c_master_start(cmdlnk));
  check_hdc1080_error(i2c_master_write_byte(cmdlnk, (hdc1080_set.i2c_address << 1) | I2C_MASTER_READ, true));
  check_hdc1080_error(i2c_master_read(cmdlnk, read_buff, read_len, I2C_MASTER_LAST_NACK));
  check_hdc1080_error(i2c_master_stop(cmdlnk));
  err_ck = check_hdc1080_error(i2c_master_cmd_begin(hdc1080_set.i2c_port_number, cmdlnk, hdc1080_set.timeout_length));
  i2c_cmd_link_delete(cmdlnk);
  return err_ck;
}

/* --------------------------------------------------------------
 * @name static esp_err_t check_hdc1080_error(esp_err_t hdc_err)
 * --------------------------------------------------------------
 * @brief Check for esp errors and print them
 * @param hdc_err -> The returned error from the check
 * @return ESP_OK on success, original error on fail
 * @note Any special error handling can be put in here
 */
static esp_err_t check_hdc1080_error(esp_err_t hdc_err){
  if(hdc_err == ESP_OK){ return ESP_OK; }
  ESP_LOGE("HDC1080", "ERROR HAS OCCURED: %s", esp_err_to_name(hdc_err));
  return hdc_err;
}