#include "ui.h"
#include "wifi.h"
#include "nvs.h"
#include "esp_wifi_types.h"
#include "mqtt_client.h"
#include "lvgl_port.h"

extern void trigger_mqtt_retry();

static esp_netif_t *sta_netif;


bool connection_flag = false; // Flag to track if a connection was successfully made previously
bool connection_last_flag = false; // Flag to check if the STA is reconnecting while in AP mode
static esp_netif_ip_info_t ip_info; // Stores IP information

extern char saved_ssid[32] ;
extern char saved_password[64];

extern bool found_saved_ap;



extern lv_obj_t *saved_wifi_button;
extern bool user_selected_wifi;

int reconnect=0;
extern int cnt;


int start_api=0;

extern void api_task(void *pvParameters);
extern void wifi_update_list_cb(lv_timer_t * timer) ;

extern void mqtt_start(void);
extern esp_mqtt_client_handle_t mqttClient;

extern lv_obj_t * ui_Image38;

 extern bool rssi_task_running;


extern  void start_rssi_task(void);
extern void stop_rssi_task(void);
extern bool is_rssi_task_running(void);

extern esp_err_t mqtt_retry_task_init(void);

 bool wifi_connected = false;
 bool wifi_need_mqtt_start = false;
 bool wifi_need_mqtt_stop = false;

 static uint32_t last_time = 0;
static bool is_waiting = false;




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

    //bool need_update = false;
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
    
    /*
    nvs_erase_key(nvs_handle, "ssid");
    nvs_erase_key(nvs_handle, "password");
    nvs_erase_key(nvs_handle, "bssid");
    */
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





