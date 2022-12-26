#include <M5GFX.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

M5GFX display;
M5Canvas canvas(&display);

static constexpr char text0[] = "Xebec Technology";
static constexpr char text1[] = "Beautiful Deburring";
static constexpr char text2[] = "Enjoy life, enjoy working with XEBEC!";
static constexpr char text3[] = "Work smartest!";

static constexpr const char* text[] = {text0, text1, text2, text3};

extern "C" void app_main()
{
    int count = 0;

    display.begin();
    if (display.isEPD())
    {
        display.setEpdMode(epd_mode_t::epd_fastest);
        display.invertDisplay(true);
        display.clear(TFT_BLACK);
    }
    if (display.width() < display.height()) {
        display.setRotation(display.getRotation() ^ 1);
    }
    canvas.setColorDepth(1);
    canvas.createSprite(display.width(), display.height());
    canvas.setTextSize((float)canvas.width()/ 160);
    canvas.setTextScroll(true);

    
    while(1) {
        canvas.printf("%s\r\n", text[count & 0x03]);
        canvas.pushSprite(0,0);
        ++count;
    }

}