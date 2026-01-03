#include "ui.h"
#include "wifi.h"
#include "nvs.h"
#include "esp_wifi_types.h"
#include "mqtt_client.h"
#include "lvgl_port.h"

extern bool wifi_open;

 esp_netif_t *sta_netif;


bool connection_flag = false; // Flag to track if a connection was successfully made previously
bool connection_last_flag = false; // Flag to check if the STA is reconnecting while in AP mode
static esp_netif_ip_info_t ip_info; // Stores IP information


extern lv_obj_t *saved_wifi_button;
extern bool user_selected_wifi;

int reconnect=0;
extern int cnt;


//int start_api=0;

extern void api_task(void *pvParameters);
extern void wifi_update_list_cb(lv_timer_t * timer) ;

extern void mqtt_start(void);
extern esp_mqtt_client_handle_t mqttClient;

extern lv_obj_t * ui_Image16;

uint8_t s_retry_num=0;

//uint8_t disconnected_retry=15;

extern lv_obj_t *label_rescan;
extern lv_obj_t *ui_WIFI_Rescan_Button;



bool wifi_connected = false;
bool wifi_need_mqtt_start = false;




static inline bool lv_obj_valid_safe(lv_obj_t *obj)
{
    return (obj != NULL) && lv_obj_is_valid(obj);
}



//////////////// lưu ssid,pass và bssid

void save_wifi_credentials(const char *ssid, const char *password, const uint8_t* bssid)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_cred", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_STA, "Failed to open NVS handle!");
        return;
    }

    char old_ssid[32] = {0};
    char old_password[64] = {0};
    uint8_t old_bssid[6] = {0};
    size_t ssid_len = sizeof(old_ssid);
    size_t pass_len = sizeof(old_password);
    size_t bssid_len = sizeof(old_bssid);

    nvs_get_str(nvs_handle, "ssid", old_ssid, &ssid_len);
    nvs_get_str(nvs_handle, "password", old_password, &pass_len);
    nvs_get_blob(nvs_handle, "bssid", old_bssid, &bssid_len);

    bool ssid_update = false;
    bool pass_update = false;
    bool bssid_update = false;

    if (strcmp(ssid, old_ssid) != 0) {
        ssid_update = true;
    }
    if (strcmp(password, old_password) != 0) {
        pass_update = true;
    }
    if (memcmp(bssid, old_bssid, 6) != 0) {
        bssid_update = true;
    }

    if (!ssid_update&&!pass_update&&!bssid_update) {
        ESP_LOGI(TAG_STA, "Wi-Fi credentials unchanged, skip saving");
        nvs_close(nvs_handle);
        return;
    }
    

    ESP_LOGI(TAG_STA, "Updating Wi-Fi credentials in NVS...");
    

    nvs_set_str(nvs_handle, "ssid", ssid);
    nvs_set_str(nvs_handle, "password", password);
    nvs_set_blob(nvs_handle, "bssid", bssid, 6);

    err = nvs_commit(nvs_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG_STA, "Wi-Fi credentials saved successfully!");
    } else {
        ESP_LOGE(TAG_STA, "Failed to save Wi-Fi credentials!");
    }

    nvs_close(nvs_handle);
}



