#include "ui.h"
#include "wifi.h"
//#include "esp_mesh.h"
#include "nvs.h"

static esp_netif_t *sta_netif;

//int8_t wifi_last_index = -1;  // Stores the last connected Wi-Fi index
//lv_obj_t *wifi_last_Button;  // Stores the last button associated with the Wi-Fi
bool connection_flag = false; // Flag to track if a connection was successfully made previously
bool connection_last_flag = false; // Flag to check if the STA is reconnecting while in AP mode
static esp_netif_ip_info_t ip_info; // Stores IP information

extern char saved_ssid[32] ;
extern char saved_password[64];

extern bool found_saved_ap;

//extern lv_obj_t *wifi_last_Button;

static void wifi_connected_cb(lv_timer_t *timer)
{
    if (wifi_last_Button != NULL) {
        lv_obj_clean(wifi_last_Button);
        lv_obj_t *img = lv_img_create(wifi_last_Button);
        lv_img_set_src(img, &ui_img_ok_png);   // icon Wi-Fi OK
        lv_obj_align(img, LV_ALIGN_RIGHT_MID, -10, 0);
    }
}

 extern lv_obj_t *saved_wifi_button;


//////////////// lưu ssid,pass và bssid

void save_wifi_credentials (const char *ssid, const char *password, const uint8_t* bssid) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("wifi_cred", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Failed to open NVS handle (%s)", esp_err_to_name(err));
        return;
    }
    nvs_set_str(my_handle, "ssid", ssid);
    nvs_set_str(my_handle, "password", password);
    nvs_set_blob(my_handle, "bssid", bssid, 6); 
    nvs_commit(my_handle);
    nvs_close(my_handle);
    ESP_LOGI("NVS", "Saved wifi credentials successfully!");
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
            WIFI_CONNECTION_DONE = true; 
            if (wifi_last_index != -1 && wifi_last_index != wifi_index)
            {  


                // Update the Wi-Fi signal icon based on RSSI
                img = lv_obj_get_child(wifi_last_Button, 0);


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

     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {          
        ESP_LOGI(TAG_STA, "WIFI STA_DISCONNECTED.");
        
        lv_timer_t *t = lv_timer_create(wif_disconnected_cb, 100, NULL); // Create a timer to handle disconnection
        lv_timer_set_repeat_count(t, 1);  // Run the callback once
    }  

    
      else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG_STA, "WIFI GOT_IP.");

       // lv_timer_t *t = lv_timer_create(wifi_connected_cb, 100, NULL);
        //lv_timer_set_repeat_count(t, 1);
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
    // ESP_LOGI(TAG_STA, "GET: SSID:%s, password:%s", sta_config.sta.ssid, sta_config.sta.password);

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

                // Log the obtained IP address and indicate successful connection
                ESP_LOGI("WiFi", "Connected with IP: " IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG_STA, "Connected to AP SSID:%s, password:%s, bssid:%s", sta_config.sta.ssid, sta_config.sta.password,(uint8_t*)ap_info_bssid);
                connection_flag = true;  // Set the connection flag to true

                //////////////////
                save_wifi_credentials((char*)sta_config.sta.ssid,(char*)sta_config.sta.password,ap_info.bssid);// lưu ssid, password và của wifi kết nối thành công vào nvs

                ///////////////////////

                s_retry_num = 0;  // Reset retry counter on successful connection
                ////
                 if (found_saved_ap){
                    found_saved_ap=false;
                 }
                ////
                break;  // Exit the loop since the connection is successful
            } else {
                // Log the failure to connect or obtain an IP address
                ESP_LOGI(TAG_STA, "Failed to connect to the AP");

                // Retry connection if the retry counter is less than 5
                if (s_retry_num < 5)
                {
                    s_retry_num++;
                    ESP_LOGI(TAG_STA, "Retrying to connect to the AP");
                }
                else {
                    // Reset retry counter after 5 attempts and log the failure
                    s_retry_num = 0;
                    lv_timer_t *t = lv_timer_create(wifi_ok_cb, 100, NULL);  // Update the UI every 100ms
                    lv_timer_set_repeat_count(t, 1);
                    ESP_LOGI(TAG_STA, "Failed to connect to SSID:%s, password:%s",
                            sta_config.sta.ssid, sta_config.sta.password);

                     ////
                 if (found_saved_ap){
                    found_saved_ap=false;
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
}


void wifi_sta_init(uint8_t *ssid, uint8_t *pwd, wifi_auth_mode_t authmode,  const uint8_t *bssid)
{
    if (connection_flag)  // Check if already connected
    {
        printf("esp_wifi_disconnect \r\n");  // Log disconnection
        esp_wifi_disconnect();  // Disconnect from the current WiFi
        connection_flag = false;  // Reset connection flag
    }
    
    wifi_config_t wifi_config = {              \
        .sta = {                                \
            .threshold.authmode = authmode,     \
        },                                      \
    };
    
   

if (found_saved_ap){//
     
    //found_saved_ap=false;

    strncpy((char *)wifi_config.sta.ssid, (const char *)saved_ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';

    strncpy((char *)wifi_config.sta.password, (const char *)saved_password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';


}  
//
  else {//

    strncpy((char *)wifi_config.sta.ssid, (const char *)ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';

    strncpy((char *)wifi_config.sta.password, (const char *)pwd, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    
 }//


    ///////////////////


    if (bssid != NULL) {
        wifi_config.sta.bssid_set = true;  // Bật chế độ chỉ định BSSID
        memcpy(wifi_config.sta.bssid, bssid, 6);  // Copy BSSID (6 bytes)
        
        ESP_LOGI(TAG_STA, "Connecting to specific BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
                 bssid[0], bssid[1], bssid[2],
                bssid[3],bssid[4], bssid[5]);
    } else {
        wifi_config.sta.bssid_set = false;  // Kết nối đến bất kỳ AP nào có SSID phù hợp
    }


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
            // stop_wifi_events();  // Optional: Stop WiFi events if implemented
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
