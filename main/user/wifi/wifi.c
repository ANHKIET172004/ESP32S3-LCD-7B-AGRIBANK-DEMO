#include "ui.h"
#include "rgb_lcd_port.h"
#include "wifi.h"
#include "lvgl_port.h"

#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "esp_mac.h"
#include "esp_log.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "wifi.h"
#include "esp_coexist.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "esp_mqtt_client/esp_mqtt_client.h"

#define RSSI_UPDATE_INTERVAL_MS  5000


const char *TAG_AP = "WiFi SoftAP";  // Tag for SoftAP mode
const char *TAG_STA = "WiFi Sta";    // Tag for Station mode

TaskHandle_t wifi_TaskHandle;
static wifi_sta_list_t sta_list;  // List to hold connected stations information


////////// các biến liên quan đến cấu hình wifi

extern wifi_config_t wifi_config;
extern char saved_ssid[32];
extern char saved_password[64];//
//////////
extern bool found_saved_ap;
extern int change;
extern int cnt;
int connect_success=0;

extern lv_obj_t * ui_Image16;
int8_t pressed=0;
//SemaphoreHandle_t check_sema=NULL;
extern SemaphoreHandle_t check_sema;

extern void reset_recent_number(void);

extern esp_netif_t *sta_netif;

//extern SemaphoreHandle_t nvs_mutex;

extern void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);


char saved_ssid1[32]={0} ;
char saved_pass1[32]={0} ;
size_t ssid_len1=sizeof(saved_pass1);
size_t password_len1=sizeof(saved_pass1) ;

bool force_reset=false;


esp_err_t read_wifi_credentials_from_nvs1(char *ssid, size_t *ssid_len, char *password, size_t *password_len,uint8_t* bssid) {
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
 void wifi_connection_cb(lv_timer_t *timer)
{
    // Hide the "waiting for connection" spinner and enable Wi-Fi related buttons
    _ui_flag_modify(ui_WIFI_Wait_CONNECTION, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
}

// Callback function to update UI with connected stations information in SoftAP mode
/*
static void wifi_ap_cb(lv_timer_t *timer)
{
    char mac_str[32];  // Buffer to hold formatted MAC address string
    snprintf(mac_str, sizeof(mac_str), "Connected: %d", sta_list.num); // Format the number of connected stations
    lv_label_set_text(ui_WIFI_AP_CON_NUM, mac_str); // Update the UI with the number of connected stations
    ESP_LOGI(TAG_AP, "Total connected stations: %d", sta_list.num);

    // Hide or show the MAC address list based on the number of connected stations
    if (sta_list.num == 0)
        _ui_flag_modify(ui_WIFI_AP_MAC_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    else {
        _ui_flag_modify(ui_WIFI_AP_MAC_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);

        lv_obj_clean(ui_WIFI_AP_MAC_List);  // Clean the MAC address list to prevent duplicates
    }

    // Iterate over the connected stations and display their MAC addresses
    for (int i = 0; i < sta_list.num; i++)
    {
        wifi_sta_info_t sta_info = sta_list.sta[i];

        // Format the MAC address and display it in the list
        snprintf(mac_str, sizeof(mac_str), "MAC: " MACSTR, MAC2STR(sta_info.mac));
        lv_obj_t * WIFI_AP_MAC_List_Button = lv_list_add_btn(ui_WIFI_AP_MAC_List, NULL, mac_str);

        // Customize button style
        lv_obj_set_style_bg_opa(WIFI_AP_MAC_List_Button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(WIFI_AP_MAC_List_Button, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(WIFI_AP_MAC_List_Button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(WIFI_AP_MAC_List_Button, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Log information about the connected stations
        ESP_LOGI(TAG_AP, "STA %d: MAC Address: " MACSTR, i, MAC2STR(sta_info.mac));
        ESP_LOGI(TAG_AP, "STA %d: RSSI: %d", i, sta_info.rssi);
    }
}
*/

// Initialize Wi-Fi for STA (Station) and AP (Access Point) modes
/*
void wifi_init1(void)
{ 
    
    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize the Wi-Fi driver with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_coex_preference_set(ESP_COEX_PREFER_WIFI);//


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));  // Set Wi-Fi mode to null initially
    start_wifi_events();  // Start handling Wi-Fi events
    ESP_ERROR_CHECK(esp_wifi_start());  // Start the Wi-Fi driver



}
*/

void wifi_init(void) {
   // s_wifi_event_group = xEventGroupCreate();

    char saved_ssid[32] = {0};
    char saved_pass[64] = {0};
    uint8_t saved_bssid[6] = {0};
    size_t ssid_len = sizeof(saved_ssid);
    size_t pass_len = sizeof(saved_pass);

    esp_err_t err = read_wifi_credentials_from_nvs1(saved_ssid, &ssid_len, 
                                                    saved_pass, &pass_len, 
                                                    saved_bssid);

    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();// QUAN TRá»ŒNG

    sta_netif = esp_netif_create_default_wifi_sta();// QUAN TRá»ŒNG



    // WiFi init
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //start_wifi_events();
    ///
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    ///

    

    // Cáº¥u hÃ¬nh WiFi
    wifi_config_t wifi_config = {0};
    
    if (err == ESP_OK && strlen(saved_ssid) > 0) {
        ESP_LOGI("reconnect", "Found saved WiFi: %s", saved_ssid);
        strncpy((char *)wifi_config.sta.ssid, saved_ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char *)wifi_config.sta.password, saved_pass, sizeof(wifi_config.sta.password) - 1);
        
        
    } 
    
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
 //   wifi_config.sta.pmf_cfg.capable = true;
 //   wifi_config.sta.pmf_cfg.required = false;

 //if (no_wifi==1){

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    start_wifi_events();  // Start handling Wi-Fi events
    
    ESP_ERROR_CHECK( esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_connect());//

    ESP_LOGI("reconnect", "WiFi init complete, waiting for connection...");
    wifi_wait_connect(); //
    
 }

// Set DNS address for the SoftAP mode
void wifi_ap_set_dns_addr(esp_netif_t *sta_netif, esp_netif_t *ap_netif)
{
    esp_netif_dns_info_t dns;

    // Get DNS information from the STA network interface
    esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns);

    uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    
    // Stop the DHCP server temporarily
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(ap_netif));

    // Set DNS address for the AP network interface
    ESP_ERROR_CHECK(esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns));

    // Restart the DHCP server
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(ap_netif));
}

