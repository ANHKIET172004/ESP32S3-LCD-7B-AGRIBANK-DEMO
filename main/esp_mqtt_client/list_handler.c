#include "list_handler.h"

static const char *TAG = "MQTT_SAVE_NUMBER"; 

static int extract_counter_number(const char *name) {
    if (!name) return -1;

    const char *ptr=name;
    
   
    
    if (ptr) {
        //while (*ptr == ' ' || *ptr == ':' || *ptr == '-'||*ptr < '0' || *ptr > '9') {
        while (*ptr < '0' || *ptr > '9') {
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
        cJSON *id = cJSON_GetObjectItem(item, "id");   
        if (name && id && count < MAX_DEVICES&&cJSON_IsString(name) && (name->valuestring != NULL)
        &&cJSON_IsString(id) && id->valuestring)
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
