#include "ui.h"
#include "wifi.h"

#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "wifi_scan";

wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];

extern void wifi_connection_cb(lv_timer_t *timer);//
//extern lv_obj_t *wifi_last_Button;

//////////////////
char saved_ssid[32] = {0};
char saved_password[64] = {0};;
//uint8_t saved_bssid[18];
uint8_t saved_bssid[6];
size_t ssid_len = sizeof(saved_ssid);
size_t password_len = sizeof(saved_password);
bool found_saved_ap = false;

int found=-1;

 lv_obj_t *saved_wifi_button = NULL;

//extern int8_t wifi_last_index;

// Cấu trúc để truyền tham số an toàn 
typedef struct {
    char ssid[32];
    char password[64];
    uint8_t bssid[6];
    wifi_auth_mode_t authmode;
    bool use_bssid;
} wifi_reconnect_params_t;

//int cnt=0;
 lv_obj_t *wifi_last_Button = NULL;   //
 int8_t wifi_last_index;//
/////
//static lv_obj_t *wifi_buttons[DEFAULT_SCAN_LIST_SIZE];//

/*
static void clear_wifi_list(lv_obj_t *list) {
    while (lv_obj_get_child_cnt(list) > 0) {
        lv_obj_t *child = lv_obj_get_child(list, 0);
        lv_obj_del(child);
    }
}
*/

void wifi_set_last_button(lv_obj_t *btn) {
    wifi_last_Button = btn;
}

lv_obj_t* wifi_get_last_button(void) {
    return wifi_last_Button;
}

void wifi_set_last_index(int idx) {
    wifi_last_index = idx;
}


//////////




// Hàm callback để cập nhật UI sau khi kết nối thành công
/*
static void update_wifi_ui_cb(lv_timer_t *timer) {
    
    wifi_reconnect_params_t *params = (wifi_reconnect_params_t *)timer->user_data;
    if (connection_flag&&found_saved_ap) {
        found_saved_ap=false;
        //ESP_LOGI(TAG, "Kết nối Wi-Fi thành công: %s", params->ssid);
        
          if (wifi_last_Button) {
    
                //lv_obj_t *img = lv_obj_get_child(WIFI_List_Button, 0);  // Get the image object  //
                lv_obj_t *img = lv_obj_get_child(wifi_last_Button, 0);  // Get the image object  //
                lv_img_set_src(img, &ui_img_ok_png);  // Set success icon  //
          }


    } else {
        ESP_LOGE(TAG, "Kết nối Wi-Fi thất bại: %s", params->ssid);
    }

    //    lv_obj_t *btn = wifi_get_last_button(); // lấy đúng button đã lưu
       if (btn) {
            
            //lv_obj_t *img = lv_obj_get_child(btn, 0);
            lv_obj_t  *img = lv_obj_get_child(saved_wifi_button, 0);//

            lv_img_set_src(img, &ui_img_ok_png);
      }

    //free(params); // Giải phóng bộ nhớ
    lv_timer_del(timer); // Xóa timer
}


}
*/

void wifi_reconnect_task(void *pvParameters) {
    wifi_reconnect_params_t *params = (wifi_reconnect_params_t *)pvParameters;
    
    ESP_LOGI(TAG, "Connecting to SSID: %s ...", params->ssid);
    

    wifi_sta_init((uint8_t*)params->ssid, 
                  (uint8_t*)params->password, 
                  params->authmode,
                  params->use_bssid ? params->bssid : NULL);


   // _ui_flag_modify(ui_WIFI_Spinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    

    
    // Tạo timer để cập nhật UI trên luồng chính của LVGL
    //lv_timer_t *timer = lv_timer_create(update_wifi_ui_cb, 100, params);
    //lv_timer_t *timer = lv_timer_create(update_wifi_ui_cb, 100, NULL);

    //lv_timer_set_repeat_count(timer, 1);
       // Giải phóng memory sau khi sử dụng
    free(params); 
    // Xóa task
    vTaskDelete(NULL);
}

