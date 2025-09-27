/*****************************************************************************
 * | File       :   main.c
 * | Author     :   Waveshare team
 * | Function   :   Main function
 * | Info       :
 * |                UI Design：
 *                          1. User Login and Creation: Users can log in or create new accounts, and the created users are saved to NVS, so data is not lost after power-down.
 *                          2. Wi-Fi: Can connect to Wi-Fi and start an access point (hotspot).
 *                          3. RS485: Can send and receive data, with data displayed on the screen.
 *                          4. PWM: Can modify PWM output in multiple ways to control screen brightness. Additionally, it can display information from a Micro SD card.
 *                          5. CAN: Can send and receive data, with data displayed on the screen.
 *----------------
 * | Version    :   V1.0
 * | Date       :   2025-05-08
 * | Info       :   Basic version
 *
 ******************************************************************************/
#include "rgb_lcd_port.h" // Header for Waveshare RGB LCD driver
#include "gt911.h"        // Header for touch screen operations (GT911)
#include "lvgl_port.h"    // Header for LVGL port initialization and locking
//#include "add_password.h" // Header for password handling logic
//#include "pwm.h"          // Header for PWM initialization (used for backlight control)
#include "wifi.h"         // Header for Wi-Fi functionality
//#include "sd_card.h"      // Header for SD card operations
//#include "D:/ESP32/ESP32S3_LCD_7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/ui/ui.h"           // Header for user interface initialization
#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/ui/ui.h"
#include "nvs_flash.h"

// các file header hỗ trợ gatt

//#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/gatt_server/gatt_server.h"
#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/gatt_client/gatt_client.h"





static const char *TAG = "main"; // Tag used for ESP log output

static esp_lcd_panel_handle_t panel_handle = NULL; // Handle for the LCD panel
static esp_lcd_touch_handle_t tp_handle = NULL;    // Handle for the touch panel


/**
 * @brief Main application function.
 *
 * This function initializes the necessary hardware components such as the touch screen
 * and RGB LCD display, sets up the LVGL library for graphics rendering, and runs
 * the LVGL demo UI.
 *
 * - Initializes the GT911 touch screen controller.
 * - Initializes the Waveshare ESP32-S3 RGB LCD display.
 * - Initializes the LVGL library for graphics rendering.
 * - Runs the LVGL demo UI.
 *
 * @return None
 */


////////////gatt

void mainscreen_wifi_rssi_task(void *pvParameters) {
    wifi_ap_record_t ap_info;

    while (1) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI("RSSI", "Connected SSID:%s, BSSID:%02X:%02X:%02X:%02X:%02X:%02X, RSSI:%d dBm",
                     ap_info.ssid,
                     ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                     ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5],
                     ap_info.rssi);


              if (ap_info.rssi == 0 && ap_info.ssid[0] == '\0')
        {    
            //if (lvgl_port_lock(0)) {
            lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );  
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );

            //lvgl_port_unlock();
           // }   
            //break;
            //vTaskDelete(NULL);
        }
         
         else if(ap_info.rssi > -25)  // Strong signal (RSSI > -25)
        {   
            lv_obj_clear_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with strong signal icon
          //  WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_4_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info.rssi < -25) && (ap_info.rssi > -50))  // Medium signal
        { 
            lv_obj_clear_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with medium signal icon
          //  WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_3_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info.rssi < -50) && (ap_info.rssi > -75))  // Weak signal
        { 
            lv_obj_clear_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with weak signal icon
           // WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_2_png, (const char *)ap_info[i].ssid);
        }
        else  // Very weak signal (RSSI < -75)
        { 
            lv_obj_clear_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with very weak signal icon
              //_ui_flag_modify(ui_WIFI_SCAN_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);      
   }         
        } else {
            ESP_LOGW("RSSI", "Not connected to any AP");
            lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );  
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
        }
      
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay 5s
    }
}

void app_main()
{
    // Initialize the Non-Volatile Storage (NVS) for settings and data persistence
    // This ensures that user data and settings are retained even after power loss.
  //  init_nvs();
  //////////////
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase and re-initialize if no free pages or new version found
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

   ble_init();
    
    // Open NVS for reading
    /*
    nvs_handle_t nvs_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS handle!\n");
        return false;
    }
*/
    // Read username from NVS
    /*
    size_t username_size = MAX_LENGTH;
    err = nvs_get_str(nvs_handle, USERNAME_KEY, saved_username, &username_size);
    if (err != ESP_OK) {
        printf("Read failed!\n");
        nvs_close(nvs_handle);
        return false;
    }

    nvs_close(nvs_handle);

    return err;

*/


  ////////////////

    //static esp_lcd_panel_handle_t panel_handle = NULL; // Handle for the LCD panel
    //static esp_lcd_touch_handle_t tp_handle = NULL;    // Handle for the touch panel

    // Initialize the GT911 touch screen controller
    // This sets up the touch functionality of the screen.
    tp_handle = touch_gt911_init();

    // Initialize the Waveshare ESP32-S3 RGB LCD hardware
    // This prepares the LCD panel for display operations.
    panel_handle = waveshare_esp32_s3_rgb_lcd_init();

    // Turn on the LCD backlight
    // This ensures the display is visible.
    wavesahre_rgb_lcd_bl_on();

    // Initialize the LVGL library, linking it to the LCD and touch panel handles
    // LVGL is a lightweight graphics library used for rendering UI elements.
    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

    ESP_LOGI(TAG, "Display LVGL demos");

    // Lock the LVGL port to ensure thread safety during API calls
    // This prevents concurrent access issues when using LVGL functions.
    if (lvgl_port_lock(-1))
    {

        // Initialize the UI components with LVGL (e.g., demo screens, sliders)
        // This sets up the user interface elements using the LVGL library.
        ui_init();

        // Release the mutex after LVGL operations are complete
        // This allows other tasks to access the LVGL port.
        lvgl_port_unlock();
    }
    vTaskDelay(100); // Delay for a short period to ensure stable initialization

    // Initialize PWM for controlling backlight brightness (if applicable)
    // PWM is used to adjust the brightness of the LCD backlight.
    //pwm_init();

    // Initialize SD card operations
    // This sets up the Micro SD card for data storage and retrieval.
    //sd_init();

    // Start the WIFI task to handle Wi-Fi functionality
    // This task manages Wi-Fi connections and hotspot creation.
    xTaskCreate(wifi_task, "wifi_task", 6 * 1024, NULL, 9, &wifi_TaskHandle);
    

   // xTaskCreate(ble_server_task, "ble_server_task", 8 * 1024, NULL, 10, NULL);
     xTaskCreate(mainscreen_wifi_rssi_task, "mainscreen_wifi_rssi_task", 8 * 1024, NULL, 9, NULL);

   xTaskCreate(send_message_task, "send_message_task", 8 * 1024, NULL, 10, NULL);

    






}