static void wifi_ok_cb(lv_timer_t * timer) 
{
    if (connection_flag)
    {    
        ESP_LOGI("NVS", "last index: %d", wifi_last_index);
        ESP_LOGI("NVS", "wifi index: %d", wifi_index);
        
        if (connection_last_flag)
        {    
            // Kiá»ƒm tra wifi_last_Button cÃ³ há»£p lá»‡ khÃ´ng
            if (lv_obj_valid_safe(wifi_last_Button)) {
                lv_obj_t *img = lv_obj_get_child(wifi_last_Button, 0);
                if (lv_obj_valid_safe(img)) {
                    lv_img_set_src(img, &ui_img_ok_png);
                    _ui_flag_modify(ui_WIFI_Details_Win, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
                    WIFI_CONNECTION = wifi_last_index;
                    connection_last_flag = false;
                } else {
                    ESP_LOGW("WIFI_UI", "Image child invalid in wifi_last_Button");
                }
            } else {
                ESP_LOGW("WIFI_UI", "wifi_last_Button invalid in wifi_ok_cb (connection_last_flag)");
            }
        }
        else
        {
            // Kiá»ƒm tra WIFI_List_Button cÃ³ há»£p lá»‡ khÃ´ng
            if (!lv_obj_valid_safe(WIFI_List_Button)) {
                ESP_LOGW("WIFI_UI", "WIFI_List_Button is invalid");
                return;
            }

            // Update icon vÃ  IP khi káº¿t ná»‘i thá»§ cÃ´ng
            char ip[20];
            lv_obj_t *img = lv_obj_get_child(WIFI_List_Button, 0);
            lv_obj_t *label = lv_obj_get_child(WIFI_List_Button, 1);
            
            // Kiá»ƒm tra img vÃ  label cÃ³ há»£p lá»‡ khÃ´ng
            if (lv_obj_valid_safe(img)) {
                lv_img_set_src(img, &ui_img_ok_png);
            } else {
                ESP_LOGW("WIFI_UI", "Image child invalid in WIFI_List_Button");
            }
            
            if (lv_obj_valid_safe(label)) {
                lv_label_set_text(label, (const char *)ap_info[wifi_index].ssid);
            } else {
                ESP_LOGW("WIFI_UI", "Label child invalid in WIFI_List_Button");
            }

            // Hiá»ƒn thá»‹ IP address
            snprintf(ip, sizeof(ip), "IP " IPSTR, IP2STR(&ip_info.ip));
            if (lv_obj_valid_safe(ui_WIFI_IP)) {
                lv_label_set_text(ui_WIFI_IP, ip);
                _ui_flag_modify(ui_WIFI_IP, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
            }

            lv_obj_move_to_index(WIFI_List_Button, 0);
            WIFI_CONNECTION_DONE = true;

            // Cáº­p nháº­t icon WiFi cÅ© náº¿u cÃ³
            if (wifi_last_index != -1 && wifi_last_index != wifi_index)
            {  
                if (lv_obj_valid_safe(wifi_last_Button)) {
                    img = lv_obj_get_child(wifi_last_Button, 0);
                    
                    if (lv_obj_valid_safe(img)) {
                        // Cáº­p nháº­t icon dá»±a trÃªn RSSI
                        if (ap_info[wifi_last_index].rssi > -25)
                            lv_img_set_src(img, &ui_img_wifi_4_png);
                        else if (ap_info[wifi_last_index].rssi > -50)
                            lv_img_set_src(img, &ui_img_wifi_3_png);
                        else if (ap_info[wifi_last_index].rssi > -75)
                            lv_img_set_src(img, &ui_img_wifi_2_png);
                        else 
                            lv_img_set_src(img, &ui_img_wifi_1_png);
                    } else {
                        ESP_LOGW("WIFI_UI", "Previous WiFi image invalid");
                    }
                } else {
                    ESP_LOGW("WIFI_UI", "wifi_last_Button is NULL, skip update");
                }
            }

            WIFI_CONNECTION = wifi_index;
            wifi_last_index = wifi_index;
            wifi_last_Button = WIFI_List_Button;
        } 
    }
    else
    {
        // Xá»­ lÃ½ khi káº¿t ná»‘i tháº¥t báº¡i
        if (lv_obj_valid_safe(ui_WIFI_IP)) {
            _ui_flag_modify(ui_WIFI_IP, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
        }
        
        if (lv_obj_valid_safe(ui_WIFI_PWD_Error)) {
            _ui_flag_modify(ui_WIFI_PWD_Error, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);

        }
/*
        if (user_manual==true){
            user_manual=false;
        }
            */
        
        WIFI_CONNECTION_DONE = true;

        if (wifi_last_index != -1 && wifi_last_index != wifi_index)
        {   
            if (lv_obj_valid_safe(wifi_last_Button)) {
                lv_obj_t *img = lv_obj_get_child(wifi_last_Button, 0);
                
                if (lv_obj_valid_safe(img)) {
                    if (ap_info[wifi_last_index].rssi > -25)
                        lv_img_set_src(img, &ui_img_wifi_4_png);
                    else if (ap_info[wifi_last_index].rssi > -50)
                        lv_img_set_src(img, &ui_img_wifi_3_png);
                    else if (ap_info[wifi_last_index].rssi > -75)
                        lv_img_set_src(img, &ui_img_wifi_2_png);
                    else 
                        lv_img_set_src(img, &ui_img_wifi_1_png);
                } else {
                    ESP_LOGW("WIFI_UI", "Image invalid in connection failure");
                }
            }
        }
        
        WIFI_CONNECTION = -1;
        wifi_last_index = wifi_index;
        wifi_last_Button = WIFI_List_Button;
    }
}



static void wif_disconnected_cb1()
{ 
    // Kiá»ƒm tra wifi_last_Button cÃ³ há»£p lá»‡ khÃ´ng
    if (!lv_obj_valid_safe(wifi_last_Button)) {
        ESP_LOGW("WIFI", "wifi_last_Button is NULL or invalid, skip UI update");
        return;
    }

    if (wifi_last_index != -1)
    {
        WIFI_CONNECTION = -1;
        lv_obj_t *img = lv_obj_get_child(wifi_last_Button, 0);
        
        // Kiá»ƒm tra img cÃ³ há»£p lá»‡ khÃ´ng
        if (lv_obj_valid_safe(img))
        {
            // Cáº­p nháº­t icon WiFi dá»±a trÃªn RSSI
            if (ap_info[wifi_last_index].rssi > -25)
                lv_img_set_src(img, &ui_img_wifi_4_png);
            else if (ap_info[wifi_last_index].rssi > -50)
                lv_img_set_src(img, &ui_img_wifi_3_png);
            else if (ap_info[wifi_last_index].rssi > -75)
                lv_img_set_src(img, &ui_img_wifi_2_png);
            else 
                lv_img_set_src(img, &ui_img_wifi_1_png);
        }
        else
        {
            ESP_LOGW("WIFI_UI", "Image child invalid in disconnected callback");
        }
        
        // áº¨n IP address náº¿u object há»£p lá»‡
        if (lv_obj_valid_safe(ui_WIFI_IP)) {
            _ui_flag_modify(ui_WIFI_IP, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
        }
    }
}

static void wif_disconnected_cb()
{ 
    // Kiá»ƒm tra wifi_last_Button cÃ³ há»£p lá»‡ khÃ´ng
    if (!lv_obj_valid_safe(wifi_last_Button)) {
        ESP_LOGW("WIFI", "wifi_last_Button is NULL or invalid, skip UI update");
        return;
    }

    if (wifi_last_index != -1)
    {
        WIFI_CONNECTION = -1;
        lv_obj_t *img = lv_obj_get_child(wifi_last_Button, 0);
        
        // Kiá»ƒm tra img cÃ³ há»£p lá»‡ khÃ´ng
        if (lv_obj_valid_safe(img))
        {
            // Cáº­p nháº­t icon WiFi dá»±a trÃªn RSSI
            if (ap_info[wifi_last_index].rssi > -25)
                lv_img_set_src(img, &ui_img_wifi_4_png);
            else if (ap_info[wifi_last_index].rssi > -50)
                lv_img_set_src(img, &ui_img_wifi_3_png);
            else if (ap_info[wifi_last_index].rssi > -75)
                lv_img_set_src(img, &ui_img_wifi_2_png);
            else 
                lv_img_set_src(img, &ui_img_wifi_1_png);
        }
        else
        {
            ESP_LOGW("WIFI_UI", "Image child invalid in disconnected callback");
        }
        
        // áº¨n IP address náº¿u object há»£p lá»‡
        if (lv_obj_valid_safe(ui_WIFI_IP)) {
            _ui_flag_modify(ui_WIFI_IP, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
        }
    }
}

static void wifi_ui_retry_start(void *param)
{
    //_ui_flag_modify(ui_WIFI_Spinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE); 
    _ui_flag_modify(ui_WIFI_Wait_CONNECTION, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);

    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_ADD);
    lv_obj_clear_flag(ui_WIFI_Rescan_Button, LV_OBJ_FLAG_CLICKABLE);
}

static void wifi_ui_retry_fail(void *param)
{
    //_ui_flag_modify(ui_WIFI_Spinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD); 
    _ui_flag_modify(ui_WIFI_Wait_CONNECTION, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD); 

    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
    lv_obj_add_flag(ui_WIFI_Rescan_Button, LV_OBJ_FLAG_CLICKABLE);
}

static void wifi_ui_retry_success(void *param)
{
    _ui_flag_modify(ui_WIFI_Spinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD); 
    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
    lv_obj_add_flag(ui_WIFI_Rescan_Button, LV_OBJ_FLAG_CLICKABLE);
}


// Event handler for Wi-Fi events
void wifi_event_handler1(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)

{    


    //wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;//
    wifi_config_t sta_config;

    // Get the current Wi-Fi station configuration
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &sta_config));


     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {    
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;//
        ESP_LOGI(TAG_STA, "STA disconnected, reason=%d", event->reason);
        ESP_LOGI(TAG_STA, "WIFI STA_DISCONNECTED.");
         wifi_connected=false;
       
          //  wifi_connected = false;
            
          //  wifi_need_mqtt_start = false;

            /*
            if  (disconnected<100){
            disconnected++;
            }
            else {
                disconnected=disconnected_retry+1;
            }
                */

          //  if (!user_selected_wifi&&(wifi_open==true)&&(disconnected<disconnected_retry)){

                
                 if (!user_selected_wifi&&(wifi_open==true)){

                    

                    if (s_retry_num < 10) {
                        if (s_retry_num==0){
                            lv_async_call(wifi_ui_retry_start, NULL);

                        /*
                        _ui_flag_modify(ui_WIFI_Spinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE); 
                        _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_ADD);
                        lv_obj_clear_flag(ui_WIFI_Rescan_Button, LV_OBJ_FLAG_CLICKABLE);
                        */

                        }
                        esp_wifi_connect();
                        s_retry_num++;
                        ESP_LOGW("rety", "Wi-Fi disconnected, retrying... (%d/%d)", s_retry_num, 10);
                    } 
                    else if (s_retry_num==10) {
                        ESP_LOGE("retry", "Wi-Fi failed to reconnect after %d retries", 10);
                        s_retry_num++;
                        //s_retry_num=0;
                        /*
                        _ui_flag_modify(ui_WIFI_Spinner, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD); 
                         _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);

                        lv_obj_add_flag(ui_WIFI_Rescan_Button, LV_OBJ_FLAG_CLICKABLE);
                        */
                        lv_async_call(wifi_ui_retry_fail, NULL);


                }
        }
                        
      //  }
        /*
        else if(!user_selected_wifi&&(wifi_open==true)&&(disconnected==disconnected_retry)){
               // lv_async_call(wifi_ui_retry_success, NULL);
                  ESP_LOGE("retry", "Wi-Fi failed to reconnect after %d retries", disconnected_retry);
                //  s_retry_num=0;     
                  lv_async_call(wifi_ui_retry_fail, NULL);
        }
                  */

            


        lv_timer_t *t = lv_timer_create(wif_disconnected_cb, 100, NULL); // Create a timer to handle disconnection
        lv_timer_set_repeat_count(t, 1);  // Run the callback once
              
    } 
    
   
    
      else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_need_mqtt_start = true; 
        wifi_connected=true;
        //disconnected=0;
        ESP_LOGI(TAG_STA, "WIFI GOT_IP.");  

        //wifi_need_mqtt_start = true; 

        if (user_selected_wifi==true){
        user_selected_wifi=false;
        save_wifi_credentials((char*)sta_config.sta.ssid,(char*)sta_config.sta.password,sta_config.sta.bssid);// lưu ssid, password và của wifi kết nối thành công vào nvs

        }
       
        ////
        //s_retry_num=0;
        //lv_async_call(wifi_ui_retry_success, NULL);
///
          
          
         if ( (user_selected_wifi==false)&&(wifi_open==true)){
             s_retry_num=0;
             lv_async_call(wifi_ui_retry_success, NULL);           
             
         }


        }

        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *) event_data;
        ESP_LOGI(TAG_STA, "Connected to SSID:%s, channel:%d", event->ssid, event->channel);


    }

     
  


}

 void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)

