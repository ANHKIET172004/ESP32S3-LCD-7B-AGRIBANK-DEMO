#include "action.h"

static char* TAG ="MQTT_SAVE_NUMBER";

extern SemaphoreHandle_t check_sema;

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

               xSemaphoreGive(check_sema); 
             }
             else {
                
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
