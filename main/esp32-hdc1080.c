#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_event.h>
#include <driver/i2c.h>
#include "hdc1080/hdc1080.h"

#define I2C_SCL                   (22)
#define I2C_SDA                   (21)
#define I2C_PORT_MASTER           I2C_NUM_0
#define I2C_MASTER_FREQ_HZ        (400000)    /* 400kHz */
#define I2C_MASTER_TX_BUF_DISABLE (0)
#define I2C_MASTER_RX_BUF_DISABLE (0)
#define I2C_READ_TIMEOUT_PERIOD   ((TickType_t)200 / portTICK_PERIOD_MS)

#define HDC1080_I2C_ADDRESS       0x40        /* I2C ADDRESS OF THE HDC1080 */

static bool i2c_init(void);

/* THIS IS THE CALLBACK FOR THE SENSOR READINGS,
 * THE HDC1080 REQUIRES A SHORT CONVERSION PERIOD
 * WHEN THE READINGS ARE REQUESTED. INSTEAD OF BLOCKING 
 * A TIMER IS STARTED WHEN THE CONVERSION IS FINISHED
 * THE VALUES ARE READ AND THEN RETURNED TO THIS CALLBACK 
 * ON COMPLETE. IF BOTH VALUES ARE 0 THEN AN ERROR MAY HAVE OCCURED */
void temperature_readings_callback(hdc1080_sensor_readings_t sens_readings){
  ESP_LOGI("SENS", "TEMP: %.2f C", sens_readings.temperature);
  ESP_LOGI("SENS", "HUMI: %.2f %%", sens_readings.humidity);
}

void app_main(void){
  // KEEP EVERYTHING GOING
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_create_default());
  // CONFIGURE YOUR I2C BUS
  if(i2c_init()){
    // FILL IN YOUR HDC SETTINGS
    hdc1080_settings_t hdc_settings = {
      .i2c_address = HDC1080_I2C_ADDRESS,
      .i2c_port_number = I2C_PORT_MASTER,
      .timeout_length = I2C_READ_TIMEOUT_PERIOD,
      .callback = temperature_readings_callback
    };
    
    // SETUP YOUR HDC REGISTER CONFIGURATION
    hdc1080_config_t hdc_config = {
      .humidity_measurement_resolution = HDC1080_HUMIDITY_RESOLUTION_14BIT,
      .temperature_measurement_resolution = HDC1080_TEMPERATURE_RESOLUTION_14BIT,
      .mode_of_acquisition = HDC1080_ACQUISITION_HUMIDITY_AND_TEMPERATURE,
      .heater = HDC1080_HEATER_DISABLED
    };

    // SETUP AND CONFIGURE THE SENSOR AND ABSTRACTION
    if(hdc1080_configure(&hdc_settings, hdc_config) == ESP_OK){
      ESP_LOGI("MAIN", "HDC1080 CONFIGURATION SUCCESSFUL");
      // DO A REQUEST FOR THE SENSOR READINGS 
      // THIS WILL CALLBACK TO void temperature_readings_callback(hdc1080_sensor_readings_t sens_readings)
      // AS SET IN THE hdc_settings
      if(hdc1080_request_readings() == ESP_OK){
        ESP_LOGI("MAIN", "READINGS WERE REQUESTED");
      }
    }

    // AS AN EXAMPLE, ANYTIME READINGS HAVE BEEN REQUESTED AND CONVERSION HAS STARTED
    // ANYTHING CALLED WILL RETURN IN AN ERROR HDC1080_CONVERTING, A WAIT AND CHECK
    // FUNCTION WILL BE REQURED BEFORE THE NEXT COMMAND CAN BE RUN
    // THE WAIT PERIOD CAN BE ADJUSTED IN THE hdc1080.h FILE, HDC1080_CONVERSION_WAIT_PERIOD
    // THE VALUE IS IN MICROSECONDS. MINIMUM WAIT TIME IS 6.8uS HOWEVER THE DEFAULT IS 1/2 SECOND
    // SINCE READS SHOULD ONLY OCCUR > ONCE A SECOND FOR STABILITY
    esp_err_t gcfg = hdc1080_get_configuration(&hdc_config);
    if(gcfg == HDC1080_CONVERTING){
      ESP_LOGE("MAIN", "REQUEST FAILED, CONVERSION IN PROGRESS");
    }
  }
}

