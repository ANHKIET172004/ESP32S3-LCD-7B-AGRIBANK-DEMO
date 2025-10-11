/*****************************************************************************
 * | File       :   main.c
 * | Author     :   Waveshare team
 * | Function   :   Main function
 * | Info       :
 * |                UI Design：
 *                          1. User Login and Creation: Users can log in or create new accounts, and the created users are saved to NVS, so data is not lost after power-down.
 *                          2. Wi-Fi: Can connect to Wi-Fi and start an access point (hotspot).
 *                          3. RS485: Can send and receive data, with data displayed on the screen.
 *                          4. PWM: Can modify PWM output in multiple ways to control screen brightness. Additionally, it can display information from a Micro SD card.
 *                          5. CAN: Can send and receive data, with data displayed on the screen.
 *----------------
 * | Version    :   V1.0
 * | Date       :   2025-05-08
 * | Info       :   Basic version
 *
 ******************************************************************************/
#include "rgb_lcd_port.h" // Header for Waveshare RGB LCD driver
#include "gt911.h"        // Header for touch screen operations (GT911)
#include "lvgl_port.h"    // Header for LVGL port initialization and locking
//#include "add_password.h" // Header for password handling logic
//#include "pwm.h"          // Header for PWM initialization (used for backlight control)
#include "wifi.h"         // Header for Wi-Fi functionality
//#include "sd_card.h"      // Header for SD card operations
//#include "D:/ESP32/ESP32S3_LCD_7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/ui/ui.h"           // Header for user interface initialization
#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/ui/ui.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"

// các file header hỗ trợ gatt

#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/gatt_server/gatt_server.h"
//#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/gatt_client/gatt_client.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "freertos/event_groups.h"



static const char *TAG = "main"; // Tag used for ESP log output
static const char *MQTT_TAG ="MQTT";

static esp_lcd_panel_handle_t panel_handle = NULL; // Handle for the LCD panel
static esp_lcd_touch_handle_t tp_handle = NULL;    // Handle for the touch panel

extern int rate;
extern int score;

extern int mesh_enb;

 extern EventGroupHandle_t wifi_event_group;
extern int WIFI_CONNECTED_BIT ;


esp_mqtt_client_handle_t mqttClient;

int mqtt=0;

int connect_success=0;


uint8_t key_id=0;

extern lv_obj_t * ui_Image37;
extern lv_obj_t * ui_Image36;
extern lv_obj_t * ui_Image38;



#include "esp_sleep.h"

 //LV_USE_PERF_MONITOR 


//extern const uint8_t hivemq_root_ca_pem_start[] asm("_binary_hivemq_root_ca_pem_start");
//extern const uint8_t hivemq_root_ca_pem_end[]   asm("_binary_hivemq_root_ca_pem_end");

// Khai báo để linker nhúng dữ liệu PEM vào firmware
//extern const uint8_t trustid_x3_root_pem_start[] asm("_binary_trustid_x3_root_pem_start");
//extern const uint8_t trustid_x3_root_pem_end[]   asm("_binary_trustid_x3_root_pem_end");

//extern const uint8_t isrgrootx1_pem_start[] asm("_binary_isrgrootx1_pem_start");
//extern const uint8_t isrgrootx1_pem_end[]   asm("_binary_isrgrootx1_pem_end");


