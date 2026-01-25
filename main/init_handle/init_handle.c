#include "init_handle.h"

static uint8_t max_retry=5;
static uint8_t counter=0;

esp_err_t save_retry(uint8_t x){

    nvs_handle_t handle;
    uint8_t cnt;

    esp_err_t err=nvs_open("ESP_RESTART",NVS_READWRITE,&handle);

    if (err!=ESP_OK){
        return err;

    }

    char id[10];
    snprintf(id,sizeof(id),"retry%d",x);
    err=nvs_get_u8(handle,id,&cnt);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        cnt = 0;
    } else if (err != ESP_OK) {
        ESP_LOGE("NVS_LOG","CAN'T OPEN ESP_RESTART SPACE");

        nvs_close(handle);
        return err;
    }
    err=nvs_set_u8(handle,id,cnt+1);
    if (err!=ESP_OK){
         ESP_LOGE("NVS_LOG","CAN'T SAVE RETRY"); 

    }
    else {
        ESP_LOGI("NVS_LOG","SAVE RETRY SUCCESSFULLY");
    }
    nvs_commit(handle);
    nvs_close(handle);

    return err;

}

uint8_t read_retry(uint8_t x){

    nvs_handle_t handle;
    uint8_t cnt;

    esp_err_t err=nvs_open("ESP_RESTART",NVS_READONLY,&handle);

    if (err!=ESP_OK){
        ESP_LOGE("NVS_LOG","CAN'T OPEN ESP_RESTART SPACE");
        return 0;

    }
    char id[10];
    snprintf(id,sizeof(id),"retry%d",x);
    err=nvs_get_u8(handle,id,&cnt);

    if (err!=ESP_OK){
        nvs_close(handle);
        return 0;
    }
    else {
        nvs_close(handle);
        return cnt;
    }

}

void reset_retry(uint8_t x){
    nvs_handle_t handle;
    uint8_t cnt;
    esp_err_t err=nvs_open("ESP_RESTART",NVS_READWRITE,&handle);

    if (err!=ESP_OK){       
        ESP_LOGE("NVS_LOG","CAN'T OPEN ESP_RESTART SPACE");
        return;
     
    }
    char id[10];
    snprintf(id,sizeof(id),"retry%d",x);

    err=nvs_set_u8(handle,id,0);

    if (err!=ESP_OK){
        ESP_LOGE("NVS_LOG","CAN'T RESET RETRY");
        nvs_close(handle);

    }
    else {
        ESP_LOGI("NVS_LOG","RESET RETRY SUCCESSFULLY");
        nvs_commit(handle);
        nvs_close(handle);
    }

}

void init_fail_hanlde(uint8_t x){
         counter=read_retry(x);//  
         if (counter<max_retry){
            ESP_LOGE("MAIN","INIT %d FAILED, RESTART %d/%d",x,counter,max_retry);
            save_retry(x);// tăng giá trị số lần retry trong nvs lên 1 đơn vị
            vTaskDelay(pdMS_TO_TICKS(1000));// chờ 1s trước khi restart
            esp_restart();
         }
         else {
            ESP_LOGE("MAIN","INIT %d FAILED AFTER %d RETRIES",x,max_retry);

            while (1){// cho treo chương trình tại đây khi restart 5 lần vẫn lỗi khởi tạo
              vTaskDelay(pdMS_TO_TICKS(1000));
            }
         }
}