// Callback function triggered when Wi-Fi connection is successful
static void wifi_ok_cb(lv_timer_t * timer) 
{
    if (connection_flag)
    {    

       ESP_LOGI("NVS", "last index: %d",wifi_last_index);//
       ESP_LOGI("NVS", "wifi index: %d",wifi_index);//
        if (connection_last_flag)
        {    

            
             //Update the icon and IP display when connected automatically
             lv_obj_t *img = lv_obj_get_child(wifi_last_Button, 0);
             lv_img_set_src(img, &ui_img_ok_png);  // Set success icon
             _ui_flag_modify(ui_WIFI_Details_Win, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);  // Hide details window
             WIFI_CONNECTION = wifi_last_index;  // Save the connection index
             connection_last_flag = false;  // Reset the reconnect flag

        }
        
        else
        {
            // Update the icon and IP display when manually connecting
            char ip[20];
            lv_obj_t *img = lv_obj_get_child(WIFI_List_Button, 0);  // Get the image object
            lv_obj_t *label = lv_obj_get_child(WIFI_List_Button, 1); // Get the label object
            lv_img_set_src(img, &ui_img_ok_png);  // Set success icon
            lv_label_set_text(label, (const char *)ap_info[wifi_index].ssid);  // Display the SSID

            // Format the IP address and display it
            snprintf(ip, sizeof(ip), "IP " IPSTR, IP2STR(&ip_info.ip));
            lv_label_set_text(ui_WIFI_IP, ip);  // Display IP address
            _ui_flag_modify(ui_WIFI_IP, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);  // Show the IP address
            lv_obj_move_to_index(WIFI_List_Button, 0);//

            WIFI_CONNECTION_DONE = true; 
            if (wifi_last_index != -1 && wifi_last_index != wifi_index)
            {  


                // Update the Wi-Fi signal icon based on RSSI
                img = lv_obj_get_child(wifi_last_Button, 0);


               // if(img != NULL)
                if(img && lv_obj_is_valid(img)) //

                {
                    if(ap_info[wifi_last_index].rssi > -25)
                        lv_img_set_src(img, &ui_img_wifi_4_png); // Strong signal
                    else if ((ap_info[wifi_last_index].rssi < -25) && (ap_info[wifi_last_index].rssi > -50))
                        lv_img_set_src(img, &ui_img_wifi_3_png); // Moderate signal
                    else if ((ap_info[wifi_last_index].rssi < -50) && (ap_info[wifi_last_index].rssi > -75))
                        lv_img_set_src(img, &ui_img_wifi_2_png); // Weak signal
                    else 
                        lv_img_set_src(img, &ui_img_wifi_1_png); // Very weak signal
                }
                else
                    printf("1Invalid image source\n");
            }

            WIFI_CONNECTION = wifi_index;  // Update connection index
            wifi_last_index = wifi_index;  // Save the current Wi-Fi index
            wifi_last_Button = WIFI_List_Button;  // Update the last Wi-Fi button object

            //update_main_screen_wifi_status();//
            



        } 
    }
    else
    {
        // Hide IP address and show password error if connection fails
        _ui_flag_modify(ui_WIFI_IP, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
        _ui_flag_modify(ui_WIFI_PWD_Error, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
        WIFI_CONNECTION_DONE = true; 
        if (wifi_last_index != -1 && wifi_last_index != wifi_index)
        {   
            
            // Update the Wi-Fi signal icon based on RSSI for the last connection
            lv_obj_t *img = lv_obj_get_child(wifi_last_Button, 0);
            if(img != NULL)
            {
                if(ap_info[wifi_last_index].rssi > -25)
                    lv_img_set_src(img, &ui_img_wifi_4_png); // Strong signal
                else if ((ap_info[wifi_last_index].rssi < -25) && (ap_info[wifi_last_index].rssi > -50))
                    lv_img_set_src(img, &ui_img_wifi_3_png); // Moderate signal
                else if ((ap_info[wifi_last_index].rssi < -50) && (ap_info[wifi_last_index].rssi > -75))
                    lv_img_set_src(img, &ui_img_wifi_2_png); // Weak signal
                else 
                    lv_img_set_src(img, &ui_img_wifi_1_png); // Very weak signal
            }
            else
                printf("2Invalid image source\n");
        }
        WIFI_CONNECTION = -1;  // Reset Wi-Fi connection
        wifi_last_index = wifi_index;  // Update the last Wi-Fi index
        wifi_last_Button = WIFI_List_Button;  // Update the last Wi-Fi button object
    }
}

// Callback for Wi-Fi disconnection event
static void wif_disconnected_cb()
{ 

    //vTaskDelay(pdMS_TO_TICKS(2000));//
    if (wifi_last_index != -1)
    {
        WIFI_CONNECTION = -1;  // Reset Wi-Fi connection
        lv_obj_t *img = lv_obj_get_child(wifi_last_Button, 0);  // Get the image object
        if(img != NULL)
        {
            // Update the Wi-Fi signal icon based on RSSI for the last connection
            if(ap_info[wifi_last_index].rssi > -25)
                lv_img_set_src(img, &ui_img_wifi_4_png); // Strong signal
            else if ((ap_info[wifi_last_index].rssi < -25) && (ap_info[wifi_last_index].rssi > -50))
                lv_img_set_src(img, &ui_img_wifi_3_png); // Moderate signal
            else if ((ap_info[wifi_last_index].rssi < -50) && (ap_info[wifi_last_index].rssi > -75))
                lv_img_set_src(img, &ui_img_wifi_2_png); // Weak signal
            else 
                lv_img_set_src(img, &ui_img_wifi_1_png); // Very weak signal
        }
        else
            printf("3Invalid image source\n");
        _ui_flag_modify(ui_WIFI_IP, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);  // Hide IP address
        //update_main_screen_wifi_status();//
    }
}

// Event handler for Wi-Fi events
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
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
       
      /*
        if (mqttClient) {
            esp_mqtt_client_stop(mqttClient);
        }

        if (!user_selected_wifi){
            esp_wifi_connect() ;
        }
            */

            wifi_connected = false;
            wifi_need_mqtt_stop = true;  
            
            
            
         //  esp_mqtt_client_stop(mqttClient);
     //    esp_mqtt_client_disconnect(mqttClient);

  //          stop_rssi_task();
          
       //    esp_mqtt_client_stop(mqttClient);
       
       /*

        if (lvgl_port_lock(-1)) {
                lv_obj_clear_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN);
                
                lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN);
                
                lvgl_port_unlock();
            }
                */

            



        lv_timer_t *t = lv_timer_create(wif_disconnected_cb, 100, NULL); // Create a timer to handle disconnection
        lv_timer_set_repeat_count(t, 1);  // Run the callback once
        //vTaskDelay(pdMS_TO_TICKS(1000));//
        

        
    } 
    
   


    
      else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG_STA, "WIFI GOT_IP.");  
        


        }


        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *) event_data;
        ESP_LOGI(TAG_STA, "Connected to SSID:%s, channel:%d", event->ssid, event->channel);

        /*
       
         if (mqttClient) {
            esp_mqtt_client_start(mqttClient); 

        } else {
            mqtt_start(); 
   
        }
*/
       //   wifi_connected = true;
       //   wifi_need_mqtt_start = true;  
            
           //esp_mqtt_client_reconnect(mqttClient);
           
        
         
        
       //esp_mqtt_client_start(mqttClient); 
      // mqtt_retry_task_init();
      // start_rssi_task();
        /*

        if (lvgl_port_lock(-1)) {
                //lv_obj_add_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN);
                
                lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN);
                
                lvgl_port_unlock();
            }
                
               */ 

            

    }
