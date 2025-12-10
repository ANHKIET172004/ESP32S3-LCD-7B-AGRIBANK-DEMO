#ifndef ACTION_H
#define ACTION_H

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

void save_number(const char *number);

void skip_number(const char* number);

void save_current_number(const char *number);

void save_next_number(const char *number);

esp_err_t read_number(char *number, size_t max_len);

esp_err_t read_next_number(char *number, size_t max_len);

esp_err_t delete_current_number(void);

esp_err_t delete_next_number(void);

void reset_recent_number(void);

esp_err_t transfer_number(void);

#endif