{    


    wifi_config_t sta_config;

    // Get the current Wi-Fi station configuration
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &sta_config));


     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {    
        wifi_connected=false;
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;//
        ESP_LOGI(TAG_STA, "STA disconnected, reason=%d", event->reason);
        ESP_LOGI(TAG_STA, "WIFI STA_DISCONNECTED.");

        if (!user_selected_wifi&&(wifi_open==true)){

            if (s_retry_num < 10) {
                if (s_retry_num==0){
                    lv_async_call(wifi_ui_retry_start, NULL);


                }
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGW("rety", "Wi-Fi disconnected, retrying... (%d/%d)", s_retry_num, 10);
            } 
            else {
                ESP_LOGE("retry", "Wi-Fi failed to reconnect after %d retries", 10);
                //s_retry_num=0;

                lv_async_call(wifi_ui_retry_fail, NULL);


        }
        }

        lv_timer_t *t = lv_timer_create(wif_disconnected_cb, 100, NULL); // Create a timer to handle disconnection
        lv_timer_set_repeat_count(t, 1);  // Run the callback once

        
    } 
    
    
      else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        //wifi_need_mqtt_start=true; 
        wifi_connected=true;

        ESP_LOGI(TAG_STA, "WIFI GOT_IP."); 
        wifi_need_mqtt_start=true; 
        //found_saved_ap=false;//

        if (user_selected_wifi==true){
            user_selected_wifi=false;
            save_wifi_credentials((char*)sta_config.sta.ssid,(char*)sta_config.sta.password,sta_config.sta.bssid);// lÆ°u ssid, password vÃ  cá»§a wifi káº¿t ná»‘i thÃ nh cÃ´ng vÃ o nvs

        }

        //mqtt_start();
         if ( (user_selected_wifi==false)&&(wifi_open==true)){
             s_retry_num=0;

            lv_async_call(wifi_ui_retry_success, NULL); 

                }

        }


         else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *) event_data;
        ESP_LOGI(TAG_STA, "Connected to SSID:%s, channel:%d", event->ssid, event->channel);
         // if (user_manual==true){
           //  user_manual=false;
   //}
         }

}

