#include <M5GFX.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define M5STACK_LEFT_BUTTON   GPIO_NUM_39
#define M5STACK_MIDDLE_BUTTON GPIO_NUM_38
#define M5STACK_RIGHT_BUTTON  GPIO_NUM_37

#define GPIO_BUTTON_INPUT_PIN_SEL ((1ULL << M5STACK_LEFT_BUTTON) | (1ULL << M5STACK_MIDDLE_BUTTON | (1ULL << M5STACK_RIGHT_BUTTON)))

M5GFX display;
M5Canvas canvas(&display);

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
    canvas.setColorDepth(1);
    canvas.createSprite(display.width(), display.height());
    canvas.setTextSize((float)canvas.width() / 160);
    canvas.setTextScroll(true);

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
    setupDevice();
    
    while(1) {
        if (xQueueReceive(main_queue, &io_num, 1000 / portTICK_RATE_MS)) 
        {
            if (io_num == M5STACK_LEFT_BUTTON) 
            {
                canvas.printf("Left Button is pressed!\r\n");
            }
            else if (io_num == M5STACK_MIDDLE_BUTTON)
            {
                canvas.printf("Middle Button is pressed!\r\n");
            }
            else if (io_num == M5STACK_RIGHT_BUTTON)
            {
                canvas.printf("Right Button is pressed!\r\n");
            }
            canvas.pushSprite(0,0);
        }
        else 
        {
            canvas.printf("%s\r\n", text[count & 0x03]);
            canvas.pushSprite(0,0);
            ++count;
        }
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