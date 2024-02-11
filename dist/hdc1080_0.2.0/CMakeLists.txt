set(depends esp_timer esp_system i2c esp_log esp_event esp_err math)

idf_component_register(
    SRCS "hdc1080.c"
    INCLUDE_DIRS "include"
    REQUIRES ${depends})
