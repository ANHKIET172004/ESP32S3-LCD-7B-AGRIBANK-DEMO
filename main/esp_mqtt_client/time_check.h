#ifndef TIME_CHECK_H
#define TIME_CHECK_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "mqtt_data.h"

void save_timeout(const uint8_t time);

esp_err_t read_time(uint8_t* time);

#endif