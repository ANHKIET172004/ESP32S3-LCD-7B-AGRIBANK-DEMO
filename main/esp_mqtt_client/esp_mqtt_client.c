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


extern char selected_device_id[18];

char selected_keypad_id[18]={0};


extern volatile bool checktime_stop;

static int extract_counter_number(const char *name) {
    if (!name) return -1;

    const char *ptr=name;
    
   
    
    if (ptr) {
        while (*ptr == ' ' || *ptr == ':' || *ptr == '-'||*ptr < '0' || *ptr > '9') {
            ptr++;
        }
        
        if (*ptr >= '0' && *ptr <= '9') {
            int num = atoi(ptr);
            if (num >= 1 && num <= 50) {
                return num;
            }
        }
    }
    
    return -1; 
}



static void sort_device_list_by_counter1(int count)
{
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {

            int c1 = atoi(device_list[i].name);
            int c2 = atoi(device_list[j].name);

        

            if (c1 > c2) {
                device_info_t   temp = device_list[i];
                device_list[i] = device_list[j];
                device_list[j] = temp;
            }
        }
    }
}

static void sort_device_list_by_counter(int count)
{
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {

            int c1=extract_counter_number(device_list[j].name);
            int c2=extract_counter_number(device_list[j+1].name);


            //int c1 = atoi(device_list[j].name);
            //int c2 = atoi(device_list[j + 1].name);
            char a[3];
            char b[3];
            sprintf(a,"%d",c1);
            sprintf(b,"%d",c2);
            strncpy(device_list[j].name,a,sizeof(device_list[j].name));
            strncpy(device_list[j+1].name,b,sizeof(device_list[j+1].name));


            if (c1 > c2) {
                device_info_t temp = device_list[j];
                device_list[j] = device_list[j + 1];
                device_list[j + 1] = temp;
            }
        }
    }
}




void save_number(const char *number)
{
    if (number == NULL) {
        ESP_LOGE(TAG, "Invalid number (NULL pointer)");
        return;
    }

    if (strlen(number) >= 16) {
        ESP_LOGE(TAG, "Number too long (max 15 chars)");
        return;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't open NVS: %s", esp_err_to_name(err));
        return;
    }

    char temp[16];
    size_t required_size = sizeof(temp);
    bool has_current = false;
    bool has_next = false;

    //  Kiểm tra current_number 
    err = nvs_get_str(nvs_handle, "current_number", temp, &required_size);
    if (err == ESP_OK) {
        has_current = true;
    } else if (err == ESP_ERR_NVS_INVALID_LENGTH) {
        ESP_LOGW(TAG, "current_number too long -> erasing");
        nvs_erase_key(nvs_handle, "current_number");
    }

    // Kiểm tra next_number 
    required_size = sizeof(temp);
    err = nvs_get_str(nvs_handle, "next_number", temp, &required_size);
    if (err == ESP_OK) {
        has_next = true;
    } else if (err == ESP_ERR_NVS_INVALID_LENGTH) {
        ESP_LOGW(TAG, "next_number too long -> erasing");
        nvs_erase_key(nvs_handle, "next_number");
    }

    const char *key_to_use = NULL;

    if (!has_current) {
        key_to_use = "current_number";
    } else if (!has_next) {
        key_to_use = "next_number";
       // xSemaphoreGive(check_sema);//
    } else {
        // Nếu cả hai đều tồn tại, chỉ cập nhật next_number
        key_to_use = "next_number";
    }

    // Ghi dữ liệu 
    err = nvs_set_str(nvs_handle, key_to_use, number);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Saved number \"%s\" into key \"%s\"", number, key_to_use);
            if (strcmp(key_to_use, "next_number") == 0 ) {
                /*
            if (lvgl_port_lock(-1)){
            lv_textarea_set_placeholder_text(ui_TextArea3,number);
            lvgl_port_unlock();
            }
            */
               xSemaphoreGive(check_sema); 
             }
             else {
                /*
                if (lvgl_port_lock(-1)){
            lv_textarea_set_placeholder_text(ui_TextArea2,number);
            lvgl_port_unlock();
            }  
            */
              // xSemaphoreGive(check_sema); 
             }
        } else {
            ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed (%s): %s", key_to_use, esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}

void skip_number(const char* number)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't open NVS: %s", esp_err_to_name(err));
        return;
    }

    char temp[16];
    size_t required_size;
    bool has_current = false;
    bool has_next = false;

    // Kiểm tra current_number 
    required_size = sizeof(temp);
    err = nvs_get_str(nvs_handle, "current_number", temp, &required_size);
    if (err == ESP_OK) {
        has_current = true;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No current_number found");
    } else if (err == ESP_ERR_NVS_INVALID_LENGTH) {
        ESP_LOGW(TAG, "current_number invalid length -> treating as exist");
        has_current = true;
    } else {
        ESP_LOGE(TAG, "Error reading current_number: %s", esp_err_to_name(err));
    }

    //  Kiểm tra next_number 
    required_size = sizeof(temp);
    err = nvs_get_str(nvs_handle, "next_number", temp, &required_size);
    if (err == ESP_OK) {
        has_next = true;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No next_number found");
    } else if (err == ESP_ERR_NVS_INVALID_LENGTH) {
        ESP_LOGW(TAG, "next_number invalid length -> treating as exist");
        has_next = true;
    } else {
        ESP_LOGE(TAG, "Error reading next_number: %s", esp_err_to_name(err));
    }

    //  Xử lý theo logic 
    if (has_current && has_next) {
        //ESP_LOGI(TAG, "Both current_number and next_number exist -> skip doing anything");
        ESP_LOGI(TAG, "Both current_number and next_number exist, skip next number");
        ////////
err = nvs_set_str(nvs_handle, "next_number", number);
    
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Saved number \"%s\" into key next number successfully", number);
            /*
            if (lvgl_port_lock(-1)){
            lv_textarea_set_placeholder_text(ui_TextArea3, number);
            lvgl_port_unlock();
            }
            */
            
        } else {
            ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed : %s", esp_err_to_name(err));
    }




        //////////
    } 
    else if (has_current && !has_next) {
        ESP_LOGW(TAG, "Only current_number exists -> deleting it");

        err = nvs_erase_key(nvs_handle, "current_number");
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Deleted current_number successfully");
            /*
            if (lvgl_port_lock(-1)){
            lv_textarea_set_placeholder_text(ui_TextArea2, "0000");
            lvgl_port_unlock();
            }
            */
        } else {
            ESP_LOGE(TAG, "Failed to delete current_number: %s", esp_err_to_name(err));
        }

        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Commit failed after erase: %s", esp_err_to_name(err));
        }
        /////////