// Wi-Fi task to handle scanning, station, and AP modes
void wifi_task(void *arg)
{   
   

    wifi_init();  // Initialize the Wi-Fi




    static uint8_t connection_num = 0;  // Variable to track the number of connected stations
    
    /////

    /////////

    while (1)
    {   
        //
       
       
        // Scan Wi-Fi networks if the scan flag is set
        if (WIFI_SCAN_FLAG)
        {
            wifi_scan();
            WIFI_SCAN_FLAG = false;
            
        }

        // Connect to a Wi-Fi network if the station flag is set
        if (WIFI_STA_FLAG)
        {   
           
            

            WIFI_STA_FLAG = false;
            
            waveahre_rgb_lcd_set_pclk(12 * 1000 * 1000);  // Set pixel clock for the LCD
            vTaskDelay(20);  // Delay for a short while
           //   if (found_saved_ap)///// khi reset lại/ switch on thì kết nối lại với wifi được lưu trong nvs
           // {/////
             //  found_saved_ap=false; // dừng kết nối cho các lần sau cho tới khi reset/swtich on 
              // wifi_sta_init((uint8_t*)saved_ssid, (uint8_t*)saved_password, ap_info[wifi_index].authmode,ap_info[wifi_index].bssid);        
                
             //}/////// 
              //else {// kết nối tới wifi chọn thủ công
            //wifi_sta_init(ap_info[wifi_index].ssid, wifi_pwd, ap_info[wifi_index].authmode,NULL);  // Initialize Wi-Fi as STA and connect
            
            wifi_sta_init(ap_info[wifi_index].ssid, wifi_pwd, ap_info[wifi_index].authmode,ap_info[wifi_index].bssid);  // Initialize Wi-Fi as STA and connect//
             //}//
            waveahre_rgb_lcd_set_pclk(EXAMPLE_LCD_PIXEL_CLOCK_HZ);  // Restore original pixel clock


     
            lv_timer_t *t = lv_timer_create(wifi_connection_cb, 100, NULL);  // Update UI every 100ms
            
            lv_timer_set_repeat_count(t, 2);  // Run only once
            

            
            
        }

        // Handle the SoftAP mode if the flag is set
        /*
        if (WIFI_AP_FLAG)
        {
            esp_err_t ret = esp_wifi_ap_get_sta_list(&sta_list);  // Get the list of connected stations
            if (ret == ESP_OK)
            {
                // If the number of connected stations has changed, update the UI
                if (connection_num != sta_list.num)
                {
                    lv_timer_t *t = lv_timer_create(wifi_ap_cb, 100, NULL);  // Update UI every 100ms
                    lv_timer_set_repeat_count(t, 1);  // Run only once
                    connection_num = sta_list.num;  // Update connection number
                }
            }
            else
            {
                ESP_LOGE(TAG_AP, "Failed to get STA list");
            }
        }
        */
        vTaskDelay(10);
    }
} 