const char mqtt_ca_cert_pem[] = "-----BEGIN CERTIFICATE-----\n"
"MIIFBTCCAu2gAwIBAgIQWgDyEtjUtIDzkkFX6imDBTANBgkqhkiG9w0BAQsFADBP\n"
"MQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFy\n"
"Y2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMTAeFw0yNDAzMTMwMDAwMDBa\n"
"Fw0yNzAzMTIyMzU5NTlaMDMxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBF\n"
"bmNyeXB0MQwwCgYDVQQDEwNSMTMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
"AoIBAQClZ3CN0FaBZBUXYc25BtStGZCMJlA3mBZjklTb2cyEBZPs0+wIG6BgUUNI\n"
"fSvHSJaetC3ancgnO1ehn6vw1g7UDjDKb5ux0daknTI+WE41b0VYaHEX/D7YXYKg\n"
"L7JRbLAaXbhZzjVlyIuhrxA3/+OcXcJJFzT/jCuLjfC8cSyTDB0FxLrHzarJXnzR\n"
"yQH3nAP2/Apd9Np75tt2QnDr9E0i2gB3b9bJXxf92nUupVcM9upctuBzpWjPoXTi\n"
"dYJ+EJ/B9aLrAek4sQpEzNPCifVJNYIKNLMc6YjCR06CDgo28EdPivEpBHXazeGa\n"
"XP9enZiVuppD0EqiFwUBBDDTMrOPAgMBAAGjgfgwgfUwDgYDVR0PAQH/BAQDAgGG\n"
"MB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcDATASBgNVHRMBAf8ECDAGAQH/\n"
"AgEAMB0GA1UdDgQWBBTnq58PLDOgU9NeT3jIsoQOO9aSMzAfBgNVHSMEGDAWgBR5\n"
"tFnme7bl5AFzgAiIyBpY9umbbjAyBggrBgEFBQcBAQQmMCQwIgYIKwYBBQUHMAKG\n"
"Fmh0dHA6Ly94MS5pLmxlbmNyLm9yZy8wEwYDVR0gBAwwCjAIBgZngQwBAgEwJwYD\n"
"VR0fBCAwHjAcoBqgGIYWaHR0cDovL3gxLmMubGVuY3Iub3JnLzANBgkqhkiG9w0B\n"
"AQsFAAOCAgEAUTdYUqEimzW7TbrOypLqCfL7VOwYf/Q79OH5cHLCZeggfQhDconl\n"
"k7Kgh8b0vi+/XuWu7CN8n/UPeg1vo3G+taXirrytthQinAHGwc/UdbOygJa9zuBc\n"
"VyqoH3CXTXDInT+8a+c3aEVMJ2St+pSn4ed+WkDp8ijsijvEyFwE47hulW0Ltzjg\n"
"9fOV5Pmrg/zxWbRuL+k0DBDHEJennCsAen7c35Pmx7jpmJ/HtgRhcnz0yjSBvyIw\n"
"6L1QIupkCv2SBODT/xDD3gfQQyKv6roV4G2EhfEyAsWpmojxjCUCGiyg97FvDtm/\n"
"NK2LSc9lybKxB73I2+P2G3CaWpvvpAiHCVu30jW8GCxKdfhsXtnIy2imskQqVZ2m\n"
"0Pmxobb28Tucr7xBK7CtwvPrb79os7u2XP3O5f9b/H66GNyRrglRXlrYjI1oGYL/\n"
"f4I1n/Sgusda6WvA6C190kxjU15Y12mHU4+BxyR9cx2hhGS9fAjMZKJss28qxvz6\n"
"Axu4CaDmRNZpK/pQrXF17yXCXkmEWgvSOEZy6Z9pcbLIVEGckV/iVeq0AOo2pkg9\n"
"p4QRIy0tK2diRENLSF2KysFwbY6B26BFeFs3v1sYVRhFW9nLkOrQVporCS0KyZmf\n"
"wVD89qSTlnctLcZnIavjKsKUu1nA1iU0yYMdYepKR7lWbnwhdx3ewok=\n"
"-----END CERTIFICATE-----\n";







/**
 * @brief Main application function.
 *
 * This function initializes the necessary hardware components such as the touch screen
 * and RGB LCD display, sets up the LVGL library for graphics rendering, and runs
 * the LVGL demo UI.
 *
 * - Initializes the GT911 touch screen controller.
 * - Initializes the Waveshare ESP32-S3 RGB LCD display.
 * - Initializes the LVGL library for graphics rendering.
 * - Runs the LVGL demo UI.
 *
 * @return None
 */


#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "mqtt_client.h" // Thư viện MQTT client của ESP-IDF

/*
#define TAG_RETRY "MQTT_RETRY"
#define NVS_NAMESPACE "wifi_cred" 
#define MAX_MISSED_MESSAGES 100   
#define RETRY_DELAY_SEC 60        
*/


/**
 * @brief Đọc, Publish và Xóa tin nhắn bị lỡ từ NVS
 */

