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

bool device_list_is_different(device_info_t *list1, int count1,
                              device_info_t *list2, int count2);
esp_err_t load_device_list_from_nvs_to_buffer(device_info_t *list, int *count);
esp_err_t parse_json_to_device_list(const char *json,
                                    device_info_t *list,
                                    int *count);
int device_compare_by_counter(const void *a, const void *b);
esp_err_t save_device_list_to_nvs_from_buffer(
        const device_info_t *list,
        int count);
 void build_new_list(device_info_t *new_list, int count);
#endif