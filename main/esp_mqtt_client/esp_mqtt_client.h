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
#include "action.h"//
#include "time_check.h"//
#include "list_handler.h"//

#define MQTT_MAX_TOPIC_LEN 64
#define MQTT_MAX_PAYLOAD_LEN 1024

typedef struct {
    char topic[MQTT_MAX_TOPIC_LEN];
    char payload[MQTT_MAX_PAYLOAD_LEN];
} mqtt_mess_t;

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_start(void);

void save_login_status(const char *status);

void save_last_id(uint16_t value);
uint16_t read_last_id();

#endif