#define MQTT_RETRY_TASK_STACK_SIZE 4096
#define MQTT_RETRY_TASK_PRIORITY 5

static TaskHandle_t mqtt_retry_task_handle = NULL;



esp_err_t read_backup_message(nvs_handle_t nvs_handle, const char *key, char **topic, char **payload)
{
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    
    if (err != ESP_OK) {
        return err;
    }

    char *backup_data = malloc(required_size);
    if (backup_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_str(nvs_handle, key, backup_data, &required_size);
    if (err != ESP_OK) {
        free(backup_data);
        return err;
    }

    cJSON *json = cJSON_Parse(backup_data);
    free(backup_data);
    
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *topic_item = cJSON_GetObjectItem(json, "topic");
    cJSON *payload_item = cJSON_GetObjectItem(json, "payload");

    if (topic_item == NULL || payload_item == NULL) {
        ESP_LOGE(TAG, "Invalid JSON structure");
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    *topic = strdup(topic_item->valuestring);
    *payload = strdup(payload_item->valuestring);

    cJSON_Delete(json);
    return ESP_OK;
}

esp_err_t delete_backup_message(nvs_handle_t nvs_handle, const char *key)
{
    esp_err_t err = nvs_erase_key(nvs_handle, key);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    return err;
}

uint32_t get_backup_count(nvs_handle_t nvs_handle)
{
    uint32_t key_id = 0;
    esp_err_t err = nvs_get_u32(nvs_handle, "key_id", &key_id);
    if (err != ESP_OK) {
        return 0;
    }
    return key_id;
}



void mqtt_retry_publish(void)
{
    ESP_LOGI(TAG, "MQTT Retry Publish started");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("MQTT_BACKUP", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return;
    }

    uint32_t total_messages = get_backup_count(nvs_handle);
    if (total_messages == 0) {
        ESP_LOGI(TAG, "No backup messages to send");
        nvs_close(nvs_handle);
        return;
    }

    ESP_LOGI(TAG, "Found %lu backup messages to send", total_messages);

    if (mqttClient) {
        for (uint32_t i = 0; i < total_messages; i++) {
            char key[16];
            snprintf(key, sizeof(key), "msg_%lu", i);

            char *topic = NULL;
            char *payload = NULL;

            err = read_backup_message(nvs_handle, key, &topic, &payload);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Skip key %s: %s", key, esp_err_to_name(err));
                continue;
            }

            ESP_LOGI(TAG, "Attempting to send backup: %s -> %s", key, topic);

            bool sent_success = false;
            for (int retry = 0; retry < 5; retry++) {
                int msg_id = esp_mqtt_client_publish(mqttClient, topic, payload, 0, 1, 0);
                if (msg_id >= 0) {
                    ESP_LOGI(TAG, "Sent successfully: key=%s, msg_id=%d", key, msg_id);
                    sent_success = true;
                    break;
                } else {
                    ESP_LOGW(TAG, "Send failed (attempt %d/%d): %s", retry + 1, 5, key);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }

            if (sent_success) {
                err = delete_backup_message(nvs_handle, key);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Deleted backup: %s", key);
                } else {
                    ESP_LOGE(TAG, "Failed to delete backup: %s", key);
                }
            }

            free(topic);
            free(payload);
            vTaskDelay(pdMS_TO_TICKS(500)); // nhỏ hơn để mượt hơn
        }
    }

    // Kiểm tra còn lại tin nào không
    uint32_t remaining = 0;
    for (uint32_t i = 0; i < total_messages; i++) {
        char key[16];
        snprintf(key, sizeof(key), "msg_%lu", i);
        size_t required_size;
        if (nvs_get_str(nvs_handle, key, NULL, &required_size) == ESP_OK) {
            remaining++;
        }
    }

    if (remaining == 0) {
        ESP_LOGI(TAG, "All backups sent, resetting key_id");
        nvs_set_u32(nvs_handle, "key_id", 0);
        nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "MQTT Retry Publish finished");
}




void client_mqtt_retry_publish(void)
{
    ESP_LOGI(TAG, "Client MQTT Retry Publish started");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("MQTT_BACKUP2", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return;
    }

    uint32_t total_messages = get_backup_count(nvs_handle);
    if (total_messages == 0) {
        ESP_LOGI(TAG, "No backup messages to send");
        nvs_close(nvs_handle);
        return;
    }

    ESP_LOGI(TAG, "Found %lu backup messages to send", total_messages);

    if (mqttClient) {
        for (uint32_t i = 0; i < total_messages; i++) {
            char key[16];
            snprintf(key, sizeof(key), "msg_%lu", i);

            char *topic = NULL;
            char *payload = NULL;

            err = read_backup_message(nvs_handle, key, &topic, &payload);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Skip key %s: %s", key, esp_err_to_name(err));
                continue;
            }

            ESP_LOGI(TAG, "Attempting to send backup: %s -> %s", key, topic);

            bool sent_success = false;
            for (int retry = 0; retry < 5; retry++) {
                int msg_id = esp_mqtt_client_publish(mqttClient, topic, payload, 0, 1, 0);
                if (msg_id >= 0) {
                    ESP_LOGI(TAG, "Sent successfully: key=%s, msg_id=%d", key, msg_id);
                    sent_success = true;
                    break;
                } else {
                    ESP_LOGW(TAG, "Send failed (attempt %d/%d): %s", retry + 1, 5, key);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }

            if (sent_success) {
                err = delete_backup_message(nvs_handle, key);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Deleted backup: %s", key);
                } else {
                    ESP_LOGE(TAG, "Failed to delete backup: %s", key);
                }
            }

            free(topic);
            free(payload);
            vTaskDelay(pdMS_TO_TICKS(500)); // nhỏ hơn để mượt hơn
        }
    }

    // Kiểm tra còn lại tin nào không
    uint32_t remaining = 0;
    for (uint32_t i = 0; i < total_messages; i++) {
        char key[16];
        snprintf(key, sizeof(key), "msg_%lu", i);
        size_t required_size;
        if (nvs_get_str(nvs_handle, key, NULL, &required_size) == ESP_OK) {
            remaining++;
        }
    }

    if (remaining == 0) {
        ESP_LOGI(TAG, "All backups sent, resetting key_id");
        nvs_set_u32(nvs_handle, "key_id", 0);
        nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "MQTT Retry Publish finished");
}