int8_t get_rssi_level(int8_t rssi){

    if (rssi==0){
        return 0; // no wifi
    }
    else if (rssi>-25){
        return 1; // strong
    }
    else if ((rssi<-25)&&(rssi>-50)){
        return 2; // medium
    }
    else if((rssi<-50)&&(rssi>-75)){
        return 3; // weak

    }

    else {
        return 4; // very weak
    }

}

void hide_all_icon(){

            lv_obj_add_flag(ui_Image9, LV_OBJ_FLAG_HIDDEN );  
            lv_obj_add_flag(ui_Image10, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image12, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image11, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image13, LV_OBJ_FLAG_HIDDEN );

}


void mainscreen_wifi_rssi_task(void *pvParameters) {
    wifi_ap_record_t ap_info;
    //int8_t rssi;
    int8_t current_rssi_level=-1;
    int8_t last_rssi_level=-1;


   
    while (1) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {

            current_rssi_level=get_rssi_level(ap_info.rssi);// update rssi level

            connect_success=1;// đánh dấu đã kết nối wifi thành công

            
         if (current_rssi_level!=last_rssi_level){  
            if (lvgl_port_lock(-1)) {

            hide_all_icon();

            
            switch(current_rssi_level){
                case 0:
                 lv_obj_clear_flag(ui_Image9, LV_OBJ_FLAG_HIDDEN ); 
                 break;

                case 1:
                 lv_obj_clear_flag(ui_Image10, LV_OBJ_FLAG_HIDDEN ); 
                 break;

                case 2:
                 lv_obj_clear_flag(ui_Image11, LV_OBJ_FLAG_HIDDEN ); 
                 break; 
                
                case 3:
                 lv_obj_clear_flag(ui_Image12, LV_OBJ_FLAG_HIDDEN ); 
                 break;
                
                case 4:
                 lv_obj_clear_flag(ui_Image13, LV_OBJ_FLAG_HIDDEN ); 
                 break;


            }

             lvgl_port_unlock();
           }  



        last_rssi_level=current_rssi_level;
   }         
        } else {
            ESP_LOGW("RSSI", "Not connected to any AP");
            if (lvgl_port_lock(-1)) {
            
            last_rssi_level=-1;

            hide_all_icon();
            
            lv_obj_clear_flag(ui_Image16, LV_OBJ_FLAG_HIDDEN ); // không kết nối đến mqtt

            lv_obj_clear_flag(ui_Image9, LV_OBJ_FLAG_HIDDEN );  

            lvgl_port_unlock();
            }
        }
      
        
        vTaskDelay(pdMS_TO_TICKS(500)); // Delay 2s
   }
   
}



void checktime_task1(void *pvParameters)// sau khi gọi số mới mà số hiện tại chưa được đánh giá thì chờ 10s, trong 10s nếu số cũ được đánh giá thì xóa current number, copy next vào current.
// Nếu sau 10s số cũ không được đánh giá thì xóa current, copy next vào current number
// tin nhắn chứa nội dung đánh giá dịch vụ bao gồm điểm đánh giá và current number
{   
 
     uint8_t x=0;
     TickType_t timeout=0;

    while (1)
    {    
       
        

        if (xSemaphoreTake(check_sema, portMAX_DELAY))
        
        //if (xSemaphoreTake(check_sema, pdMS_TO_TICKS(100)))
        {   

            if (read_time(&x)==ESP_OK){
                if (x>0){
                ESP_LOGI("CHECKTIME TASK","saved timeout: %u",x);
                //const TickType_t timeout = pdMS_TO_TICKS(x);
                timeout = pdMS_TO_TICKS(x*1000);
                }

            }
            else {
            //const TickType_t timeout = pdMS_TO_TICKS(10000);
            timeout = pdMS_TO_TICKS(10000);

            }
           
  
            TickType_t start = xTaskGetTickCount();

            ESP_LOGI("CHECKTIME TASK", "Start waiting for %d seconds...",x);

           

            while ((xTaskGetTickCount() - start) < timeout)
            {
                if (pressed == 1)// icon đánh giá được chọn
                {
                    pressed = 0;
                    ESP_LOGI("CHECKTIME TASK", "Pressed");
                    //xSemaphoreTake(nvs_mutex, portMAX_DELAY);
                    reset_recent_number();
                    //xSemaphoreGive(nvs_mutex);

                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(100));
            }
        

            if ((xTaskGetTickCount() - start) >= timeout)
            {
                ESP_LOGI("CHECKTIME TASK", "TIMEOUT ");
                //xSemaphoreTake(nvs_mutex, portMAX_DELAY);
                reset_recent_number();
                
                //xSemaphoreGive(nvs_mutex);

            }

            if (force_reset==true){
                force_reset=false;
                reset_recent_number();

            }
        }
        
    }
}


