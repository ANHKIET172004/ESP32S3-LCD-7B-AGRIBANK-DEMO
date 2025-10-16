#include "esp_mqtt_client.h"
#include "lvgl.h"
#include "lvgl_port.h"


//#define MQTT_RETRY_TASK_STACK_SIZE 4096
#define MQTT_RETRY_TASK_STACK_SIZE 6144
#define MQTT_RETRY_TASK_PRIORITY 5 //5


extern lv_obj_t * ui_Image20 ;
extern lv_obj_t * ui_Image24 ;
extern lv_obj_t * ui_Image31 ;
extern lv_obj_t * ui_Image34 ;
extern lv_obj_t * ui_Image32 ;

//lv_obj_t * ui_Image37 = NULL;

static TaskHandle_t mqtt_retry_task_handle = NULL;


esp_mqtt_client_handle_t mqttClient;

int mqtt=0;

uint8_t key_id=0;

// Khai báo semaphore global
SemaphoreHandle_t mqtt_retry_semaphore = NULL;
SemaphoreHandle_t mqtt_retry_semaphore2 = NULL;

static const char *TAG = "MQTT_BACKUP"; // Tag used for ESP log output
static const char *MQTT_TAG ="MQTT";

//static TaskHandle_t mqtt_retry_task_handle = NULL;

extern lv_obj_t * ui_Image38;

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



void save_number(const char *number)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_str(nvs_handle, "number", number);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Saved number: %s", number);
        } else {
            ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Failed to save number: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}

esp_err_t read_number(char *number, size_t max_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "number", NULL, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get size for number: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    if (required_size > max_len) {
        ESP_LOGE(TAG, "Buffer too small: required %d bytes, provided %d", required_size, max_len);
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_SIZE;
    }

    err = nvs_get_str(nvs_handle, "number", number, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read number: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "Read number: %s", number);
    nvs_close(nvs_handle);
    return ESP_OK;
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
        // BLOCK tại đây cho đến khi nhận semaphore
        if (xSemaphoreTake(mqtt_retry_semaphore, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Semaphore received, starting MQTT retry process");

            nvs_handle_t nvs_handle;
            esp_err_t err = nvs_open("MQTT_BACKUP", NVS_READWRITE, &nvs_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
                // KHÔNG trả semaphore ở đây, để task tiếp tục block
                continue;
            }

            uint32_t total_messages = get_backup_count(nvs_handle);
            if (total_messages == 0) {
                ESP_LOGI(TAG, "No backup messages to send");
                nvs_close(nvs_handle);
                // KHÔNG trả semaphore, để task tiếp tục block
                continue;
            }

            ESP_LOGI(TAG, "Found %lu backup messages to send", total_messages);

            uint32_t sent_count = 0;
            uint32_t failed_count = 0;

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
                            sent_count++;
                        } else {
                            ESP_LOGE(TAG, "Failed to delete backup: %s", key);
                            failed_count++;
                        }
                    } else {
                        failed_count++;
                    }

                    free(topic);
                    free(payload);
                    vTaskDelay(pdMS_TO_TICKS(100)); // Tăng delay để giảm tải
                }
            }

            ESP_LOGI(TAG, "Retry summary: sent=%lu, failed=%lu", sent_count, failed_count);

            // Kiểm tra và reset key_id nếu cần
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
            
            // KHÔNG gọi xSemaphoreGive ở đây!
            // Task sẽ loop lại và BLOCK tại xSemaphoreTake
        }
    }

    vTaskDelete(NULL);
}








void mqtt_retry_publish_task2(void *pvParameters)
{
    ESP_LOGI(TAG, "MQTT Retry Publish Task started");

    while (1) {
        if (xSemaphoreTake(mqtt_retry_semaphore, portMAX_DELAY) == pdTRUE) {
        // Chờ notification để bắt đầu retry
       // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        ESP_LOGI(TAG, "Starting MQTT retry process");

        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open("MQTT_BACKUP2", NVS_READWRITE, &nvs_handle);
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
        xSemaphoreGive(mqtt_retry_semaphore); // Trả lại semaphore sau khi hoàn thành
    }
}

    vTaskDelete(NULL); // Không bao giờ đến đây
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
        //mqtt_trigger_retry();
        trigger_mqtt_retry();
        lv_obj_add_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN ); 
        //msg_id = esp_mqtt_client_subscribe(client, "feedback", 0);



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
       // .network.disable_auto_reconnect = false,
        
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttClient = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}