void backup_mqtt_data(const char *topic, const char *payload)
{   
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("MQTT_BACKUP", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return;
    }

    uint32_t current_key_id = 0;
    err = nvs_get_u32(nvs_handle, "key_id", &current_key_id);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to read key_id: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    char key[16];
    snprintf(key, sizeof(key), "msg_%lu", current_key_id);
    
    cJSON *msg_json = cJSON_CreateObject();
    if (msg_json == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        nvs_close(nvs_handle);
        return;
    }
    
    cJSON_AddStringToObject(msg_json, "topic", topic);
    cJSON_AddStringToObject(msg_json, "payload", payload);
    char *backup_data = cJSON_PrintUnformatted(msg_json);
    
    if (backup_data == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(msg_json);
        nvs_close(nvs_handle);
        return;
    }

    err = nvs_set_str(nvs_handle, key, backup_data);
    if (err == ESP_OK) {
        current_key_id++;
        err = nvs_set_u32(nvs_handle, "key_id", current_key_id);
        
        if (err == ESP_OK) {
            err = nvs_commit(nvs_handle);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Saved successfully, key=%s, next_id=%lu", key, current_key_id);
            } else {
                ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGE(TAG, "Failed to update key_id: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed: %s", esp_err_to_name(err));
    }

    cJSON_Delete(msg_json);
    free(backup_data);
    nvs_close(nvs_handle);
}



void backup_client_mqtt_data(const char *topic, const char *payload)
{   
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("MQTT_BACKUP2", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return;
    }

    uint32_t current_key_id = 0;
    err = nvs_get_u32(nvs_handle, "key_id", &current_key_id);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to read key_id: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    char key[16];
    snprintf(key, sizeof(key), "msg_%lu", current_key_id);
    
    cJSON *msg_json = cJSON_CreateObject();
    if (msg_json == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        nvs_close(nvs_handle);
        return;
    }
    
    cJSON_AddStringToObject(msg_json, "topic", topic);
    cJSON_AddStringToObject(msg_json, "payload", payload);
    char *backup_data = cJSON_PrintUnformatted(msg_json);
    
    if (backup_data == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(msg_json);
        nvs_close(nvs_handle);
        return;
    }

    err = nvs_set_str(nvs_handle, key, backup_data);
    if (err == ESP_OK) {
        current_key_id++;
        err = nvs_set_u32(nvs_handle, "key_id", current_key_id);
        
        if (err == ESP_OK) {
            err = nvs_commit(nvs_handle);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Saved successfully, key=%s, next_id=%lu", key, current_key_id);
            } else {
                ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGE(TAG, "Failed to update key_id: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed: %s", esp_err_to_name(err));
    }

    cJSON_Delete(msg_json);
    free(backup_data);
    nvs_close(nvs_handle);
}






void check_device_message(const char *json_string) {
    cJSON *root = cJSON_Parse(json_string);

    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("cJSON Parse Error!");
        }
        return;
    }

    cJSON *name_item = cJSON_GetObjectItemCaseSensitive(root, "name");

    if (cJSON_IsString(name_item) && (name_item->valuestring != NULL)) {
        const char *device_name = name_item->valuestring;
        

        if (strcmp(device_name, "Device-01") == 0) {
            printf("Found Device-01's message\n");
        } 
    } 

    cJSON_Delete(root);
}