err = nvs_set_str(nvs_handle, "current_number", number);
    
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Saved number \"%s\" into key current number successfully", number);
            /*
            if (lvgl_port_lock(-1)){
            lv_textarea_set_placeholder_text(ui_TextArea3, number);
            lvgl_port_unlock();
            }
            */
            
        } else {
            ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed : %s", esp_err_to_name(err));
    }




        /////////
    } 
    else {
        ESP_LOGI(TAG, "No valid number to skip");
    }

    nvs_close(nvs_handle);
}



void save_current_number(const char* number){
        if (number == NULL) {
        ESP_LOGE(TAG, "Invalid number (NULL pointer)");
        return;
    }

    if (strlen(number) >= 16) {
        ESP_LOGE(TAG, "Number too long (max 15 chars)");
        return;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't open NVS: %s", esp_err_to_name(err));
        return;
    }

    char temp[16];
    //size_t required_size = sizeof(temp);

     err = nvs_set_str(nvs_handle, "current_number", number);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Saved number \"%s\" into key current number successfully", number);
            /*
            if (lvgl_port_lock(-1)){
            lv_textarea_set_placeholder_text(ui_TextArea2, number);
            lvgl_port_unlock();
            }
            */
        } else {
            ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed : %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);

}

void save_next_number(const char* number){
        if (number == NULL) {
        ESP_LOGE(TAG, "Invalid number (NULL pointer)");
        return;
    }

    if (strlen(number) >= 16) {
        ESP_LOGE(TAG, "Number too long (max 15 chars)");
        return;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't open NVS: %s", esp_err_to_name(err));
        return;
    }

    char temp[16];
    //size_t required_size = sizeof(temp);

     err = nvs_set_str(nvs_handle, "next_number", number);
    
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Saved number \"%s\" into key next number successfully", number);
            /*
            if (lvgl_port_lock(-1)){
            lv_textarea_set_placeholder_text(ui_TextArea3, number);
            lvgl_port_unlock();
            }
            */
            
        } else {
            ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed : %s", esp_err_to_name(err));
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
    err = nvs_get_str(nvs_handle, "current_number", NULL, &required_size);// number
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get size for number: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    if (required_size > max_len) {
        ESP_LOGE(TAG, "Buffer too small: required %zu bytes, provided %d", required_size, max_len);
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_SIZE;
    }

    err = nvs_get_str(nvs_handle, "current_number", number, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read number: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
        

    ESP_LOGI(TAG, "Read number: %s", number);
    nvs_close(nvs_handle);
    return ESP_OK;
}


esp_err_t read_next_number(char *number, size_t max_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "next_number", NULL, &required_size);// number
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get size for number: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    if (required_size > max_len) {
        ESP_LOGE(TAG, "Buffer too small: required %zu bytes, provided %d", required_size, max_len);
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_SIZE;
    }

    err = nvs_get_str(nvs_handle, "next_number", number, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read number: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "Read number: %s", number);
    nvs_close(nvs_handle);
    return ESP_OK;
}