// Hàm helper để tạo task kết nối lại
void start_wifi_reconnect_task(const char *ssid, const char *password, 
                              const uint8_t *bssid, wifi_auth_mode_t authmode) {
    // Cấp phát memory cho tham số
    wifi_reconnect_params_t *params = malloc(sizeof(wifi_reconnect_params_t));
    if (params == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for wifi reconnect params");
        return;
    }
    
    // Copy dữ liệu vào struct
    strncpy(params->ssid, ssid, sizeof(params->ssid) - 1);
    params->ssid[sizeof(params->ssid) - 1] = '\0';
    
    strncpy(params->password, password, sizeof(params->password) - 1);
    params->password[sizeof(params->password) - 1] = '\0';
    
    params->authmode = authmode;
    
    if (bssid != NULL) {
        memcpy(params->bssid, bssid, 6);
        params->use_bssid = true;
    } else {
        params->use_bssid = false;
    }
    
    // Tạo task
   // BaseType_t result = xTaskCreate(wifi_reconnect_task, "wifi_reconnect_task", 4096, params, 5, NULL);
     xTaskCreate(wifi_reconnect_task, "wifi_reconnect_task", 4096, params, 5, NULL);
  
 /*   
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create wifi reconnect task");
        free(params);
    }
     */   
         
}



esp_err_t read_wifi_credentials_from_nvs(char *ssid, size_t *ssid_len, char *password, size_t *password_len,uint8_t* bssid) {
    nvs_handle_t my_handle;
    esp_err_t err;
    err = nvs_open("wifi_cred", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_str(my_handle, "ssid", ssid, ssid_len);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return err;
    }
    err = nvs_get_str(my_handle, "password", password, password_len);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return err;
    }
    size_t bssid_len = 6;
    err = nvs_get_blob(my_handle, "bssid", bssid, &bssid_len);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return err;
    }
    nvs_close(my_handle);
    return ESP_OK;
}



//////////

void print_bssid(wifi_ap_record_t ap_info){
         ESP_LOGI(TAG, "BSSID %02X:%02X:%02X:%02X:%02X:%02X",ap_info.bssid[0]
            ,ap_info.bssid[1],ap_info.bssid[2],ap_info.bssid[3],ap_info.bssid[4],ap_info.bssid[5]);

}


////////////



