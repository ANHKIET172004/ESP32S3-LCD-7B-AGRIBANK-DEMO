#ifndef LIST_HANDLER_H
#define LIST_HANDLER_H

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

void parse_json_and_store(const char *json_data);

///////// mqtt
void save_device_list_to_nvs(void);
esp_err_t load_device_list_from_nvs(void);
esp_err_t load_selected_device_id(char *id_buf, size_t buf_size);



#endif