// Start Wi-Fi event handling
void start_wifi_events() {
    // Register the Wi-Fi event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    printf("Wi-Fi event handler registered.\n");
}


void wifi_wait_connect()
{
    static int s_retry_num = 0;  // Counter to track the number of connection retries
    wifi_config_t sta_config;

    // Get the current Wi-Fi station configuration
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &sta_config));

    // Log the current SSID and password (disabled by default)
    ESP_LOGI(TAG_STA, "GET: SSID:%s, password:%s", sta_config.sta.ssid, sta_config.sta.password);

    while (1)
    {
        // Get the network interface handle for the default Wi-Fi STA interface
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif)
        {   
            vTaskDelay(pdMS_TO_TICKS(500));//
            // Get the IP information associated with the Wi-Fi STA interface
            esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
            if (ret == ESP_OK && ip_info.ip.addr != 0) {

                ////
                char ap_info_bssid[18];
                wifi_ap_record_t ap_info;
                esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
                if (err==ESP_OK){
                    snprintf(ap_info_bssid,sizeof(ap_info_bssid),"%02X:%02X:%02X:%02X:%02X:%02X",ap_info.bssid[0],ap_info.bssid[1]
                    ,ap_info.bssid[2],ap_info.bssid[3],ap_info.bssid[4],ap_info.bssid[5]);
                }
                else {
                    ESP_LOGI("wifi:","Failed to get the ap_info");
                }
                

                ////
                // If the IP is valid, call the Wi-Fi success callback and update the UI
                lv_timer_t *t = lv_timer_create(wifi_ok_cb, 100, NULL);  // Update UI every 100ms
                lv_timer_set_repeat_count(t, 1);
                // Log the obtained IP address and indicate successful connection
                ESP_LOGI("WiFi", "Connected with IP: " IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG_STA, "Connected to AP SSID:%s, password:%s, bssid:%s", sta_config.sta.ssid, sta_config.sta.password,(uint8_t*)ap_info_bssid);
                connection_flag = true;  // Set the connection flag to true
                //wifi_connected = true;//
                //wifi_need_mqtt_start = true; //
                _ui_flag_modify(ui_WIFI_PWD_Error, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);//
                if ((cnt==1)&&(reconnect==0)){// chip khởi động lại hoặc switch wifi on
                    reconnect=1;// đánh dấu đã reconnect thành công
                }
                /*
                if (user_selected_wifi){// nếu connect bằng cách nhập pass thì lưu pass, ngược lại ko lưu
                  user_selected_wifi=false;
                 // save_wifi_credentials((char*)sta_config.sta.ssid,(char*)sta_config.sta.password,ap_info.bssid);// lưu ssid, password và của wifi kết nối thành công vào nvs
                //  save_wifi_credentials((char*)sta_config.sta.ssid,(char*)sta_config.sta.password,sta_config.sta.bssid);
                  ESP_LOGI(TAG_STA,"Saved AP SSID:%s, password:%s, bssid:%s",sta_config.sta.ssid, sta_config.sta.password,(uint8_t*)ap_info_bssid);
           }  
                  */

              //  s_retry_num = 0;  // Reset retry counter on successful connection

                break;  // Exit the loop since the connection is successful
            } else {
                // Log the failure to connect or obtain an IP address
                ESP_LOGI(TAG_STA, "Failed to connect to the AP");

                // Retry connection if the retry counter is less than 5
                if (s_retry_num < 5)
                {
                    s_retry_num++;
                    ESP_LOGI(TAG_STA, "Retrying to connect to the AP");
                    //esp_mqtt_client_stop(mqttClient);//
                }
                else {
                    // Reset retry counter after 5 attempts and log the failure
                    s_retry_num = 0;
                    lv_timer_t *t = lv_timer_create(wifi_ok_cb, 100, NULL);  // Update the UI every 100ms
                    lv_timer_set_repeat_count(t, 1);
                    ESP_LOGI(TAG_STA, "Failed to connect to SSID:%s, password:%s",
                            sta_config.sta.ssid, sta_config.sta.password);
                     

                 if (user_selected_wifi){// nếu connect bằng cách nhập pass thì lưu pass, ngược lại ko lưu
                  user_selected_wifi=false;

             }  
                ////
                    break;  // Exit the loop after failed retries
                }

                // Wait for 1 second before retrying
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        else 
        {
            // Log an error if the network interface handle is not found
            ESP_LOGE("WiFi", "Netif handle not found");

        }

        // Short delay (10ms) before checking the connection status again
        vTaskDelay(pdMS_TO_TICKS(10));
    }
};




