#include <M5GFX.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#define XEBEC_TAG "LOG"
#define ESP_INTR_FLAG_DEFAULT 0

#define M5STACK_LEFT_BUTTON   GPIO_NUM_39
#define M5STACK_MIDDLE_BUTTON GPIO_NUM_38
#define M5STACK_RIGHT_BUTTON  GPIO_NUM_37

#define GPIO_BUTTON_INPUT_PIN_SEL ((1ULL << M5STACK_LEFT_BUTTON) | (1ULL << M5STACK_MIDDLE_BUTTON | (1ULL << M5STACK_RIGHT_BUTTON)))

M5GFX display;
M5Canvas canvas(&display);
M5Canvas logo_canvas(&canvas);
M5Canvas log_canvas(&canvas);

static constexpr char text0[] = "Xebec Technology";
static constexpr char text1[] = "Beautiful Deburring";
static constexpr char text2[] = "Enjoy life, enjoy working with XEBEC!";
static constexpr char text3[] = "Work smartest!";

static constexpr const char* text[] = {text0, text1, text2, text3};

TaskHandle_t g_handle = nullptr;
static xQueueHandle main_queue = nullptr;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(main_queue, &gpio_num, NULL);
}

static void setupDevice()
{
    gpio_config_t io_conf = {};

    /* SPIFFS設定 (Flash上にファイルシステムを作る）*/
    esp_vfs_spiffs_conf_t fsconf = {
      .base_path = "/spiffs",
      .partition_label = NULL, 
      .max_files = 5,
      .format_if_mount_failed = true
    };
    esp_err_t ret;

    /* SPIFFSの初期化 */
    ret = esp_vfs_spiffs_register(&fsconf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(XEBEC_TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(XEBEC_TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(XEBEC_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        esp_restart();
    }
    ESP_LOGI(XEBEC_TAG, "SPIFFS initialized.");

    display.begin();
    if (display.isEPD()) 
    {
        display.setEpdMode(epd_mode_t::epd_fastest);
        display.invertDisplay(true);
        display.clear(TFT_BLACK);
    }
    if (display.width() < display.height())
    {
        display.setRotation(display.getRotation() ^ 1);
    }
    canvas.createSprite(display.width(), display.height());
    logo_canvas.setColorDepth(16);
    logo_canvas.createSprite(display.width(), display.height() / 3);

    log_canvas.setColorDepth(1);
    log_canvas.createSprite(display.width(), display.height() * 2 / 3);
    log_canvas.setTextSize((float)canvas.width() / 160);
    log_canvas.setTextScroll(true);

    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = GPIO_BUTTON_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(M5STACK_LEFT_BUTTON, gpio_isr_handler, (void*) M5STACK_LEFT_BUTTON);
    gpio_isr_handler_add(M5STACK_MIDDLE_BUTTON, gpio_isr_handler, (void*) M5STACK_MIDDLE_BUTTON);
    gpio_isr_handler_add(M5STACK_RIGHT_BUTTON, gpio_isr_handler, (void*) M5STACK_RIGHT_BUTTON);
}

void mainLoop(void *args)
{
    uint32_t io_num;
    static int count = 0;
    struct stat st;
    const char *logo_file = "/spiffs/xebec_logo.jpg";
    bool logo_existed = false;

    setupDevice();

    /* ロゴファイルが存在すれば描画する */
    if (stat(logo_file, &st) == 0) 
    {
        uint8_t* buf = new uint8_t[st.st_size];
        FILE *f = fopen(logo_file, "r");
        fread(buf, sizeof(char), st.st_size, f);
        logo_canvas.drawJpg(buf, ~0u,
                        0, 0, 
                        display.width(), display.height() / 3,
                        0, 0, 0, 0,
                        datum_t::middle_center);
        logo_existed = true;
        delete[] buf;
    }
    
    while(1) {
        if (xQueueReceive(main_queue, &io_num, 1000 / portTICK_RATE_MS)) 
        {
            if (io_num == M5STACK_LEFT_BUTTON) 
            {
                log_canvas.printf("Left Button is pressed!\r\n");
            }
            else if (io_num == M5STACK_MIDDLE_BUTTON)
            {
                log_canvas.printf("Middle Button is pressed!\r\n");
            }
            else if (io_num == M5STACK_RIGHT_BUTTON)
            {
                log_canvas.printf("Right Button is pressed!\r\n");
            }
        }
        else 
        {
            log_canvas.printf("%s\r\n", text[count & 0x03]);
            ++count;
        }
        if  (logo_existed == true) 
        {
            logo_canvas.pushSprite(0, 0);
        }
        log_canvas.pushSprite(0, display.height() / 3);

        display.startWrite();
        canvas.pushSprite(0,0);
        display.endWrite();
    }
}

void initializeTasks() 
{
    main_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreatePinnedToCore(&mainLoop, "main", 8192, nullptr, 1, &g_handle, 1);

    configASSERT(g_handle);
}

extern "C" void app_main()
{
    initializeTasks();
}