void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
        lv_label_set_text(ui_WIFI_Aurhmode,"Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_OWE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OWE");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_OWE");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_ENTERPRISE");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA3_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_ENTERPRISE");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_WPA3_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_ENTERPRISE");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_WPA2_WPA3_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_ENT_192:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_ENT_192");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_WPA3_ENT_192");
        break;
    default:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        lv_label_set_text(ui_WIFI_Aurhmode, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_AES_CMAC128");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_SMS4");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        lv_label_set_text(ui_WIFI_Pairwise, "Authmode \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_SMS4");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        lv_label_set_text(ui_WIFI_Group, "Authmode \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}




static void wifi_update_list_cb(lv_timer_t * timer) {

      //  clear_wifi_list(ui_WIFI_SCAN_List);
/////////////////
           // đọc ssid và password của wifi đã lưu trước đó
          esp_err_t err = read_wifi_credentials_from_nvs(saved_ssid, &ssid_len, saved_password, &password_len,saved_bssid);
          if (err==ESP_OK){
            ESP_LOGI("NVS", "có SSID đã lưu trong nvs");
            
        }

////////////////

    // Show the loading spinner while updating the Wi-Fi list
    _ui_flag_modify(ui_WIFI_Spinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    // Disable the Wi-Fi open button and AP open button while scanning
    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
    _ui_state_modify(ui_WIFI_AP_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
    
   // wifi_last_index = -1;//
    // Iterate through the scanned AP list
    for (int i = 0; i < DEFAULT_SCAN_LIST_SIZE; i++)
    {
        // If no more valid APs in the list, stop
        if (ap_info[i].rssi == 0 && ap_info[i].ssid[0] == '\0')
        {
            break;
        }
        else if(ap_info[i].rssi > -25)  // Strong signal (RSSI > -25)
        {
            // Add button with strong signal icon
            WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_4_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info[i].rssi < -25) && (ap_info[i].rssi > -50))  // Medium signal
        {
            // Add button with medium signal icon
            WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_3_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info[i].rssi < -50) && (ap_info[i].rssi > -75))  // Weak signal
        {
            // Add button with weak signal icon
            WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_2_png, (const char *)ap_info[i].ssid);
        }
        else  // Very weak signal (RSSI < -75)
        {
            // Add button with very weak signal icon
            WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_1_png, (const char *)ap_info[i].ssid);
        }
   
        
        // Customize button appearance
        lv_obj_set_style_bg_opa(WIFI_List_Button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);  // Set background opacity to 0 (transparent)
        lv_obj_set_style_text_color(WIFI_List_Button, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);  // Set text color to white
        lv_obj_set_style_text_opa(WIFI_List_Button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);  // Set text opacity to full (255)

        // Add event callback for each button
        lv_obj_add_event_cb(WIFI_List_Button, ui_WIFI_list_event_cb, LV_EVENT_ALL, (void *)i);  // Pass index as user data

          ////////////
       // wifi_buttons[i] = lv_list_add_btn(ui_WIFI_SCAN_List, icon, (const char *)ap_info[i].ssid);

        //if (strcmp((const char *)ap_info[i].ssid, saved_ssid) == 0) {
        //lv_obj_t *btn = lv_obj_get_child(ui_WIFI_SCAN_List, i);  // i là index của AP trong ap_info


        if (memcmp(ap_info[i].bssid, saved_bssid, 6) == 0) {   //
                ESP_LOGI(TAG, "" );
                ESP_LOGI(TAG, "Found saved wifi network in scan list, ssid: %s",(const char *)ap_info[i].ssid );
                ESP_LOGI(TAG, "BSSID %02X:%02X:%02X:%02X:%02X:%02X",ap_info[i].bssid[0],ap_info[i].bssid[1]
                    ,ap_info[i].bssid[2],ap_info[i].bssid[3],ap_info[i].bssid[4],ap_info[i].bssid[5]);
                ESP_LOGI(TAG, "Saved BSSID %02X:%02X:%02X:%02X:%02X:%02X",saved_bssid[0],saved_bssid[1],saved_bssid[2],saved_bssid[3],saved_bssid[4],saved_bssid[5]);

                found_saved_ap = true;
                 
               wifi_set_last_button(WIFI_List_Button);
               // saved_wifi_button = WIFI_List_Button; 
               // saved_wifi_button = btn; 
                 //wifi_set_last_button(btn);
                // saved_wifi_button=WIFI_List_Button;  

                // WIFI_CONNECTION = wifi_index;  // Update connection index
                // wifi_last_index = i;  // Save the current Wi-Fi index

                wifi_set_last_index(i);
                
                 //wifi_last_Button = wifi_get_last_button();
                
                
                 wifi_index=i;
                 WIFI_STA_FLAG = true;//
               

             
              

              

            }
            else {
                  ESP_LOGI(TAG, "" );
                  ESP_LOGI(TAG, "BSSID %02X:%02X:%02X:%02X:%02X:%02X",ap_info[i].bssid[0],ap_info[i].bssid[1],ap_info[i].bssid[2],ap_info[i].bssid[3],ap_info[i].bssid[4],ap_info[i].bssid[5]);
                  ESP_LOGI(TAG, "Not saved wifi network, ssid:%s",(const char *)ap_info[i].ssid );
                  ESP_LOGI(TAG, "Saved BSSID %02X:%02X:%02X:%02X:%02X:%02X",saved_bssid[0],saved_bssid[1],saved_bssid[2],saved_bssid[3],saved_bssid[4],saved_bssid[5]);

            }


        ///////////
    }
///////////////
if (found_saved_ap){
   WIFI_List_Button = wifi_get_last_button();
}
    //////////////
}

/* Initialize Wi-Fi as STA (Station mode) and set the scan method */
void wifi_scan(void)
{
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;  // Maximum number of APs to scan for
    uint16_t ap_count = 0;  // Variable to hold the number of found APs

    // Clear the ap_info array to hold fresh scan results
    memset(ap_info, 0, sizeof(ap_info));

    //esp_wifi_set_mode(WIFI_MODE_STA);//
///////////
    wifi_mode_t mode;
    if (esp_wifi_get_mode(&mode) != ESP_OK || mode != WIFI_MODE_STA) {
        ESP_LOGI(TAG, "Đặt chế độ Wi-Fi thành STA");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    }
    ////////

    // Start the Wi-Fi scan
    esp_wifi_scan_start(NULL, true);

    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));  // Get the number of scanned APs
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));  // Get the scan results (AP records)
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

    // Create a timer to update the UI with the scanned Wi-Fi list
    lv_timer_t *t = lv_timer_create(wifi_update_list_cb, 100, NULL); // Update the UI every 100ms
    lv_timer_set_repeat_count(t, 1);

    // Delay to allow UI update time
    vTaskDelay(pdMS_TO_TICKS(100));
}