void wifi_sta_init(uint8_t *ssid, uint8_t *pwd, wifi_auth_mode_t authmode,  const uint8_t *bssid)
{   
       
     


    if (connection_flag)  // Check if already connected
    {
        printf("esp_wifi_disconnect \r\n");  // Log disconnection
        esp_wifi_disconnect();  // Disconnect from the current WiFi
        vTaskDelay(pdMS_TO_TICKS(500));//
        connection_flag = false;  // Reset connection flag
        //wifi_need_mqtt_stop=true;//

    }



    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));//

    wifi_config_t wifi_config = {              
        .sta = {                                
            .threshold.authmode = authmode,
            .scan_method = WIFI_FAST_SCAN,


        },                                      
    };


    
    memset(wifi_config.sta.ssid, 0, sizeof(wifi_config.sta.ssid));//
    memset(wifi_config.sta.password, 0, sizeof(wifi_config.sta.password));//
    memset(wifi_config.sta.bssid, 0, sizeof(wifi_config.sta.password));//

    strcpy((char *)wifi_config.sta.ssid, (const char *)ssid);

    strcpy((char *)wifi_config.sta.password, (const char *)pwd);

    memcpy(wifi_config.sta.bssid, bssid, 6); 
    wifi_config.sta.bssid_set = true;  
        
        
    ESP_LOGI(TAG_STA, "Connecting to wifi network, BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
                 bssid[0], bssid[1], bssid[2],
                bssid[3],bssid[4], bssid[5]);
                
    /////////
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);

    if (mode != WIFI_MODE_STA) {
        esp_wifi_stop();                      // Dá»«ng Wi-Fi náº¿u Ä‘ang á»Ÿ cháº¿ Ä‘á»™ khÃ¡c
        esp_wifi_set_mode(WIFI_MODE_STA);     // Chuyá»ƒn sang STA
        esp_wifi_start();                     // Khá»Ÿi Ä‘á»™ng láº¡i Wi-Fi
    }
    ///////////


    // Set WiFi configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_stop();  // Stop WiFi module

    esp_wifi_start(); // Start WiFi with the new configuration
    ESP_ERROR_CHECK(esp_wifi_connect());  // Attempt to connect to the WiFi
    wifi_wait_connect();  // Wait for the connection to establish
    //是否需要阻塞，会有部分wifi无法连接上
    while (!WIFI_CONNECTION_DONE)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    WIFI_CONNECTION_DONE = false;
     


    
}


