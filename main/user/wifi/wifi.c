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
#include "esp_wifi.h"


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

extern lv_obj_t * ui_Image38;

// bool rssi_task_running = false;



//////////

#define RSSI_UPDATE_INTERVAL_MS  5000

static const char *TAG_RSSI = "WiFiRSSI";
static TaskHandle_t wifi_rssi_task_handle = NULL;
static EventGroupHandle_t wifi_rssi_event_group = NULL;

#define WIFI_CONNECTED_BIT BIT0

// Cache thông tin AP từ event (an toàn, không gọi driver)
typedef struct {
    uint8_t bssid[6];
    char ssid[33];
    int8_t rssi;
    bool connected;
    wifi_auth_mode_t authmode;
    TickType_t last_update;
} wifi_ap_cache_t;

static wifi_ap_cache_t ap_cache = {
    .connected = false,
    .rssi = -127,
    .last_update = 0
};

// Callback function to update UI when Wi-Fi connection is established
/*
static void wifi_connection_cb(lv_timer_t *timer)
{
    // Hide the "waiting for connection" spinner and enable Wi-Fi related buttons
    _ui_flag_modify(ui_WIFI_Wait_CONNECTION, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
}
*/
 void wifi_connection_cb(lv_timer_t *timer)
{
    // Hide the "waiting for connection" spinner and enable Wi-Fi related buttons
    _ui_flag_modify(ui_WIFI_Wait_CONNECTION, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
}

// Callback function to update UI with connected stations information in SoftAP mode
static void wifi_ap_cb(lv_timer_t *timer)
{
    char mac_str[32];  // Buffer to hold formatted MAC address string
    snprintf(mac_str, sizeof(mac_str), "Connected: %d", sta_list.num); // Format the number of connected stations
    lv_label_set_text(ui_WIFI_AP_CON_NUM, mac_str); // Update the UI with the number of connected stations
    ESP_LOGI(TAG_AP, "Total connected stations: %d", sta_list.num);

    // Hide or show the MAC address list based on the number of connected stations
    if (sta_list.num == 0)
        _ui_flag_modify(ui_WIFI_AP_MAC_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    else
        _ui_flag_modify(ui_WIFI_AP_MAC_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);

    lv_obj_clean(ui_WIFI_AP_MAC_List);  // Clean the MAC address list to prevent duplicates

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

// Initialize Wi-Fi for STA (Station) and AP (Access Point) modes
void wifi_init(void)
{
    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize the Wi-Fi driver with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));  // Set Wi-Fi mode to null initially
    start_wifi_events();  // Start handling Wi-Fi events
    ESP_ERROR_CHECK(esp_wifi_start());  // Start the Wi-Fi driver


    ///////////////
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
              if (found_saved_ap)///// khi reset lại/ switch on thì kết nối lại với wifi được lưu trong nvs
            {/////
               found_saved_ap=false; // dừng kết nối cho các lần sau cho tới khi reset/swtich on 
               wifi_sta_init((uint8_t*)saved_ssid, (uint8_t*)saved_password, ap_info[wifi_index].authmode,ap_info[wifi_index].bssid);        
                
             }/////// 
              else {// kết nối tới wifi chọn thủ công
               // found_saved_ap=false; // dừng kết nối cho các lần sau cho tới khi reset/swtich on
            //wifi_sta_init(ap_info[wifi_index].ssid, wifi_pwd, ap_info[wifi_index].authmode,NULL);  // Initialize Wi-Fi as STA and connect
            wifi_sta_init(ap_info[wifi_index].ssid, wifi_pwd, ap_info[wifi_index].authmode,ap_info[wifi_index].bssid);  // Initialize Wi-Fi as STA and connect//
             }//
            waveahre_rgb_lcd_set_pclk(EXAMPLE_LCD_PIXEL_CLOCK_HZ);  // Restore original pixel clock


     
            lv_timer_t *t = lv_timer_create(wifi_connection_cb, 100, NULL);  // Update UI every 100ms
            
            lv_timer_set_repeat_count(t, 1);  // Run only once
            

            
            
        }

        // Handle the SoftAP mode if the flag is set
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

void mainscreen_wifi_rssi_task(void *pvParameters) {
    wifi_ap_record_t ap_info;
    int8_t rssi;
    int8_t current_rssi_level=-1;
    int8_t last_rssi_level=-1;


   
    while (1) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {

            current_rssi_level=get_rssi_level(ap_info.rssi);// update rssi level
        

            connect_success=1;// đánh dấu đã kết nối wifi thành công
            /*
            ESP_LOGI("RSSI", "Connected SSID:%s, BSSID:%02X:%02X:%02X:%02X:%02X:%02X, RSSI:%d dBm",
                     ap_info.ssid,
                     ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                     ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5],
                      
                     ap_info.rssi);
              */
            
         if (current_rssi_level!=last_rssi_level){     
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );  
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
              if (ap_info.rssi == 0 && ap_info.ssid[0] == '\0')
         {    
            if (lvgl_port_lock(-1)) {
            lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );  
           // lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );

            lvgl_port_unlock();
           }   
          //  break;
            //vTaskDelete(NULL);
        }
         
         else if(ap_info.rssi > -25)  // Strong signal (RSSI > -25)
        {   
            if (lvgl_port_lock(-1)) {
            lv_obj_clear_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
           // lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
        }
            // Add button with strong signal icon
          //  WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_4_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info.rssi < -25) && (ap_info.rssi > -50))  // Medium signal
        { 

            if (lvgl_port_lock(-1)) {
            lv_obj_clear_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
        }
   
            // Add button with medium signal icon
          //  WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_3_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info.rssi < -50) && (ap_info.rssi > -75))  // Weak signal
        {     
            if (lvgl_port_lock(-1)) {
            lv_obj_clear_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            lvgl_port_unlock();
            }
            // Add button with weak signal icon
           // WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_2_png, (const char *)ap_info[i].ssid);
        }
        else  // Very weak signal (RSSI < -75)
        {   

            if (lvgl_port_lock(-1)) {
            lv_obj_clear_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lvgl_port_unlock();
            }
            // Add button with very weak signal icon
              //_ui_flag_modify(ui_WIFI_SCAN_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);  
        }    

        last_rssi_level=current_rssi_level;
   }         
        } else {
            ESP_LOGW("RSSI", "Not connected to any AP");
            if (lvgl_port_lock(-1)) {
            
            lv_obj_clear_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN ); // không kết nối đến mqtt

            lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );  
            //lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            //lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            lvgl_port_unlock();
            }
        }
      
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay 2s
    }
}

