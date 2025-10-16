#ifndef ESP_MQTT_CLIENT_H
#define ESP_MQTT_CLIENT_H

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






esp_err_t read_backup_message(nvs_handle_t nvs_handle, const char *key, char **topic, char **payload);

esp_err_t delete_backup_message(nvs_handle_t nvs_handle, const char *key);

uint32_t get_backup_count(nvs_handle_t nvs_handle);


void mqtt_retry_publish(void);

void client_mqtt_retry_publish(void);
void backup_mqtt_data(const char *topic, const char *payload);
void backup_client_mqtt_data(const char *topic, const char *payload);

void check_device_message(const char *json_string) ;



void mqtt_retry_publish_task(void *pvParameters);

void mqtt_retry_init(void);
void trigger_mqtt_retry(void);


void mqtt_retry_publish_task2(void *pvParameters);

void mqtt_retry_init2(void);
void trigger_mqtt_retry2(void);

void save_number(const char *number);
esp_err_t read_number(char *number, size_t max_len);


///////// mqtt


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);


void mqtt_start(void);




#endif