void wifi_open_sta()
{
   
   if (connection_flag==false){
    ESP_ERROR_CHECK(esp_wifi_stop());  // Stop WiFi if it's running
   }

    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));  // Get the current WiFi mode

    // If the mode is AP+STA (Access Point and Station mode)
    if (mode == WIFI_MODE_AP)
    {
        // Switch to STA only mode
        printf("STA:STA + AP \r\n");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));  // Set mode to AP+STA
    }
    else
    {
        printf("STA:STA \r\n");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));  // Set mode to STA only
    }

    if (sta_netif == NULL)
    {
        sta_netif = esp_netif_create_default_wifi_sta();  // Create default STA network interface
        assert(sta_netif);  // Ensure the network interface was created successfully
    }

    ESP_ERROR_CHECK(esp_wifi_start());  // Start WiFi in the new mode
}


void wifi_close_sta()
{
    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));  // Get current WiFi mode

    // If not in AP+STA mode
    if (mode != WIFI_MODE_APSTA)
    {
        if (connection_flag)  // If already connected
        {
            //stop_wifi_events();  // Optional: Stop WiFi events if implemented
            WIFI_CONNECTION = -1;
            wifi_last_index = -1;
            esp_wifi_disconnect();  // Disconnect from WiFi
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));  // Set mode to NULL (no WiFi)
            ESP_ERROR_CHECK(esp_wifi_stop());  // Stop WiFi
            connection_flag = false;  // Reset connection flag
        }
        else
        {
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));  // Set mode to NULL if not connected
            ESP_ERROR_CHECK(esp_wifi_stop());  // Stop WiFi
        }       
    }
    else
    {
        if (connection_flag)  // If connected in AP+STA mode
        {
            WIFI_CONNECTION = -1;
            wifi_last_index = -1;
            //stop_wifi_events();  // Optional: Stop WiFi events if implemented//
            esp_wifi_disconnect();  // Disconnect from WiFi
            ESP_ERROR_CHECK(esp_wifi_stop());  // Stop WiFi
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));  // Set mode to AP only
            ESP_ERROR_CHECK(esp_wifi_start());  // Restart WiFi in AP mode
            connection_flag = false;  // Reset connection flag
        }
        else
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));  // Set mode to AP if not connected
    }
    
    // Destroy the STA network interface if it exists
    if (sta_netif != NULL)
    {
        esp_netif_destroy(sta_netif);  // Destroy the network interface
        sta_netif = NULL;  // Prevent repeated destruction
    }
}

void wifi_set_default_netif()
{
    /* Set sta as the default interface */
    esp_netif_set_default_netif(sta_netif);  // Set the STA network interface as the default
}


void wifi_mqtt_manager_task(void *pv)
{
    while (1)
    {  
        
        // Nếu có lệnh start
        if (wifi_need_mqtt_start)
        {
            wifi_need_mqtt_start = false;
            if (mqttClient)
            {
                ESP_LOGI("MQTT_MANAGER", "Starting MQTT safely...");
                //esp_mqtt_client_start(mqttClient);
                esp_mqtt_client_reconnect(mqttClient);
                ESP_LOGI("MQTT_MANAGER", "MQTT started.");
            }
            else
            {
                ESP_LOGI("MQTT_MANAGER", "No existing client, creating new one...");
                mqtt_start();  
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 200 tránh chiếm CPU
    }
}