void mqtt_retry_publish_task(void *pvParameters)
{
    ESP_LOGI(TAG, "MQTT Retry Publish Task started");

    while (1) {
        // Chờ notification để bắt đầu retry
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Starting MQTT retry process");

        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open("MQTT_BACKUP", NVS_READWRITE, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
            continue;
        }

        uint32_t total_messages = get_backup_count(nvs_handle);
        if (total_messages == 0) {
            ESP_LOGI(TAG, "No backup messages to send");
            nvs_close(nvs_handle);
            continue;
        }

        ESP_LOGI(TAG, "Found %lu backup messages to send", total_messages);

        if (mqttClient) {
            for (uint32_t i = 0; i < total_messages; i++) {
                char key[16];
                snprintf(key, sizeof(key), "msg_%lu", i);

                char *topic = NULL;
                char *payload = NULL;

                err = read_backup_message(nvs_handle, key, &topic, &payload);
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "Skip key %s: %s", key, esp_err_to_name(err));
                    continue;
                }

                ESP_LOGI(TAG, "Attempting to send backup: %s -> %s", key, topic);

                bool sent_success = false;
                for (int retry = 0; retry < 5; retry++) {
                    int msg_id = esp_mqtt_client_publish(mqttClient, topic, payload, 0, 1, 0);
                    if (msg_id >= 0) {
                        ESP_LOGI(TAG, "Sent successfully: key=%s, msg_id=%d", key, msg_id);
                        sent_success = true;
                        break;
                    } else {
                        ESP_LOGW(TAG, "Send failed (attempt %d/%d): %s", retry + 1, 5, key);
                        vTaskDelay(pdMS_TO_TICKS(1000));
                    }
                }

                if (sent_success) {
                    err = delete_backup_message(nvs_handle, key);
                    if (err == ESP_OK) {
                        ESP_LOGI(TAG, "Deleted backup: %s", key);
                    } else {
                        ESP_LOGE(TAG, "Failed to delete backup: %s", key);
                    }
                }

                free(topic);
                free(payload);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }

        // Kiểm tra còn lại tin nào không
        uint32_t remaining = 0;
        for (uint32_t i = 0; i < total_messages; i++) {
            char key[16];
            snprintf(key, sizeof(key), "msg_%lu", i);
            size_t required_size;
            if (nvs_get_str(nvs_handle, key, NULL, &required_size) == ESP_OK) {
                remaining++;
            }
        }

        if (remaining == 0) {
            ESP_LOGI(TAG, "All backups sent, resetting key_id");
            nvs_set_u32(nvs_handle, "key_id", 0);
            nvs_commit(nvs_handle);
        }

        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "MQTT Retry Publish finished");
    }

    vTaskDelete(NULL); // Không bao giờ đến đây
}