/*
      else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
    wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;

    ESP_LOGI(TAG_STA, "WIFI STA_CONNECTED to SSID:%s, BSSID:%02X:%02X:%02X:%02X:%02X:%02X, channel:%d",
             event->ssid,
             event->bssid[0], event->bssid[1], event->bssid[2],
             event->bssid[3], event->bssid[4], event->bssid[5],
             event->channel);


    save_wifi_credentials((char*)event->ssid, (char*)sta_config.sta.password, event->bssid);
}
  
*/

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

/*
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
                 //start_api=1;//
                // Log the obtained IP address and indicate successful connection
                ESP_LOGI("WiFi", "Connected with IP: " IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG_STA, "Connected to AP SSID:%s, password:%s, bssid:%s", sta_config.sta.ssid, sta_config.sta.password,(uint8_t*)ap_info_bssid);
                connection_flag = true;  // Set the connection flag to true
                _ui_flag_modify(ui_WIFI_PWD_Error, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);//
                if ((cnt==1)&&(reconnect==0)){// chip khởi động lại hoặc switch wifi on
                    reconnect=1;// đánh dấu đã reconnect thành công
                }
                if (user_selected_wifi){// nếu connect bằng cách nhập pass thì lưu pass, ngược lại ko lưu
                  user_selected_wifi=false;
                  save_wifi_credentials((char*)sta_config.sta.ssid,(char*)sta_config.sta.password,ap_info.bssid);// lưu ssid, password và của wifi kết nối thành công vào nvs
                //  save_wifi_credentials((char*)sta_config.sta.ssid,(char*)sta_config.sta.password,sta_config.sta.bssid);
                  ESP_LOGI(TAG_STA,"Saved AP SSID:%s, password:%s, bssid:%s",sta_config.sta.ssid, sta_config.sta.password,(uint8_t*)ap_info_bssid);
           }  

                //lv_obj_add_flag(ui_Image7, LV_OBJ_FLAG_HIDDEN);     /// Flags
                //lv_obj_clear_flag(ui_Image19, LV_OBJ_FLAG_HIDDEN);//
                s_retry_num = 0;  // Reset retry counter on successful connection

                memset(sta_config.sta.ssid, 0, sizeof(sta_config.sta.ssid));
                memset(sta_config.sta.password, 0, sizeof(sta_config.sta.password));
                memset(sta_config.sta.bssid, 0, sizeof(sta_config.sta.bssid));
                //start_rssi_task();//
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
                    //lv_obj_add_flag(ui_Image19, LV_OBJ_FLAG_HIDDEN);     /// Flags
                        memset(sta_config.sta.ssid, 0, sizeof(sta_config.sta.ssid));
                        memset(sta_config.sta.password, 0, sizeof(sta_config.sta.password));
                        memset(sta_config.sta.bssid, 0, sizeof(sta_config.sta.bssid));
                        //start_rssi_task();//

                     

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
            memset(sta_config.sta.ssid, 0, sizeof(sta_config.sta.ssid));
            memset(sta_config.sta.password, 0, sizeof(sta_config.sta.password));
            memset(sta_config.sta.bssid, 0, sizeof(sta_config.sta.bssid));
            //start_rssi_task();//
        }

        // Short delay (10ms) before checking the connection status again
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

*/