volatile bool checktime_stop = false;   // biến yêu cầu dừng task

void checktime_task(void *pvParameters)
{
    uint8_t x = 0;
    TickType_t timeout = 0;

    while (1)
    {
        // Đợi tín hiệu bắt đầu kiểm tra
        xSemaphoreTake(check_sema, portMAX_DELAY);

    //RESET_TIMER:
        // Đọc thời gian timeout từ NVS
        if (read_time(&x) == ESP_OK && x > 0)
        {
            timeout = pdMS_TO_TICKS(x * 1000);
            //ESP_LOGI("CHECKTIME TASK", "timeout: %u sec", x);
            if (!pressed){
            ESP_LOGI("CHECKTIME TASK", "Start waiting for %d seconds...",x);
            }

        }
        else
        {
            timeout = pdMS_TO_TICKS(30000); 
            if (!pressed){
            ESP_LOGI("CHECKTIME TASK", "default timeout: 30s");
            ESP_LOGI("CHECKTIME TASK", "Start waiting for 30 seconds...");
            }
        }

        TickType_t start = xTaskGetTickCount();

        while ((xTaskGetTickCount() - start) < timeout)
        {
            // Nếu có nhấn đánh giá
            if (pressed)
            {
                pressed = 0;
                ESP_LOGI("CHECKTIME", "Pressed, reset current");
                reset_recent_number();
                break;
            }

            // Nếu nhận sema mới, reset thời gian lại từ đầu
            if (xSemaphoreTake(check_sema, 0) == pdTRUE)
            {
                //ESP_LOGI("CHECKTIME", "New sema, Restart timer");
                ESP_LOGI("CHECKTIME", "New sema, skip next number");
                force_reset=true;
                //goto RESET_TIMER;
                //reset_recent_number();
               // break;
            }
            if (checktime_stop==true){
                reset_recent_number();
                ESP_LOGI("CHECKTIME", "Task stopped, skip number");

                break;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // Hết thời gian
        if ((xTaskGetTickCount() - start) >= timeout)
        {
            ESP_LOGI("CHECKTIME TASK", "TIMEOUT, reset current");
            reset_recent_number();
        }
        
    }
}



void checktime_task2(void *pvParameters)
{
    uint8_t x = 0;
    TickType_t timeout = 0;

    while (1)
    {
        // Kiểm tra yêu cầu dừng task
        if (checktime_stop)
        {
            ESP_LOGW("CHECKTIME", "Task stopped");
            vTaskDelete(NULL);     // tự xóa task
        }

        // Đợi sự kiện bắt đầu vòng kiểm tra
        xSemaphoreTake(check_sema, portMAX_DELAY);

        while (1)   // vòng restart timer
        {
            // Kiểm tra yêu cầu dừng task
            if (checktime_stop)
            {
                ESP_LOGW("CHECKTIME", "Task stopped");
                vTaskDelete(NULL);
            }

            // --- Đọc thời gian từ NVS ---
            if (read_time(&x) == ESP_OK && x > 0)
            {
                timeout = pdMS_TO_TICKS(x * 1000);
                ESP_LOGI("CHECKTIME", "Timeout = %u sec", x);
            }
            else
            {
                timeout = pdMS_TO_TICKS(10000);
                ESP_LOGI("CHECKTIME", "Timeout default = 10 sec");
            }

            TickType_t start = xTaskGetTickCount();

            // --- Vòng chờ timeout ---
            while ((xTaskGetTickCount() - start) < timeout)
            {
                // Kiểm tra yêu cầu dừng task
                if (checktime_stop)
                {
                    ESP_LOGW("CHECKTIME", "Task stopped");
                    vTaskDelete(NULL);
                }

                // Nếu có nhấn đánh giá
                if (pressed)
                {
                    pressed = 0;
                    ESP_LOGI("CHECKTIME", "Pressed, reset");
                    reset_recent_number();
                    goto END_WAIT;
                }

                // Nếu có sema mới -> restart timer
                if (xSemaphoreTake(check_sema, 0) == pdTRUE)
                {
                    ESP_LOGI("CHECKTIME", "New sema → restart timer");
                    goto RESTART_WAIT;
                }

                vTaskDelay(pdMS_TO_TICKS(50));
            }

            // Timeout
            ESP_LOGI("CHECKTIME", "TIMEOUT → reset");
            reset_recent_number();

        END_WAIT:
            break;

        RESTART_WAIT:
            continue;
        }
    }
}