esp_err_t delete_current_number(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_key(nvs_handle, "current_number");
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "current_number does not exist in NVS");
        nvs_close(nvs_handle);
        return ESP_OK;  
    }
    else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase key: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit erase: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "current_number deleted successfully");
    nvs_close(nvs_handle);
    /*
    if (lvgl_port_lock(-1)){
    lv_textarea_set_placeholder_text(ui_TextArea2, "0000");
    lvgl_port_unlock();
    }
    */
    return ESP_OK;
}

esp_err_t delete_next_number(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_key(nvs_handle, "next_number");
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "next_number does not exist in NVS");
        nvs_close(nvs_handle);
        return ESP_OK;  
    }
    else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase key: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit erase: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "next_number deleted successfully");
    /*
    if (lvgl_port_lock(-1)){
    lv_textarea_set_placeholder_text(ui_TextArea3, "0000");
    lvgl_port_unlock();
    }
    */
    nvs_close(nvs_handle);
    return ESP_OK;
}


esp_err_t transfer_number(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_key(nvs_handle, "next_number");
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "next_number does not exist in NVS");
        delete_current_number();
        nvs_close(nvs_handle);
        return ESP_OK;  
    }
    else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase key: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit erase: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "next_number deleted successfully");
    /*
    if (lvgl_port_lock(-1)){
    lv_textarea_set_placeholder_text(ui_TextArea3, "0000");
    lvgl_port_unlock();
    }
    */
    nvs_close(nvs_handle);
    return ESP_OK;
}




void reset_recent_number(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't open NVS: %s", esp_err_to_name(err));
        return;
    }

    char next_number[16];
    size_t required_size = sizeof(next_number);
    err = nvs_get_str(nvs_handle, "next_number", next_number, &required_size);
    if (err == ESP_OK) {
        err = nvs_set_str(nvs_handle, "current_number", next_number);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "copied next_number (%s) to current_number", next_number);

            /*
        if (lvgl_port_lock(-1)){
        lv_textarea_set_placeholder_text(ui_TextArea2, next_number);
        lvgl_port_unlock();
        }
        */
        
        } else {
            ESP_LOGE(TAG, "Can't save current_number: %s", esp_err_to_name(err));
        }
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, " No next_number exist");

    } else {
        ESP_LOGE(TAG, "Read next_number failed: %s", esp_err_to_name(err));
    }


    
    err = nvs_erase_key(nvs_handle, "next_number");
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Deleted key next_number successfully");
        /*
         if (lvgl_port_lock(-1)){
            lv_textarea_set_placeholder_text(ui_TextArea3, "0000");
            lvgl_port_unlock();
            }
            */
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No Key next_number exist");
        delete_current_number();//
    } else {
        ESP_LOGE(TAG, "Can't delete next_number: %s", esp_err_to_name(err));
    }
        

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't commit : %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}



void save_timeout(const uint8_t time){
    /*
        if (time == NULL) {
        ESP_LOGE(TAG, "Invalid number ");
        return;
    }
*/
   

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_TIME", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't open NVS: %s", esp_err_to_name(err));
        return;
    }

   // char temp[16];
   // size_t required_size = sizeof(temp);

     err=nvs_set_u8(nvs_handle,"timeout",time);
    
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Saved \"%d\" into key timeout successfully", time);
            
        } else {
            ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed : %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);


    static uint8_t tmp;
    read_time(&tmp);
    static char str[10];
    sprintf(str,"%d",tmp);
    
    //lv_textarea_set_placeholder_text(ui_TextArea4, "Placeholder...");
    
    
    if (lvgl_port_lock(-1)){
    lv_textarea_set_placeholder_text(ui_TextArea4, str);
    lvgl_port_unlock();
    }
    

}


esp_err_t read_time(uint8_t* time)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_TIME", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    //size_t required_size = 0;
    err=nvs_get_u8(nvs_handle,"timeout",time);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get size for number: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

   
    ESP_LOGI(TAG, "Saved timeout: %u", *time);
    nvs_close(nvs_handle);
    return ESP_OK;
}


