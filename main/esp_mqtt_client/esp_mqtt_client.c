#include "esp_mqtt_client.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "mqtt_data.h"
#include "list_handler.h"
#include "ui.h"

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif


extern lv_obj_t * ui_Imag9 ;
extern lv_obj_t * ui_Image10 ;
extern lv_obj_t * ui_Image11 ;
extern lv_obj_t * ui_Image13 ;
extern lv_obj_t * ui_Image12 ;

esp_mqtt_client_handle_t mqttClient;


uint8_t key_id=0;

extern lv_obj_t * ui_TextArea4 ;
extern lv_obj_t* ui_Image26;


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

uint8_t staff_id=0;

char id_str[4]={0};

 device_info_t device_list[MAX_DEVICES];
 int device_count = 0;
extern char device_mac[18];


extern lv_obj_t * ui_Image16;

char selected_keypad_id[18]={0};

char login_topic[32]={0};

//extern volatile bool checktime_stop;

extern bool login;

extern QueueHandle_t ui_queue;

//ui_evt_t evt;

void publish_backup_mqtt_msg(esp_mqtt_client_handle_t mqttClient)
{
    nvs_handle_t nvs;
    esp_err_t err;

    err = nvs_open("BACKUP_MQTT", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Open BACKUP_MQTT failed");
        return;
    }

    uint8_t cnt = 0;
    err = nvs_get_u8(nvs, "cnt", &cnt);
    if (err != ESP_OK || cnt == 0) {
        ESP_LOGI("NVS", "No backup MQTT msg");
        nvs_close(nvs);
        return;
    }

    char msg[256];
    size_t len;

    ESP_LOGI("NVS", "Restore %d MQTT messages", cnt);

    for (uint8_t i = 0; i < cnt; i++) {
        char key[10];
        snprintf(key, sizeof(key), "msg%d", i);

        len = sizeof(msg);
        err = nvs_get_str(nvs, key, msg, &len);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Read %s failed", key);
            continue;
        }

        int msg_id = esp_mqtt_client_publish(  mqttClient, "feedback",msg,0,0, 0  );

        if (msg_id == -1) {
            ESP_LOGE("MQTT", "Publish backup failed (%s)", key);
            break;  
        }

        ESP_LOGI("MQTT", "Publish backup msg successfully: %s", key);

        nvs_erase_key(nvs, key);
    }

    nvs_set_u8(nvs, "cnt", 0);
    nvs_commit(nvs);
    nvs_close(nvs);

    ESP_LOGI("NVS", "Publish all backup MQTT messages ");
}


void save_login_status(const char *status)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("login", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("LOGIN STATUS", "Failed to open NVS handle!");
        return;
    }

    
    char old_status[4] = {0};

    size_t status_len = sizeof(old_status);


    nvs_get_str(nvs_handle, "status", old_status, &status_len);

    bool status_update = false;


    if (strcmp(status, old_status) != 0) {
        status_update = true;
    }
    if (!status_update){
        ESP_LOGI("LOGIN STATUS", "status no change, skip update");
        nvs_close(nvs_handle);
        return;
    }

    ESP_LOGI("LOGIN STATUS", "Updating login status in NVS...");

    nvs_set_str(nvs_handle, "status", status);

    err = nvs_commit(nvs_handle);
    if (err == ESP_OK) {
        ESP_LOGI("LOGIN STATUS", "login status saved successfully!");
    } else {
        ESP_LOGE("LOGIN STATUS", "Failed to save login status!");
    }

    nvs_close(nvs_handle);
}

