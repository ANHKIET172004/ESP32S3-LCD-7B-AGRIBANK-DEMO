#include "ui.h"
#include "wifi.h"

#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "wifi_scan";

wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];

extern void wifi_connection_cb(lv_timer_t *timer);//


//////////////////
char saved_ssid[32] = {0};
char saved_password[64] = {0};;

uint8_t saved_bssid[6];
size_t ssid_len = sizeof(saved_ssid);
size_t password_len = sizeof(saved_password);
bool found_saved_ap = false;

int found=-1;

 lv_obj_t *saved_wifi_button = NULL;

 int cnt=0;

extern int reconnect;
int refresh=0;

// Cấu trúc để truyền tham số an toàn 
typedef struct {
    char ssid[32];
    char password[64];
    uint8_t bssid[6];
    wifi_auth_mode_t authmode;
    bool use_bssid;
} wifi_reconnect_params_t;

 lv_obj_t *wifi_last_Button = NULL;   //
 int8_t wifi_last_index=-1;//

 lv_obj_t *wifi_refresh_Button = NULL;   //

 extern int connect_success;

 int refresh_index=-1;

extern bool wifi_connected;

void wifi_set_last_button(lv_obj_t *btn) {
    wifi_last_Button = btn;
}

lv_obj_t* wifi_get_last_button(void) {
    return wifi_last_Button;
}

void wifi_set_refresh_button(lv_obj_t *btn) {
    wifi_refresh_Button = btn;
}
lv_obj_t* wifi_get_refresh_button(void) {
    return wifi_refresh_Button;
}

void wifi_set_last_index(int idx) {
    wifi_last_index = idx;
}

void wifi_set_wifi_index(int idx) {
    wifi_index = idx;
}