void parse_json_and_store(const char *json_data)
{
    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse error");
        return;
    }

    if (!cJSON_IsArray(root)) {
        ESP_LOGE(TAG, "Root is not an array");
        cJSON_Delete(root);
        return;
    }

    cJSON *item = NULL;
    uint8_t count = 0;

    cJSON_ArrayForEach(item, root)
    {
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *id = cJSON_GetObjectItem(item, "id");   // sửa đúng key
        if (name && id && count < MAX_DEVICES)
        {
            strncpy(device_list[count].name, name->valuestring, sizeof(device_list[count].name) - 1);
            strncpy(device_list[count].device_id, id->valuestring, sizeof(device_list[count].device_id) - 1);
            count++;
        }
    }

    device_count = count;   // cập nhật số lượng thiết bị trong device list
    cJSON_Delete(root);
    ESP_LOGI(TAG, "Stored %d devices", device_count);
}


///////// mqtt
void save_device_list_to_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("DEVICE_LIST", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("TAG_NVS", "Failed to open NVS: %s", esp_err_to_name(err));
        return;
    }
    ///
    //ESP_LOGI("TAG_NVS", "Loaded %d devices from NVS", device_count);
    sort_device_list_by_counter(device_count);//
    ESP_LOGI("TAG_NVS", "sorted device list successfully");
    //save_device_list_to_nvs();
    ///

    // Táº¡o JSON array chá»©a danh sÃ¡ch thiáº¿t bá»‹
    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < device_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", device_list[i].name);
        cJSON_AddStringToObject(item, "device_id", device_list[i].device_id);
        cJSON_AddItemToArray(root, item);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        err = nvs_set_str(nvs_handle, "device_list", json_str);
        if (err == ESP_OK) {
            nvs_commit(nvs_handle);
            ESP_LOGI("TAG_NVS", "Saved %d devices to NVS", device_count);
        } else {
            ESP_LOGE("TAG_NVS", "Failed to save JSON: %s", esp_err_to_name(err));
        }
        free(json_str);
    }

    cJSON_Delete(root);
    nvs_close(nvs_handle);
}

esp_err_t load_device_list_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("DEVICE_LIST", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW("TAG_NVS", "No saved device list found");
        return err;
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "device_list", NULL, &required_size);
    if (err != ESP_OK || required_size == 0) {
        ESP_LOGW("TAG_NVS", "No data for device_list_json");
        nvs_close(nvs_handle);
        return err;
    }

    char *json_str = malloc(required_size);
    if (!json_str) {
        ESP_LOGE("TAG_NVS", "Failed to allocate memory");
        nvs_close(nvs_handle);
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_str(nvs_handle, "device_list", json_str, &required_size);
    nvs_close(nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE("TAG_NVS", "Failed to read NVS data: %s", esp_err_to_name(err));
        free(json_str);
        return err;
    }

    ESP_LOGI("TAG_NVS", "Loaded JSON from NVS: %s", json_str);

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root || !cJSON_IsArray(root)) {
        ESP_LOGE("TAG_NVS", "Invalid JSON structure");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    memset(device_list, 0, sizeof(device_list));
    int count = 0;
    cJSON *item = NULL;

    cJSON_ArrayForEach(item, root) {
        if (count >= MAX_DEVICES) break;
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *id = cJSON_GetObjectItem(item, "device_id");
        if (cJSON_IsString(name) && cJSON_IsString(id)) {
            strncpy(device_list[count].name, name->valuestring, MAX_NAME_LEN - 1);
            strncpy(device_list[count].device_id, id->valuestring, MAX_ID_LEN - 1);
            count++;
        }
    }

    device_count = count;

    cJSON_Delete(root);



    return ESP_OK;
}

esp_err_t load_selected_device_id(char *id_buf, size_t buf_size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_DEVICE_ID", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("LOAD_DEVICE_ID", "Cannot open NVS: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = buf_size;
    err = nvs_get_str(nvs_handle, "device_id", id_buf, &required_size);
    if (err == ESP_OK) {
        ESP_LOGI("LOAD_DEVICE_ID", "Loaded device_id: %s", id_buf);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW("LOAD_DEVICE_ID", "No device_id saved");
    } else {
        ESP_LOGE("LOAD_DEVICE_ID", "Get failed: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}




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

        /*
         .session = {
            //.keepalive = 60,
            .keepalive = 15,
            .disable_clean_session = false,
            .last_will.topic = "feedback_status",
            //.last_will.msg = "{\"device_id\":\"04:1A:2B:3C:4D:04\",\"status\":\"offline\"}",
            //.last_will.msg = "{\"device_id\":\"A8:42:E3:4C:7C:BC\",\"status\":\"offline\"}",
            .last_will.msg="disconnected",
            .last_will.qos = 1,
            .last_will.retain = true,
        },
        */

        .network.disable_auto_reconnect = false,
        
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttClient = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}