// Hàm khởi tạo task
esp_err_t mqtt_retry_task_init(void)
{
    BaseType_t ret = xTaskCreate(
        mqtt_retry_publish_task,
        "mqtt_retry",
        MQTT_RETRY_TASK_STACK_SIZE,
        NULL,
        MQTT_RETRY_TASK_PRIORITY,
        &mqtt_retry_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create MQTT retry task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT retry task created successfully");
    return ESP_OK;
}

// Hàm trigger retry từ bất kỳ đâu (ví dụ: khi MQTT reconnect)
void mqtt_trigger_retry(void)
{
    if (mqtt_retry_task_handle != NULL) {
        xTaskNotifyGive(mqtt_retry_task_handle);
        ESP_LOGI(TAG, "MQTT retry triggered");
    }
}

// Hàm dừng task (nếu cần)
void mqtt_retry_task_stop(void)
{
    if (mqtt_retry_task_handle != NULL) {
        vTaskDelete(mqtt_retry_task_handle);
        mqtt_retry_task_handle = NULL;
        ESP_LOGI(TAG, "MQTT retry task stopped");
    }
}






///////// mqtt


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
        //xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 7, NULL);
        //mqtt_retry_publish();
        //client_mqtt_retry_publish();
        mqtt_trigger_retry();
        lv_obj_add_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN ); 
        msg_id = esp_mqtt_client_subscribe(client, "feedback", 0);



        mqtt=1;

        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        lv_obj_clear_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN ); 
         mqtt=0;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        break;
        
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);


        char *json_buffer = NULL; 

        if (event->data_len > 0) {
            json_buffer = (char *)malloc(event->data_len + 1); 
            if (json_buffer == NULL) {
                ESP_LOGE(TAG, "Error!");
                break;
            }

            memcpy(json_buffer, event->data, event->data_len);
            json_buffer[event->data_len] = '\0';

            ESP_LOGI(TAG, "MQTT_EVENT_DATA. Payload: %s", json_buffer);


            free(json_buffer);
        }


        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(MQTT_TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(MQTT_TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(MQTT_TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(MQTT_TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(MQTT_TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
        
    default:
        ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
        break;
    }
}



void mqtt_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            //.address.uri = "mqtt://broker.hivemq.com",// exam
            //.address.port = 1883 //exam
           // .address.uri = "mqtts://0b2c802533b54670a78953e3c5758528.s1.eu.hivemq.cloud",
            .address.port = 1883,//du sap cung deo dc keu dmm
            .address.uri = "mqtt://10.10.1.27",
           // .address.port = 1883,
            //.verification.certificate = (const char *)trustid_x3_root_pem_start,
         //  .verification.certificate = (const char *)mqtt_ca_cert_pem,
          //  .verification.certificate_len = strlen(isrgrootx1_pem_start) + 1,
             //.verification.certificate_len = isrgrootx1_pem_end - isrgrootx1_pem_start,
                 //    .verification.certificate =      mqtt_cert_pem,
                      
              
        },

        .credentials = {
            .username = "appuser",
            .authentication.password = "1111",
        },
        
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttClient = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}






////////////gatt
void mainscreen_wifi_rssi_task(void *pvParameters) {
    wifi_ap_record_t ap_info;
   
    while (1) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            connect_success=1;
            ESP_LOGI("RSSI", "Connected SSID:%s, BSSID:%02X:%02X:%02X:%02X:%02X:%02X, RSSI:%d dBm",
                     ap_info.ssid,
                     ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                     ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5],
                     ap_info.rssi);


              if (ap_info.rssi == 0 && ap_info.ssid[0] == '\0')
        {    
            //if (lvgl_port_lock(0)) {
            lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );  
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );

            //lvgl_port_unlock();
           // }   
            //break;
            //vTaskDelete(NULL);
        }
         
         else if(ap_info.rssi > -25)  // Strong signal (RSSI > -25)
        {   
            lv_obj_clear_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with strong signal icon
          //  WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_4_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info.rssi < -25) && (ap_info.rssi > -50))  // Medium signal
        { 
            lv_obj_clear_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with medium signal icon
          //  WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_3_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info.rssi < -50) && (ap_info.rssi > -75))  // Weak signal
        {    
            lv_obj_clear_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with weak signal icon
           // WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_2_png, (const char *)ap_info[i].ssid);
        }
        else  // Very weak signal (RSSI < -75)
        { 
            lv_obj_clear_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with very weak signal icon
              //_ui_flag_modify(ui_WIFI_SCAN_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);      
   }         
        } else {
            ESP_LOGW("RSSI", "Not connected to any AP");
            connect_success=0;
            lv_obj_clear_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN ); //

            lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );  
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
        }
      
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay 2s
    }
}