/*
void mainscreen_wifi_rssi_task(void *pvParameters) {
    int rssi;
    
    while (rssi_task_running) {
        esp_err_t ret = esp_wifi_sta_get_rssi(&rssi);
        
        if (ret == ESP_OK) {
            connect_success = 1;
            
            ESP_LOGI("RSSI", "Current RSSI: %d dBm", rssi);
            
            if (lvgl_port_lock(-1)) {
                
                lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN);
                
                if (rssi >= -25) {
                    lv_obj_clear_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN);
                }
                else if (rssi >= -50) {
                    lv_obj_clear_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN);
                }
                else if (rssi >= -75) {
                    lv_obj_clear_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN);
                }
                else {
                    lv_obj_clear_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN);
                }
                
                lvgl_port_unlock();
            }
            
        } else {
            connect_success = 0;
            ESP_LOGW("RSSI", "Not connected to any AP");
            
            if (lvgl_port_lock(-1)) {
                lv_obj_clear_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN);
                
                lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN);
                
                lvgl_port_unlock();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
*/
// Thêm vào file wifi.c (hoặc file hiện tại)

//TaskHandle_t rssi_task_handle = NULL;  // Khai báo handle
// bool rssi_task_running = false;  // Bạn đã có biến này rồi

// Hàm khởi động RSSI task
/*
void start_rssi_task(void) {
    if (rssi_task_handle == NULL) {
        rssi_task_running = true;
        xTaskCreate(mainscreen_wifi_rssi_task, 
                    "rssi_monitor", 
                    4096, 
                    NULL, 
                    3, 
                    &rssi_task_handle);
        ESP_LOGI("RSSI", "RSSI monitoring task started");
    } else {
        ESP_LOGW("RSSI", "RSSI task already running");
    }
}

// Hàm dừng RSSI task
void stop_rssi_task(void) {
    if (rssi_task_handle != NULL) {
        rssi_task_running = false;
        ESP_LOGI("RSSI", "Stopping RSSI task...");
        
        // Chờ task tự thoát (tối đa 3 giây)
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        // Nếu task vẫn còn tồn tại, force delete
        if (rssi_task_handle != NULL) {
            vTaskDelete(rssi_task_handle);
            rssi_task_handle = NULL;
            ESP_LOGW("RSSI", "RSSI task force stopped");
        }
    } else {
        ESP_LOGW("RSSI", "RSSI task not running");
    }
}

// Hàm kiểm tra trạng thái
bool is_rssi_task_running(void) {
    return (rssi_task_handle != NULL && rssi_task_running);
}

void mainscreen_wifi_rssi_task(void *pvParameters) {
    int rssi;
    
    ESP_LOGI("RSSI", "RSSI monitoring task started");
    
   // while (rssi_task_running) {
    while (1) {
        esp_err_t ret = esp_wifi_sta_get_rssi(&rssi);
        
        if (ret == ESP_OK) {
            connect_success = 1;
            
            ESP_LOGI("RSSI", "Current RSSI: %d dBm", rssi);
            
            if (lvgl_port_lock(100)) {  // Thêm timeout 100ms thay vì -1
                
                lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN);
                
                if (rssi >= -25) {
                    lv_obj_clear_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN);
                }
                else if (rssi >= -50) {
                    lv_obj_clear_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN);
                }
                else if (rssi >= -75) {
                    lv_obj_clear_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN);
                }
                else {
                    lv_obj_clear_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN);
                }
                
                lvgl_port_unlock();
            } else {
                ESP_LOGW("RSSI", "Failed to lock LVGL");
            }
            
        } else {
            connect_success = 0;
            ESP_LOGW("RSSI", "Not connected to any AP");
            
            if (lvgl_port_lock(100)) {  // Timeout 100ms
                lv_obj_clear_flag(ui_Image38, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN);
                
                lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN);
                
                lvgl_port_unlock();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
    // Cleanup trước khi thoát
    ESP_LOGI("RSSI", "RSSI monitoring task stopped");
    rssi_task_handle = NULL;
    vTaskDelete(NULL);
}
    */
/////////////////

 