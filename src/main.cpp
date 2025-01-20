#include <cstdio>
#include "pico/stdlib.h"
#include "secrets.h"
#include "wifi.h"
#include "network_time.h"
#include "apa102.h"
#include "strip_config.h"
#include "pico/aon_timer.h"

extern "C" {
    #include "pico/cyw43_arch.h"
    #include "FreeRTOSConfig.h"
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "timers.h"
}



config_t config = {
    MAGIC_NUMBER,
    WIFI_SSID,
    WIFI_PASSWORD,
    {0, 0, 0},       // padding
    true,            // use DHCP
    {0, 0, 0},       // padding
    false,           // first LED is at the start of the strip
    {0, 0, 0, 0},    // static IP address
    {0, 0, 0, 0},    // static gateway address
    {0, 0, 0, 0},    // static netmask
    {0, 0, 0},       // padding
    false,           // 24-hour clock
    0x00000000       // CRC (not used right now)
};


#define DIGIT_LED_COUNT 50
#define DIGIT_ROW_WIDTH 5
#define NUM_DIGITS 6

WifiConnection& wifi = WifiConnection::getInstance();
NetworkTime& network_time = NetworkTime::getInstance();

APA102 leds_hours_10(DIGIT_LED_COUNT, 12, 13, pio0, 0);
APA102 leds_hours_1(DIGIT_LED_COUNT, 10, 11, pio0, 1);
APA102 leds_minutes_10(DIGIT_LED_COUNT, 8, 9, pio0, 2);
APA102 leds_minutes_1(DIGIT_LED_COUNT, 6, 7, pio0, 3);
APA102 leds_seconds_10(DIGIT_LED_COUNT, 4, 5, pio1, 0);
APA102 leds_seconds_1(DIGIT_LED_COUNT, 2, 3, pio1, 1);

static TaskHandle_t led_task_handle;

TimerHandle_t xFrameTimer;
TaskHandle_t xFrameTask;

void vTimerCallback(TimerHandle_t xTimer) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xFrameTask, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

APA102_LED digit_pattern[DIGIT_ROW_WIDTH * NUM_DIGITS];

void set_digit(APA102 *strip, APA102_LED *pattern, uint8_t digit) {
    strip->clear_strip();
    for(uint8_t n = 0; n < DIGIT_ROW_WIDTH; n++) {
        strip->set_led(n + (digit * DIGIT_ROW_WIDTH), pattern[n]);
    }
}


void led_task(void *pvParameters) {
    struct tm currentTime;
    WifiConnection *wifi = (WifiConnection *)pvParameters;
    uint8_t hours_10, hours_1, minutes_10, minutes_1, seconds_10, seconds_1;

    printf("LED TASK STARTED, WAITING FOR WIFI\n");
    wifi->wait_for_wifi_init();
    printf("LED TASK WIFI IS UP\n");

    xFrameTask = xTaskGetCurrentTaskHandle();
    xFrameTimer = xTimerCreate("FrameTimer", pdMS_TO_TICKS(40), pdTRUE, 0, vTimerCallback);
    if(xFrameTimer != NULL) {
        xTimerStart(xFrameTimer, 0);
    }

    // Lame digit pattern for now
    for(uint8_t n = 0; n < DIGIT_ROW_WIDTH * NUM_DIGITS; n++) {
        digit_pattern[n].red = 255;
        digit_pattern[n].green = 255;
        digit_pattern[n].blue = 255;
        digit_pattern[n].brightness = 2;
    }

    uint32_t frame = 0;
    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        printf("FRAME %d\n", frame);

        aon_timer_get_time_calendar(&currentTime);
        hours_10 = currentTime.tm_hour / 10;
        hours_1 = currentTime.tm_hour % 10;
        minutes_10 = currentTime.tm_min / 10;
        minutes_1 = currentTime.tm_min % 10;
        seconds_10 = currentTime.tm_sec / 10;
        seconds_1 = currentTime.tm_sec % 10;

        set_digit(&leds_hours_10,   digit_pattern + (DIGIT_ROW_WIDTH * 0 * sizeof(APA102_LED)), hours_10);
        set_digit(&leds_hours_1,    digit_pattern + (DIGIT_ROW_WIDTH * 1 * sizeof(APA102_LED)), hours_1);
        set_digit(&leds_minutes_10, digit_pattern + (DIGIT_ROW_WIDTH * 2 * sizeof(APA102_LED)), minutes_10);
        set_digit(&leds_minutes_1,  digit_pattern + (DIGIT_ROW_WIDTH * 3 * sizeof(APA102_LED)), minutes_1);
        set_digit(&leds_seconds_10, digit_pattern + (DIGIT_ROW_WIDTH * 4 * sizeof(APA102_LED)), seconds_10);
        set_digit(&leds_seconds_1,  digit_pattern + (DIGIT_ROW_WIDTH * 5 * sizeof(APA102_LED)), seconds_1);

        frame++;
    }
}



void launch() {

    wifi.set_ssid(WIFI_SSID);
    wifi.set_password(WIFI_PASSWORD);

    printf("STARTING CYW43/WIFI INITIALIZATION\n");
    wifi.init();

    printf("STARTING NTP SYNC\n");
    network_time.sntp_set_timezone(-7);
    network_time.set_wifi_connection(&wifi);
    network_time.init();

    xTaskCreate(led_task, "LED Data Task", 1024, &wifi, 1, &led_task_handle);

    vTaskStartScheduler();
}


int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("UP\n");
    sleep_ms(500);

    printf("LAUNCHING\n");
    launch();
}