void app_main()
{
    // Initialize the Non-Volatile Storage (NVS) for settings and data persistence
    // This ensures that user data and settings are retained even after power loss.
  //  init_nvs();
  //////////////
 

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase and re-initialize if no free pages or new version found
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

  // ble_init();
    
    // Open NVS for reading
    /*
    nvs_handle_t nvs_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS handle!\n");
        return false;
    }
*/
    // Read username from NVS
    /*
    size_t username_size = MAX_LENGTH;
    err = nvs_get_str(nvs_handle, USERNAME_KEY, saved_username, &username_size);
    if (err != ESP_OK) {
        printf("Read failed!\n");
        nvs_close(nvs_handle);
        return false;
    }

    nvs_close(nvs_handle);

    return err;

*/


  ////////////////

    //static esp_lcd_panel_handle_t panel_handle = NULL; // Handle for the LCD panel
    //static esp_lcd_touch_handle_t tp_handle = NULL;    // Handle for the touch panel

    // Initialize the GT911 touch screen controller
    // This sets up the touch functionality of the screen.
    tp_handle = touch_gt911_init();

    // Initialize the Waveshare ESP32-S3 RGB LCD hardware
    // This prepares the LCD panel for display operations.
    panel_handle = waveshare_esp32_s3_rgb_lcd_init();

    // Turn on the LCD backlight
    // This ensures the display is visible.
    wavesahre_rgb_lcd_bl_on();

    // Initialize the LVGL library, linking it to the LCD and touch panel handles
    // LVGL is a lightweight graphics library used for rendering UI elements.
    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

    ESP_LOGI(TAG, "Display LVGL demos");

    

    // Lock the LVGL port to ensure thread safety during API calls
    // This prevents concurrent access issues when using LVGL functions.
    if (lvgl_port_lock(-1))
    {

        // Initialize the UI components with LVGL (e.g., demo screens, sliders)
        // This sets up the user interface elements using the LVGL library.


        ui_init();


        // Release the mutex after LVGL operations are complete
        // This allows other tasks to access the LVGL port.
        lvgl_port_unlock();
    }
    vTaskDelay(100); // Delay for a short period to ensure stable initialization

    // Initialize PWM for controlling backlight brightness (if applicable)
    // PWM is used to adjust the brightness of the LCD backlight.
    //pwm_init();

    // Initialize SD card operations
    // This sets up the Micro SD card for data storage and retrieval.
    //sd_init();

    // Start the WIFI task to handle Wi-Fi functionality
    // This task manages Wi-Fi connections and hotspot creation.
    xTaskCreate(wifi_task, "wifi_task", 8 * 1024, NULL, 12, &wifi_TaskHandle);
    

     xTaskCreate(ble_server_task, "ble_server_task", 8 * 1024, NULL, 9, NULL);
     xTaskCreate(mainscreen_wifi_rssi_task, "mainscreen_wifi_rssi_task", 4* 1024, NULL, 9, NULL);
     mqtt_retry_task_init();

    // xTaskCreate(mqtt_retry_publish_task, "MQTT_Retry_Task", 4096,  NULL,  5,    NULL);
     //xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 7, NULL);
     //xTaskCreate(&api_task, "api_task", 1024*4, NULL, 5, NULL);
     //xTaskCreate(send_message_task, "send_message_task", 8 * 1024, NULL, 10, NULL);
    // if (connection_flag){
          
    // }
     


    






}