void save_last_id(uint16_t value)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("last_id", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("LOGIN STATUS", "Failed to open NVS handle!");
        return;
    }

    
    uint16_t old_value=0;



    nvs_get_u16(nvs_handle, "value", &old_value);

    bool value_update = false;


    if (value != old_value) {
        value_update = true;
    }
    if (!value_update){
        ESP_LOGI("LAST ID", "value no change, skip update");
        nvs_close(nvs_handle);
        return;
    }

    ESP_LOGI("LAST ID", "Updating last id in NVS...");

    nvs_set_u16(nvs_handle,"value",value);

    err = nvs_commit(nvs_handle);
    if (err == ESP_OK) {
        ESP_LOGI("LAST ID", "last id saved successfully!");
    } else {
        ESP_LOGE("LOGIN STATUS", "Failed to save last id!");
    }

    nvs_close(nvs_handle);
}

    uint16_t read_last_id(){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("last_id", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("LOGIN STATUS", "Failed to open NVS handle!");
        return 0;
    }

    uint16_t value;

    err=nvs_get_u16(nvs_handle,"value",&value);
    if (err==ESP_OK){
        nvs_close(nvs_handle);
        return value;
    }
    else {
        nvs_close(nvs_handle);
        return 0;
    }
}




int device_compare_by_name(const void *a, const void *b)
{
    const device_info_t *da = (const device_info_t *)a;
    const device_info_t *db = (const device_info_t *)b;

    return strcmp(da->name, db->name);
}


void handle_number_topic(cJSON* root){
      cJSON* device_id_item=cJSON_GetObjectItem(root,"device_id");
      cJSON* number_item=cJSON_GetObjectItem(root,"number");
      cJSON* skip_item=cJSON_GetObjectItem(root,"skip");

      if (cJSON_IsString(device_id_item)&&cJSON_IsString(number_item)&&cJSON_IsString(skip_item)){
         if( load_selected_device_id(selected_keypad_id,sizeof(selected_keypad_id))!=ESP_OK){// chưa có device id lưu trong nvs
             selected_keypad_id[0]='\0';
         }

         if (strcmp(device_id_item->valuestring,selected_keypad_id)==0){
            
                ESP_LOGI(TAG, "Received number: %s", number_item->valuestring);

                if (skip_item  && strcmp(skip_item->valuestring, "yes") == 0) {
                    ESP_LOGI(TAG, "Skip number");
                    skip_number(number_item->valuestring);  
                    
                }
                    
                else if (skip_item && strcmp(skip_item->valuestring, "no") == 0){
                    //checktime_stop=false;
                    
                    save_number(number_item->valuestring);
                }
         }
        }
        else {
            ESP_LOGW(TAG, "Invalid JSON fields in 'number' topic");
                    }

}

void handle_device_list(cJSON* root, mqtt_message_t msg ){
                ESP_LOGI(TAG, "Device list received");
                //parse_json_and_store(msg.data);   
                //sort_device_list_by_counter(device_count);//

                device_info_t new_list[MAX_DEVICES] = {0};
                int new_count = 0;

                if (parse_json_to_device_list(msg.data, new_list, &new_count) != ESP_OK) {
                    ESP_LOGE(TAG, "Parse device list failed");
                    //return;
                    cJSON_Delete(root);
                    //continue;
                    return;
                }
                build_new_list(new_list,new_count);//

                device_info_t old_list[MAX_DEVICES] = {0};
                int old_count = 0;

                esp_err_t err = load_device_list_from_nvs_to_buffer(old_list, &old_count);
                
                qsort(new_list, new_count, sizeof(device_info_t), device_compare_by_counter);
                qsort(old_list, old_count, sizeof(device_info_t), device_compare_by_counter);


                bool need_save = false;

                if (err != ESP_OK) {
                    ESP_LOGI(TAG, "No device list in NVS, save new list");
                    need_save = true;
                } else if (device_list_is_different(new_list, new_count,
                                                    old_list, old_count)) {
                    ESP_LOGI(TAG, "Device list changed, save new list");
                    need_save = true;
                } else {
                    ESP_LOGI(TAG, "Device list unchanged, skip save");
                }

                if (need_save) {
                    memcpy(device_list, new_list, sizeof(device_info_t) * new_count);
                    device_count = new_count;
                    //save_device_list_to_nvs();
                    save_device_list_to_nvs_from_buffer(new_list,new_count);//
                }

               // save_device_list_to_nvs();     
}

