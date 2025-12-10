#include "esp_mqtt_client.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "mqtt_data.h"

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif


extern lv_obj_t * ui_Image20 ;
extern lv_obj_t * ui_Image24 ;
extern lv_obj_t * ui_Image31 ;
extern lv_obj_t * ui_Image34 ;
extern lv_obj_t * ui_Image32 ;

//lv_obj_t * ui_Image37 = NULL;



esp_mqtt_client_handle_t mqttClient;


uint8_t key_id=0;

extern lv_obj_t * ui_TextArea4 ;


static const char *TAG = "MQTT_SAVE_NUMBER"; // Tag used for ESP log output
static const char *MQTT_TAG ="MQTT";

extern SemaphoreHandle_t check_sema;

extern int8_t pressed;

#include "cJSON.h"
#include "esp_log.h"

#define MAX_DEVICES 20
#define MAX_NAME_LEN 32
#define MAX_ID_LEN   32

typedef struct {
    char topic[64];
    char data[512];
} mqtt_message_t;

extern QueueHandle_t mqtt_queue;

 device_info_t device_list[MAX_DEVICES];
 int device_count = 0;



extern lv_obj_t * ui_Image38;

extern lv_obj_t* ui_TextArea2;
extern lv_obj_t* ui_TextArea3;



char selected_keypad_id[18]={0};


extern volatile bool checktime_stop;


void mqtt_process_task(void *pvParameters)
{
    mqtt_message_t msg;

    while (1) {
        if (xQueueReceive(mqtt_queue, &msg, portMAX_DELAY)) {

            ESP_LOGI(TAG, "Processing topic: %s", msg.topic);
            ESP_LOGI(TAG, "Data: %s", msg.data);

            // Parse JSON từ payload
            cJSON *root = cJSON_Parse(msg.data);
            if (!root&&strcmp(msg.topic,"reset_number")!=0&&strcmp(msg.topic,"transfer_number")!=0) {
                ESP_LOGE(TAG, "Failed to parse JSON");
                continue;
            }

            // Xử lý topic "number"
            if (strcmp(msg.topic, "number") == 0) {
                cJSON *device_id_item = cJSON_GetObjectItem(root, "device_id");
                cJSON *number_item    = cJSON_GetObjectItem(root, "number");
                cJSON *skip_item      = cJSON_GetObjectItem(root, "skip");

                if (cJSON_IsString(device_id_item) && cJSON_IsString(number_item)) {

                    if (load_selected_device_id(selected_keypad_id, sizeof(selected_keypad_id)) != ESP_OK) {
                        selected_keypad_id[0] = '\0'; 
                    }


                    if (strcmp(device_id_item->valuestring, selected_keypad_id) == 0) {

                        ESP_LOGI(TAG, "Received number: %s", number_item->valuestring);

                        if (skip_item && cJSON_IsString(skip_item) &&
                            strcmp(skip_item->valuestring, "yes") == 0) {
                            ESP_LOGI(TAG, "Skip number");
                            //checktime_stop=true;
                            skip_number(number_item->valuestring);  
                           
                        }
                         
                        else if (skip_item && cJSON_IsString(skip_item) &&
                            strcmp(skip_item->valuestring, "no") == 0){
                            checktime_stop=false;
                            
                            save_number(number_item->valuestring);
                        }

                    }
                } else {
                    ESP_LOGW(TAG, "Invalid JSON fields in 'number' topic");
                }
            }

            else if (strcmp(msg.topic, "device/list") == 0) {
                ESP_LOGI(TAG, "Device list received");
                parse_json_and_store(msg.data);   
                save_device_list_to_nvs();       
            }
             else if (strcmp(msg.topic, "reset_number") == 0) {
                ESP_LOGI(TAG, "Reset all number");
                delete_current_number();
                
                delete_next_number();
                
            }

            else if (strcmp(msg.topic, "transfer_number") == 0) {
                ESP_LOGI(TAG, "transfer number");            
                if (transfer_number()==ESP_OK){
                    ESP_LOGI(TAG, "transfered number successfully");
                }
                else {
                     ESP_LOGI(TAG, "transfered number failed");
                }
                
                
            }

            else if (strcmp(msg.topic, "check current number") == 0) {
                char current_num[5];
                size_t len=sizeof(current_num);
                read_number(current_num,len);
               
                ESP_LOGI(TAG, "check current number");
               
                ESP_LOGI(TAG, "current number: %s",current_num);
                if (strcmp(msg.data,current_num)!=0){
                   
                   save_current_number(msg.data);
                   /*
                    if (lvgl_port_lock(-1)){
                    lv_textarea_set_placeholder_text(ui_TextArea2, msg.data);
                    lvgl_port_unlock();
                    }
                    */
                   delete_next_number();
                   ESP_LOGI(TAG, "Save new current number");


                }
                else {
                    ESP_LOGI(TAG, "No new current number, skip saving");
                }

              
             
            }

            else {
                ESP_LOGI(TAG, "Unhandled topic: %s", msg.topic);
            }

            // Giải phóng bộ nhớ JSON
            cJSON_Delete(root);
        }
    }
}



void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");

        msg_id = esp_mqtt_client_publish(mqttClient, "feedback_status", "", 0, 1, 1);// xóa retained mess

        if (msg_id >= 0) {
            ESP_LOGI(TAG, "deleted retained message to keypad successfully, msg_id=%d",msg_id);
        } else {
            ESP_LOGW(TAG, "delete retained message to keypad failed!");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        msg_id = esp_mqtt_client_publish(mqttClient, "feedback_status", "connected", 0, 1, 0);// gửi thông báo đến topic đã kết nối thành công

        if (msg_id >= 0) {
            ESP_LOGI(TAG, "Sent connection message to keypad successfully, msg_id=%d",msg_id);
        } else {
            ESP_LOGW(TAG, "Send connection message to keypad failed!");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }


        if (lvgl_port_lock(-1)) {
        lv_obj_add_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN ); // ẩn icon lỗi kết nối
        lvgl_port_unlock();
        }
        
        msg_id = esp_mqtt_client_subscribe(client, "number", 0);

        esp_mqtt_client_subscribe(event->client, "device/list", 1);
        esp_mqtt_client_subscribe(event->client, "check current number", 0);
        esp_mqtt_client_subscribe(event->client, "reset_number", 0);
        esp_mqtt_client_subscribe(event->client, "transfer_number", 0);



        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");

        
        if (lvgl_port_lock(-1)) {
        lv_obj_clear_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN ); 
        lvgl_port_unlock();
        }
        
        break;

    case MQTT_EVENT_SUBSCRIBED:
        //ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        if (event->data && event->data_len > 0) {
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x",
                    event->msg_id, (uint8_t)event->data[0]);
        } else {
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        }
        break;
        
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_DATA:
        mqtt_message_t msg;
        int topic_len = MIN(event->topic_len, sizeof(msg.topic) - 1);
        int data_len  = MIN(event->data_len, sizeof(msg.data) - 1);

        memcpy(msg.topic, event->topic, topic_len);
        msg.topic[topic_len] = '\0';

        memcpy(msg.data, event->data, data_len);
        msg.data[data_len] = '\0';

        if (xQueueSend(mqtt_queue, &msg, 0) != pdTRUE) {
            ESP_LOGW(TAG, "MQTT queue full, message dropped");
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

            .address.port = 1883,
            .address.uri = "mqtt://10.10.1.21",
                      
              
        },
        //.network.timeout_ms = 10000,  
        .credentials = {
            .username = "appuser",
            .authentication.password = "1111",
        },



        .network.disable_auto_reconnect = false,
        
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttClient = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}