void wifi_wait_connect()
{
    static int s_retry_num = 0;
    wifi_config_t sta_config;
    
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &sta_config));
    ESP_LOGI(TAG_STA, "GET: SSID:%s, password:%s", sta_config.sta.ssid, sta_config.sta.password);

    uint32_t wait_time = 0;
    uint32_t max_wait = 10000; 
    
    while (wait_time < max_wait)
    {
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif)
        {
            esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
            if (ret == ESP_OK && ip_info.ip.addr != 0) {
                
                char ap_info_bssid[18];
                wifi_ap_record_t ap_info;
                esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
                if (err == ESP_OK) {
                    snprintf(ap_info_bssid, sizeof(ap_info_bssid), 
                             "%02X:%02X:%02X:%02X:%02X:%02X",
                             ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                             ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
                }
                
                lv_timer_t *t = lv_timer_create(wifi_ok_cb, 100, NULL);
                lv_timer_set_repeat_count(t, 1);
                
                ESP_LOGI("WiFi", "Connected with IP: " IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG_STA, "Connected to AP SSID:%s, password:%s, bssid:%s", 
                         sta_config.sta.ssid, sta_config.sta.password, (uint8_t*)ap_info_bssid);
                
                connection_flag = true;
                wifi_connected = true;//
                wifi_need_mqtt_start = true;//  
                _ui_flag_modify(ui_WIFI_PWD_Error, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
                
                if ((cnt == 1) && (reconnect == 0)) {
                    reconnect = 1;
                }
                
                if (user_selected_wifi) {
                    user_selected_wifi = false;
                    save_wifi_credentials((char*)sta_config.sta.ssid, 
                                         (char*)sta_config.sta.password, ap_info.bssid);
                    ESP_LOGI(TAG_STA, "Saved AP SSID:%s, password:%s, bssid:%s",
                             sta_config.sta.ssid, sta_config.sta.password, (uint8_t*)ap_info_bssid);
                }
                
                s_retry_num = 0;

                
                break;  //
            }
        }
        
       vTaskDelay(pdMS_TO_TICKS(500));
        wait_time += 500;
    }
    
   if (wait_time >= max_wait) {
        ESP_LOGI(TAG_STA, "Failed to connect to SSID:%s after %lu ms", 
                 sta_config.sta.ssid, max_wait);
        
        lv_timer_t *t = lv_timer_create(wifi_ok_cb, 100, NULL);
        lv_timer_set_repeat_count(t, 1);
        
        connection_flag = false;
        
        if (user_selected_wifi) {
            user_selected_wifi = false;
        }
        

    }
        
}

void wifi_sta_init(uint8_t *ssid, uint8_t *pwd, wifi_auth_mode_t authmode,  const uint8_t *bssid)
{   
       
     
    
    // rssi_task_running=false;
    
    /*
     if (mqttClient) {
            esp_mqtt_client_stop(mqttClient);
            ESP_LOGI("MQTT", "MQTT stopped before Wi-Fi reconnect");

        }
            */
            
  // esp_mqtt_client_disconnect(mqttClient);


    if (connection_flag)  // Check if already connected
    {
        printf("esp_wifi_disconnect \r\n");  // Log disconnection
        esp_wifi_disconnect();  // Disconnect from the current WiFi
        vTaskDelay(pdMS_TO_TICKS(500));//
        connection_flag = false;  // Reset connection flag
        wifi_need_mqtt_stop=true;//

    }



    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));//

    wifi_config_t wifi_config = {              
        .sta = {                                
            .threshold.authmode = authmode

        },                                      
    };
    
   

    strcpy((char *)wifi_config.sta.ssid, (const char *)ssid);

    strcpy((char *)wifi_config.sta.password, (const char *)pwd);

    memcpy(wifi_config.sta.bssid, bssid, 6); 
    wifi_config.sta.bssid_set = true;  
    //wifi_config.sta.bssid_set = false;  
        
        
    ESP_LOGI(TAG_STA, "Connecting to wifi network, BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
                 bssid[0], bssid[1], bssid[2],
                bssid[3],bssid[4], bssid[5]);


    // Set WiFi configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_stop();  // Stop WiFi module

    esp_wifi_start(); // Start WiFi with the new configuration
    //start_rssi_task();//
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
    ESP_ERROR_CHECK(esp_wifi_stop());  // Stop WiFi if it's running

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
//    if (sta_netif != NULL)
  //  {
    //    esp_netif_destroy(sta_netif);  // Destroy the network interface
      //  sta_netif = NULL;  // Prevent repeated destruction
    //}
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
        // Nếu có lệnh stop
        if (wifi_need_mqtt_stop)
        {
            wifi_need_mqtt_stop = false;
            if (mqttClient)
            {
                ESP_LOGI("MQTT_MANAGER", "Stopping MQTT safely...");
                esp_mqtt_client_disconnect(mqttClient);//
                esp_mqtt_client_stop(mqttClient);
                ESP_LOGI("MQTT_MANAGER", "MQTT stopped.");
            }
        }

        // Nếu có lệnh start
        if (wifi_need_mqtt_start)
        {
            wifi_need_mqtt_start = false;
            if (mqttClient)
            {
                ESP_LOGI("MQTT_MANAGER", "Starting MQTT safely...");
                esp_mqtt_client_start(mqttClient);
                ESP_LOGI("MQTT_MANAGER", "MQTT started.");
            }
            else
            {
                ESP_LOGI("MQTT_MANAGER", "No existing client, creating new one...");
                mqtt_start();  // dùng hàm của bạn
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200)); // tránh chiếm CPU
    }
}
