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
#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/ui/ui.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "freertos/event_groups.h"
#include "esp_mqtt_client/esp_mqtt_client.h"
#include "esp_sleep.h"
#include "esp_task_wdt.h"



static const char *TAG = "main"; // Tag used for ESP log output

static esp_lcd_panel_handle_t panel_handle = NULL; // Handle for the LCD panel
static esp_lcd_touch_handle_t tp_handle = NULL;    // Handle for the touch panel

extern int rate;
extern int score;

extern int mesh_enb;

uint8_t start=0;

extern EventGroupHandle_t wifi_event_group;
extern int WIFI_CONNECTED_BIT ;

extern void wifi_mqtt_manager_task(void *pv);
extern void checktime_task (void* pvParameters);

SemaphoreHandle_t check_sema=NULL;
//SemaphoreHandle_t nvs_mutex=NULL;


QueueHandle_t mqtt_queue=NULL;
typedef struct {
    char topic[64];
    char data[512];
} mqtt_message_t;

//extern lv_obj_t * ui_Image37;
//extern lv_obj_t * ui_Image36;

SemaphoreHandle_t nvs_mutex = NULL;


extern void mqtt_process_task(void *pvParameters);



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





void app_main()
{
    // Initialize the Non-Volatile Storage (NVS) for settings and data persistence
    // This ensures that user data and settings are retained even after power loss.
  //  init_nvs();
  //////////////
    ESP_LOGI("PSRAM", "Total heap: %d", esp_get_free_heap_size());
    //ESP_LOGI("PSRAM", "Total PSRAM: %d", esp_psram_get_size());
    ESP_LOGI("PSRAM", "Free PSRAM: %d", esp_get_free_heap_size() - heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase and re-initialize if no free pages or new version found
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    
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
    nvs_mutex = xSemaphoreCreateMutex();

    mqtt_queue = xQueueCreate(10, sizeof(mqtt_message_t));

    if (mqtt_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT queue");
    }

    //xTaskCreate(mqtt_process_task, "mqtt_process_task", 4096, NULL, 5, NULL);
    xTaskCreatePinnedToCore(mqtt_process_task,  "mqtt_retry",  4096, NULL, 4, NULL, 1 );
    //xTaskCreatePinnedToCore(mqtt_process_task,  "mqtt_retry",  6*1024, NULL, 4, NULL, 1 );


    

    xTaskCreatePinnedToCore(wifi_task, "wifi_task", 7* 1024, NULL, 6, &wifi_TaskHandle, 0);//6
  
     //mqtt_start();

    // xTaskCreate(mainscreen_wifi_rssi_task, "mainscreen_wifi_rssi_task", 4* 1024, NULL, 1, NULL);
    xTaskCreatePinnedToCore(mainscreen_wifi_rssi_task, "wifi_rssi_task", 4 * 1024, NULL, 1, NULL, 1);


    //xTaskCreate(wifi_mqtt_manager_task, "wifi_mqtt_manager_task", 4096, NULL, 5, NULL);
    xTaskCreatePinnedToCore(wifi_mqtt_manager_task, "wifi_mqtt_manager_task", 4 * 1024, NULL, 5, NULL, 1);
    

    check_sema = xSemaphoreCreateBinary();//
   
    //xSemaphoreGive(check_sema);

    //xTaskCreate(checktime_task, "check_sem_task", 4*1024, NULL, 4, NULL);//
    xTaskCreatePinnedToCore(checktime_task, "check_sem_task", 4 * 1024, NULL, 4, NULL, 1);


    /*
    if (delete_current_number()==ESP_OK){
        ESP_LOGI(TAG,"DELETE CURRENT NUMBER SUCCESSFULLY");
    }
    else {
        ESP_LOGI(TAG,"DELETE CURRENT NUMBER FAILED");
    }
        */

   // esp_task_wdt_add(NULL);  // main task
   
   /*
    while (1) {
        //system_state_proccessing();
        //esp_task_wdt_reset();       // Reset watchdog cho main
       // ESP_LOGI("HEAP", "Free heap: %u", esp_get_free_heap_size());
        if (start<255){
            start++;
        }
        else {
            start=0;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);   // Nhường CPU
    }
   
  */


}