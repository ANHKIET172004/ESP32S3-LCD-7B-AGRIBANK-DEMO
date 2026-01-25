#ifndef INIT_HANDLE_H
#define INIT_HANDLE_H

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"


esp_err_t save_retry(uint8_t x);
uint8_t read_retry(uint8_t x);
void reset_retry(uint8_t x);
void init_fail_hanlde(uint8_t x);

#endif