void handle_check_current_number(cJSON* root){
                cJSON *device_id_item = cJSON_GetObjectItem(root, "device_id");
                cJSON *number_item    = cJSON_GetObjectItem(root, "number");
                char current_num[5];
                size_t len=sizeof(current_num);
                read_number(current_num,len);
               
                ESP_LOGI(TAG, "check current number");
                if (strncmp(device_id_item->valuestring,selected_keypad_id,18)){
                ESP_LOGI(TAG, "current number of counter: %s",number_item->valuestring);
                //if (strcmp(msg.data,current_num)!=0){
                if (strcmp(number_item->valuestring,current_num)!=0){
                  
                   //save_current_number(msg.data);
                   save_current_number(number_item->valuestring);
                   delete_next_number();
                   ESP_LOGI(TAG, "Save new current number");


                }
            }
                else {
                    ESP_LOGI(TAG, "No new current number, skip saving");
                }
}
void mqtt_process_task(void *pvParameters)
{
    mqtt_message_t msg;

    while (1) {
        if (xQueueReceive(mqtt_queue, &msg, portMAX_DELAY)) {

            ESP_LOGI(TAG, "Processing topic: %s", msg.topic);
            ESP_LOGI(TAG, "Data: %s", msg.data);

            // parse JSON từ payload, chỉ parse payload kiểu json của 2 topic number, device/list và check current number
            cJSON *root = NULL;
 
            if (strcmp(msg.topic, "number") == 0 ||
                strcmp(msg.topic, "device/list") == 0||strcmp(msg.topic, "check current number") == 0
            ||strcmp(msg.topic, login_topic) == 0
            ) {

                root = cJSON_Parse(msg.data);
                if (!root) {
                    ESP_LOGE(TAG, "JSON parse failed");
                    continue;
                }
            }

/*
            // Xử lý topic "number"
            if (strcmp(msg.topic, "number") == 0) {
                handle_number_topic(root);//
               

            }
                */
            if  (strcmp(msg.topic, login_topic) == 0) {
               // cJSON *device_id_item = cJSON_GetObjectItem(root, "device_id");
                cJSON *status_item    = cJSON_GetObjectItem(root, "status");
                cJSON *id_item    = cJSON_GetObjectItem(root, "message");
               
                ESP_LOGI(TAG, "check user cred");

                if (!cJSON_IsString(status_item) || !cJSON_IsNumber(id_item)) {
                    ESP_LOGE(TAG, "Invalid login JSON");
                    cJSON_Delete(root);
                    continue;
                }

                if (strcmp(status_item->valuestring,"true")==0){
                  

                   ESP_LOGI(TAG, "successful");
                   login=true;
                   staff_id=id_item->valueint;
                   ESP_LOGI(TAG, "staff_id: %d",staff_id);
                   save_login_status("YES");
                   save_last_id(staff_id);
                   //snprintf(id_str,sizeof(id_str),"%d",staff_id);
                   //char str[128]={0};
                   //sprintf(str, "{\"device_id\":%d,\"status\":\"online\"}",staff_id);
                   //esp_mqtt_client_publish(mqttClient, "staff/status", str, 0, 1, 0);
                   /*
                   if (lvgl_port_lock(-1)) {

                    _ui_flag_modify(ui_Label14, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);//

                    _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_MOVE_LEFT, 50, 0, &ui_Screen1_screen_init);//
                     lvgl_port_unlock();
                   }                 
*/
                    ui_evt_t evt;
                     evt.type = UI_EVT_LOGIN_OK ;
                    xQueueSend(ui_queue, &evt, 0);



             //   }
            }
                else {
                    ESP_LOGI(TAG, "ERROR");
                    save_login_status("NO");
                    /*
                    if (lvgl_port_lock(-1)) {

                    _ui_flag_modify(ui_Label14, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
                    lvgl_port_unlock();
                   }
                    */
                   ui_evt_t evt;
                     evt  .type = UI_EVT_LOGIN_FAIL ;
                    xQueueSend(ui_queue, &evt, 0);

                }        

            }




/*
            else if (strcmp(msg.topic, "device/list") == 0) {
                handle_device_list(root,msg);
              
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
                handle_check_current_number(root);
                
             
            }
                */

            else {
                ESP_LOGI(TAG, "Unhandled topic: %s", msg.topic);
            }

            // Giải phóng bộ nhớ JSON
            if (root){
            cJSON_Delete(root);
            }
        }
    }
}



