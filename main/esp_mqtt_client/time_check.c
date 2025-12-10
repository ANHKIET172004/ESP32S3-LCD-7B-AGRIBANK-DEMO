#include "time_check.h"

static char* TAG ="MQTT_SAVE_NUMBER";

extern lv_obj_t * ui_TextArea4 ;


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