/* ----------------------------------------------------------------------
 * @name bool i2c_init(void)
 * ----------------------------------------------------------------------
 * @brief Configure i2c parameters, install i2c driver, 
 * perform device discovery
 * 
 * @return true on success
 */
static bool i2c_init(void){
  unsigned char devAddr = 0;
  unsigned char devCount = 0;
  unsigned char devList[128] = {0};
  /* I2C MASTER MODE, PULLUPS ENABLED */
  i2c_config_t i2c_conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = I2C_SDA,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_io_num = I2C_SCL,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = I2C_MASTER_FREQ_HZ
  };
  /* CONFIGURE THE PORT */
  esp_err_t err = i2c_param_config(I2C_PORT_MASTER, &i2c_conf);
  if (err != ESP_OK) {
    ESP_LOGE("I2C", "ERROR CONFIGURING I2C PORT %d", err);
    return false;
  }
  /* LOAD THE DRIVER */
  err = i2c_driver_install(I2C_PORT_MASTER, i2c_conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
  if (err != ESP_OK) {
    if(err == ESP_ERR_INVALID_ARG){
      ESP_LOGE("I2C", "ERROR INSTALLING I2C DRIVER, INVALID ARGUMENT");
    }else if(err == ESP_FAIL){
      ESP_LOGE("I2C", "I2C DRIVER INSTALLATION FAILED!");
    }
    return false;
  }
  /* DEVICE DISCOVERY */
  for(int ol = 0; ol < 128; ol += 16){
    for(int il =0; il < 16; il++){
      devAddr = ol + il;  // CURRENT ADDRESS IS OUTER LOOP + INNER LOOP
      i2c_cmd_handle_t cmdlnk = i2c_cmd_link_create();
      i2c_master_start(cmdlnk);
      i2c_master_write_byte(cmdlnk, (devAddr << 1) | I2C_MASTER_WRITE, I2C_MASTER_NACK);
      i2c_master_stop(cmdlnk);
      esp_err_t lnkerr = i2c_master_cmd_begin(I2C_PORT_MASTER, cmdlnk, I2C_READ_TIMEOUT_PERIOD);
      i2c_cmd_link_delete(cmdlnk);
      switch(lnkerr){
        case ESP_OK:
          if(devCount < 128){ //DON'T OVERFLOW
            if(devAddr == 0x00){ break; } //IGNORE ADDRESS 0
            devList[devCount++] = devAddr;  //ADD THE DEVICE TO THE LIST AND INCREMENT THE COUNT
          }
        break;
        case ESP_ERR_INVALID_ARG:
          ESP_LOGE("i2c_discover", "INVALID PARAMETER WAS PASSED TO i2c_master_cmd_begin");
        break;
        case ESP_ERR_NO_MEM:
          ESP_LOGE("i2c_discover", "THE CMD HANDLER BUFFER SIZE IS TOO SMALL");
        break;
        default: break; //TIMED OUT, MOVE ON
      } /** END - switch(lnkerr) */
    } /** END - for(int il =0; il < 16; il++) */
  } /** END - for(int ol = 0; ol < 128; ol += 16) */

  if(devCount == 0){
    ESP_LOGW("I2C", "NO DEVICES FOUND");
    return false;
  }

  /* PRINT DISCOVERED DEVICE ADDRESSES */
  if(devCount > 0){
    for(int x=0; x<devCount; x++){
      ESP_LOGI("I2C", "FOUND DEVICE AT ADDRESS: 0x%02X", devList[x]);
    } /** END - for(int x=0; x<devCount; x++) */
  }
  return true;
}