int wifi_get_wifi_index(void) {
    return wifi_index;
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



void print_bssid(wifi_ap_record_t ap_info){
         ESP_LOGI(TAG, "BSSID %02X:%02X:%02X:%02X:%02X:%02X",ap_info.bssid[0]
            ,ap_info.bssid[1],ap_info.bssid[2],ap_info.bssid[3],ap_info.bssid[4],ap_info.bssid[5]);

}


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




//static void wifi_update_list_cb(lv_timer_t * timer) {
 void wifi_update_list_cb(lv_timer_t * timer) {

           // đọc ssid và password của wifi đã lưu trước đó
          esp_err_t err = read_wifi_credentials_from_nvs(saved_ssid, &ssid_len, saved_password, &password_len,saved_bssid);
          if (err==ESP_OK){
            ESP_LOGI("NVS", "Have a saved wifi network in NVS");
            
        }

       // lv_obj_clean(ui_WIFI_SCAN_List);//


      
    // Show the loading spinner while updating the Wi-Fi list
    _ui_flag_modify(ui_WIFI_Spinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    // Disable the Wi-Fi open button and AP open button while scanning
    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
    _ui_state_modify(ui_WIFI_AP_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
        
    
   
    // Iterate through the scanned AP list
    for (int i = 0; i < DEFAULT_SCAN_LIST_SIZE; i++)
    {
        // If no more valid APs in the list, stop
        if (ap_info[i].rssi == 0 && ap_info[i].ssid[0] == '\0')
        {
            break;
        }

        

   //     if (i == wifi_last_index && WIFI_CONNECTION != -1) {
         //   WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_ok_png, (const char *)ap_info[i].ssid);
         //   lv_obj_move_to_index(WIFI_List_Button, 0);

    //    } else {
            // Nếu không phải AP đang kết nối → hiển thị icon theo RSSI
            if (ap_info[i].rssi > -25) { // Strong signal (RSSI > -25)
                WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_4_png, (const char *)ap_info[i].ssid);
            } else if (ap_info[i].rssi > -50) {// Medium signal
                WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_3_png, (const char *)ap_info[i].ssid);
            } else if (ap_info[i].rssi > -75) { // Weak signal
                WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_2_png, (const char *)ap_info[i].ssid);
            } else {// Very weak signal
                WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_1_png, (const char *)ap_info[i].ssid);
            }
     //   }    
   
        
        // Customize button appearance
        lv_obj_set_style_bg_opa(WIFI_List_Button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);  // Set background opacity to 0 (transparent)
        lv_obj_set_style_text_color(WIFI_List_Button, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);  // Set text color to white
        lv_obj_set_style_text_opa(WIFI_List_Button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);  // Set text opacity to full (255)
        
        
        
        // Add event callback for each button
     //   lv_obj_add_event_cb(WIFI_List_Button, ui_WIFI_list_event_cb, LV_EVENT_ALL, (void *)i);  // Pass index as user data

          ////////////

    if (strcmp((const char *)ap_info[i].ssid,(const char *) saved_ssid) == 0) {//

        if (memcmp((const uint8_t*)ap_info[i].bssid,(const uint8_t*) saved_bssid, 6) == 0) {   //
                ESP_LOGI(TAG, "" );
                ESP_LOGI(TAG, "Found saved wifi network in scan list, ssid: %s",(const char *)ap_info[i].ssid );
                ESP_LOGI(TAG, "BSSID %02X:%02X:%02X:%02X:%02X:%02X",ap_info[i].bssid[0],ap_info[i].bssid[1]
                    ,ap_info[i].bssid[2],ap_info[i].bssid[3],ap_info[i].bssid[4],ap_info[i].bssid[5]);
                //ESP_LOGI(TAG, "Saved BSSID %02X:%02X:%02X:%02X:%02X:%02X",saved_bssid[0],saved_bssid[1],saved_bssid[2],saved_bssid[3],saved_bssid[4],saved_bssid[5]);

                found_saved_ap = true;
                 
                wifi_set_last_button(WIFI_List_Button);
                wifi_set_last_index(i);
                 wifi_index=i;
                 
                 if (cnt<10){
                    cnt++;//
                 }
                 else {
                    cnt=2;
                 }
                    
                 
                 if (cnt<2){// chỉ reconnect khi reset/ switch wifi
                    WIFI_STA_FLAG = true;
                 }//

                 else {// cnt>1, đã chuyển từ main screen sang wifi screen sau khi reconnect và switch wifi đang on
                  //  if ((reconnect==1)&&(refresh==0)&&(connection_flag==true)&&(connect_success==1)){// nếu đã reconnect wifi cũ thành công thì lần scan tiếp theo chỉ cập nhật icon ok cho button list của wifi được connect
                     // if ((reconnect==1)&&(refresh==0)&&(connection_flag==true)){
                        if ((reconnect==1)&&(refresh==0)&&(wifi_connected==true)){
                        
                        reconnect=0;
                   
                       WIFI_CONNECTION = i;//
                      lv_obj_t *img = lv_obj_get_child(WIFI_List_Button, 0);
                      
                      lv_img_set_src(img, &ui_img_ok_png);  // Set success icon
                     
                      lv_obj_move_to_index(WIFI_List_Button, 0);//

                    if(refresh<10){
                      refresh++;
                    }
                    else {
                        refresh=1;
                    }
                        
                     
                    }
                    //else if ((refresh>0)&&(connection_flag==true)&&(connect_success==1)){//
                 //else if ((refresh>0)&&(connection_flag==true)){
                  //  else if ((refresh>0)&&(connection_flag==true)){
                        else if ((refresh>0)&&(wifi_connected==true)){
                      WIFI_CONNECTION = i;//
                      lv_obj_t *img = lv_obj_get_child(WIFI_List_Button, 0);
                      lv_img_set_src(img, &ui_img_ok_png);  // Set success icon
                      
                      lv_obj_move_to_index(WIFI_List_Button, 0);//
                      
                     
                      refresh++;
                      

                    }
                 }//

              

            }
            /*
            else {
                  ESP_LOGI(TAG, "" );
                  ESP_LOGI(TAG, "BSSID %02X:%02X:%02X:%02X:%02X:%02X",ap_info[i].bssid[0],ap_info[i].bssid[1],ap_info[i].bssid[2],ap_info[i].bssid[3],ap_info[i].bssid[4],ap_info[i].bssid[5]);
                  ESP_LOGI(TAG, "Not saved wifi network, ssid:%s",(const char *)ap_info[i].ssid );
                  ESP_LOGI(TAG, "Saved BSSID %02X:%02X:%02X:%02X:%02X:%02X",saved_bssid[0],saved_bssid[1],saved_bssid[2],saved_bssid[3],saved_bssid[4],saved_bssid[5]);

            }
                  */
                  
        }
        /*
        else {
                  ESP_LOGI(TAG, "" );
                  ESP_LOGI(TAG, "BSSID %02X:%02X:%02X:%02X:%02X:%02X",ap_info[i].bssid[0],ap_info[i].bssid[1],ap_info[i].bssid[2],ap_info[i].bssid[3],ap_info[i].bssid[4],ap_info[i].bssid[5]);
                  ESP_LOGI(TAG, "Not saved wifi network, ssid:%s",(const char *)ap_info[i].ssid );
                  ESP_LOGI(TAG, "Saved BSSID %02X:%02X:%02X:%02X:%02X:%02X",saved_bssid[0],saved_bssid[1],saved_bssid[2],saved_bssid[3],saved_bssid[4],saved_bssid[5]);

            }
                  */

         lv_obj_add_event_cb(WIFI_List_Button, ui_WIFI_list_event_cb, LV_EVENT_ALL, (void *)i);  // Pass index as user data//
        
    }

     

if (found_saved_ap){
   WIFI_List_Button = wifi_get_last_button();
   wifi_index=wifi_get_wifi_index();
}


   



   
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
    esp_wifi_scan_start(NULL, true);// blocking

    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
    // chỉ có thể dùng get ap num và get ap records khi set true cho esp_wifi_scan_start(blocking)
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));  // Get the number of scanned APs/ số AP thật sự quét được
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));  // Get the scan results (AP records)
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

    // Create a timer to update the UI with the scanned Wi-Fi list
    lv_timer_t *t = lv_timer_create(wifi_update_list_cb, 100, NULL); // Update the UI every 100ms
    lv_timer_set_repeat_count(t, 1);// chỉ update 1 lần
    

    // Delay to allow UI update time
    vTaskDelay(pdMS_TO_TICKS(100));
}