// Khởi tạo trong hàm init
void mqtt_retry_init(void)
{
    // Tạo binary semaphore, bắt đầu ở trạng thái "taken" (không available)
    mqtt_retry_semaphore = xSemaphoreCreateBinary();
    
    if (mqtt_retry_semaphore == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }
    
    // Tạo task
  //  xTaskCreate(mqtt_retry_publish_task, "mqtt_retry", 4096, NULL, 5, NULL);
    // Tạo task và pin vào core 1
xTaskCreatePinnedToCore(mqtt_retry_publish_task,  "mqtt_retry",  3072, NULL, 3, NULL, 1 );
}


void mqtt_retry_init2(void)
{
    // Tạo binary semaphore, bắt đầu ở trạng thái "taken" (không available)
    mqtt_retry_semaphore2 = xSemaphoreCreateBinary();
    
    if (mqtt_retry_semaphore2 == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }
    
    // Tạo task
  //  xTaskCreate(mqtt_retry_publish_task, "mqtt_retry", 4096, NULL, 5, NULL);
    // Tạo task và pin vào core 1
xTaskCreatePinnedToCore(
    mqtt_retry_publish_task2,  "mqtt_retry2",  4096, NULL, 2, NULL, 1 );
}
/*
void mqtt_retry_publish_task(void *pvParameters)
{
    ESP_LOGI(TAG, "MQTT Retry Publish Task started");

    while (1) {
        // Chờ nhận semaphore (blocking vô hạn)
        if (xSemaphoreTake(mqtt_retry_semaphore, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Semaphore received, starting MQTT retry process");

            nvs_handle_t nvs_handle;
            esp_err_t err = nvs_open("MQTT_BACKUP", NVS_READWRITE, &nvs_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
                xSemaphoreGive(mqtt_retry_semaphore); // Trả lại semaphore
                continue;
            }

            uint32_t total_messages = get_backup_count(nvs_handle);
            if (total_messages == 0) {
                ESP_LOGI(TAG, "No backup messages to send");
                nvs_close(nvs_handle);
                xSemaphoreGive(mqtt_retry_semaphore); // Trả lại semaphore
                continue;
            }

            ESP_LOGI(TAG, "Found %lu backup messages to send", total_messages);

            uint32_t sent_count = 0;
            uint32_t failed_count = 0;

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
                            sent_count++;
                        } else {
                            ESP_LOGE(TAG, "Failed to delete backup: %s", key);
                            failed_count++;
                        }
                    } else {
                        failed_count++;
                    }

                    free(topic);
                    free(payload);
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
            }

            ESP_LOGI(TAG, "Retry summary: sent=%lu, failed=%lu", sent_count, failed_count);

            // Compact lại các key còn lại (nếu cần)
            if (sent_count > 0) {
                compact_backup_keys(nvs_handle, total_messages);
            }

            nvs_close(nvs_handle);
            
            ESP_LOGI(TAG, "MQTT Retry Publish finished, returning semaphore");
            xSemaphoreGive(mqtt_retry_semaphore); // Trả lại semaphore sau khi hoàn thành
        }
    }

    vTaskDelete(NULL);
}
*/
void trigger_mqtt_retry(void)
{
    if (mqtt_retry_semaphore != NULL) {
        if (xSemaphoreGive(mqtt_retry_semaphore) == pdTRUE) {
            ESP_LOGI(TAG, "Triggered MQTT retry task");
        } else {
            ESP_LOGW(TAG, "Retry task is already running");
        }
    }
}

void trigger_mqtt_retry2(void)
{
    if (mqtt_retry_semaphore2 != NULL) {
        if (xSemaphoreGive(mqtt_retry_semaphore2) == pdTRUE) {
            ESP_LOGI(TAG, "Triggered MQTT retry task 2");
        } else {
            ESP_LOGW(TAG, "Retry task 2 is already running");
        }
    }
}