void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    //esp_mqtt_client_handle_t client = event->client;
    //int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");

        /*
        msg_id = esp_mqtt_client_publish(mqttClient, "feedback_status", "", 0, 1, 1);// xóa retained mess

        if (msg_id >= 0) {
            ESP_LOGI(TAG, "deleted retained message to keypad successfully, msg_id=%d",msg_id);
        } else {
            ESP_LOGW(TAG, "delete retained message to keypad failed!");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
*/
/*
        msg_id = esp_mqtt_client_publish(mqttClient, "feedback_status", "connected", 0, 1, 0);// gửi thông báo đến topic đã kết nối thành công

        if (msg_id >= 0) {
            ESP_LOGI(TAG, "Sent connection message to keypad successfully, msg_id=%d",msg_id);
        } else {
            ESP_LOGW(TAG, "Send connection message to keypad failed!");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
            */
        ui_evt_t evt;
        evt.type = UI_EVT_MQTT_CONNECTED ;
        xQueueSend(ui_queue, &evt, 0);

/*
        if (lvgl_port_lock(-1)) {
            lv_obj_add_flag(ui_Image16, LV_OBJ_FLAG_HIDDEN ); // ẩn icon lỗi kết nối
            lv_obj_add_flag(ui_Image26, LV_OBJ_FLAG_HIDDEN ); // ẩn icon lỗi kết nối
            
            lvgl_port_unlock();
        }
            */
        //char login_topic[32]={0};
        sprintf(login_topic,"%s/staff_id",device_mac);
        //msg_id = esp_mqtt_client_subscribe(client, "number", 0);
        char online_mess[128]={0};
        //msg_id = esp_mqtt_client_publish(mqttClient, login_topic, "connected", 0, 1, 0);
        //if (login){
        if (check_login_status()){  
                
            //snprintf(online_mess,sizeof(online_mess), "{\"staff_id\":%d,\"status\":\"online\",\"device_id\":\"%s\"}",staff_id,device_mac);
            snprintf(online_mess,sizeof(online_mess), "{\"staff_id\":%d,\"status\":\"online\",\"device_id\":\"%s\"}",read_last_id(),device_mac);
            esp_mqtt_client_publish(mqttClient, "staff/status", online_mess, 0, 1, 0);
            printf("%s",online_mess);
            ESP_LOGI("LOGIN","SEND CONNECTION MESS");
        }
        //esp_mqtt_client_subscribe(event->client, "number", 0);
        //esp_mqtt_client_subscribe(event->client, "device/list", 1);
        //esp_mqtt_client_subscribe(event->client, "check current number", 0);
        //esp_mqtt_client_subscribe(event->client, "reset_number", 0);
        //esp_mqtt_client_subscribe(event->client, "transfer_number", 0);
        esp_mqtt_client_subscribe(event->client, login_topic, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
        publish_backup_mqtt_msg(mqttClient);//



        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
       /* 
        if (lvgl_port_lock(-1)) {
        lv_obj_clear_flag(ui_Image16, LV_OBJ_FLAG_HIDDEN ); 
        lv_obj_clear_flag(ui_Image26, LV_OBJ_FLAG_HIDDEN ); 
        lvgl_port_unlock();
        }
        */
        evt.type = UI_EVT_MQTT_DISCONNECTED;

        xQueueSend(ui_queue, &evt, 0);
        
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
    char offline_mess[128]={0};
    snprintf(offline_mess,sizeof(offline_mess), "{\"staff_id\":%d,\"status\":\"offline\",\"device_id\":\"%s\"}",staff_id,device_mac);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {

            .address.port = 1885,
            .address.uri = "mqtt://10.10.1.21",
                      
              
        },
        //.network.timeout_ms = 10000,  
        .credentials = {
            .username = "appuser",
            .authentication.password = "1111",
        },

        .session = {
            .keepalive = 15,
            .disable_clean_session = false,
            .last_will.topic = "staff/status",

            .last_will.msg=offline_mess,
            .last_will.qos = 1,
            .last_will.retain = true,
        },

        .network.disable_auto_reconnect = false,
        
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttClient = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}



