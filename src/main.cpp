#include <cstdio>
#include "pico/stdlib.h"
#include "secrets.h"
#include "wifi.h"
#include "network_time.h"
#include "apa102.h"
#include "clock.h"
#include "strip_config.h"
#include "pico/aon_timer.h"

extern "C" {
    #include "pico/cyw43_arch.h"
    #include "FreeRTOSConfig.h"
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "timers.h"
    #include "hardware/adc.h"
    #include "hardware/gpio.h"
}


config_t config = {
    MAGIC_NUMBER,
    WIFI_SSID,
    WIFI_PASSWORD,
    "America/Los_Angeles",
    -480,            // TZ offset (minutes)
    {0, 0},          // padding
    true,            // use DHCP
    {0, 0, 0},       // padding
    true,            // 24-hour clock
    {0, 0, 0},       // padding
    {0, 0, 0, 0},    // static IP address
    {0, 0, 0, 0},    // static gateway address
    {0, 0, 0, 0},    // static netmask
    0x00000000       // CRC (not used right now)
};


WifiConnection& wifi = WifiConnection::getInstance();
NetworkTime& network_time = NetworkTime::getInstance();
Clock nixie_clock;

APA102_LED digit_pattern[NUM_DIGITS][DIGIT_ROW_WIDTH];

static TaskHandle_t led_task_handle;
TimerHandle_t xFrameTimer;
TaskHandle_t xFrameTask;

TimerHandle_t xStatusTimer;
TaskHandle_t xStatusTask;

APA102 status_leds(3, 16, 17, pio2, 0);
#define STATUS_LED_CURRENT 0
#define STATUS_LED_NETWORK 1

// Hardware is rated for 50W max at 5V
#define MAX_CURRENT 10.0 

#define GPIO_CURRENT_SENSOR 26
#define GPIO_STATUS_LED 22

#define BLINK_SAMPLES 50
uint8_t blink_curve[] = { 0, 0, 0, 0, 0, 1, 4, 15, 46, 109, 192, 255, 255, 192, 109, 46, 15, 4, 1, 0, 0, 0, 0, 0, 0, 
                          0, 0, 0, 0, 0, 0, 0,  0,  0,   0,   0,   0,   0,   0,   0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0 };



#define SHORT_FLICKER_FRAME_COUNT 12
#define LONG_FLICKER_FRAME_COUNT 25
float short_flicker[] = {0.95, 0.75, 0.90, 0.20, 0.60, 0.10, 0.00, 0.05, 0.70, 0.85, 0.40, 0.95};
float long_flicker[] = {0.95, 0.90, 0.85, 0.80, 0.70, 0.30, 0.10, 0.05, 0.00, 0.00, 0.02, 0.50, 0.30, 0.10, 0.40, 0.60, 0.70, 0.65, 0.30, 0.10, 0.50, 0.80, 0.85, 0.90, 0.95};
typedef struct flicker {
   uint64_t last_flicker_time;
   uint64_t next_flicker_time;
   uint32_t flicker_frame_count;
   uint32_t flicker_frame_index;
   float *this_flicker;
   bool flickering;
   bool last_flicker_long;
   bool this_flicker_long;
} flicker_t;

flicker_t digit_flicker[NUM_DIGITS];

float rand_gaussian(void) {
   float sum = 0.0f;
   for(int i = 0; i < 12; i++) {
       sum += (float)rand() / RAND_MAX;
   }
   return sum - 6.0f;
}


void vFrameTimerCallback(TimerHandle_t xTimer) {
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   vTaskNotifyGiveFromISR(xFrameTask, &xHigherPriorityTaskWoken);
   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void vStatusTimerCallback(TimerHandle_t xTimer) {
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   vTaskNotifyGiveFromISR(xStatusTask, &xHigherPriorityTaskWoken);
   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void status_task(void *pvParameters) {
   printf("STATUS TASK STARTED\n");
   WifiConnection *wifi = (WifiConnection *)pvParameters;

   adc_init();
   adc_gpio_init(GPIO_CURRENT_SENSOR);

   gpio_init(GPIO_STATUS_LED);
   gpio_set_dir(GPIO_STATUS_LED, GPIO_OUT);
   gpio_set_drive_strength(10, GPIO_DRIVE_STRENGTH_2MA);

   xStatusTask = xTaskGetCurrentTaskHandle();
   xStatusTimer = xTimerCreate("StatusTimer", pdMS_TO_TICKS(40), pdTRUE, 0, vStatusTimerCallback);
   if(xStatusTimer != NULL) {
       xTimerStart(xStatusTimer, 0);
   }

   status_leds.set_led(STATUS_LED_CURRENT, 0, 0, 0, 1);
   status_leds.set_led(STATUS_LED_NETWORK, 0, 0, 0, 1);
   status_leds.update_strip();

   float current_draw = 0.0f;
   uint16_t current_adc_raw;
   uint8_t network_blink_index = 0;

   for(;;) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

      adc_select_input(0);
      current_adc_raw = adc_read();
      printf("CURRENT RAW=%d\n", current_adc_raw);
      current_draw = current_adc_raw * 3.3f / 4096.0f; // This is from Copilot, WTF
      printf("CURRENT(A)=%f\n", current_draw);

      // The closer we are to the max current, the brighter red the current LED is--
      // meaning we use even more current. But hey, it's fine.
      uint8_t current_red = (current_draw / MAX_CURRENT) * 255;
      uint8_t current_brightness = (current_draw / MAX_CURRENT) * 31;
      status_leds.set_led(STATUS_LED_CURRENT, current_red, 0, 0, current_brightness);

      switch(wifi->get_state()) {
         case ConnectionState::INIT:
            // dim orange
            status_leds.set_led(STATUS_LED_NETWORK, 245, 122, 15, 2);
            break;

         case ConnectionState::CY43_INIT:
            // bright orange
            status_leds.set_led(STATUS_LED_NETWORK, 245, 122, 15, 15);
            break;

         case ConnectionState::CONNECTED:
            // dim blue
            status_leds.set_led(STATUS_LED_NETWORK, 0, 0, 64, 2);
            break;

         case ConnectionState::CONNECTING:
            // pulsing blue
            status_leds.set_led(STATUS_LED_NETWORK, 0, 0, blink_curve[network_blink_index], 15);
            break;

         case ConnectionState::DISCONNECTED:
            // pulsing red
            status_leds.set_led(STATUS_LED_NETWORK, blink_curve[network_blink_index], 0, 0, 15); 
            break;

         case ConnectionState::FAILED:
            // bright red
            status_leds.set_led(STATUS_LED_NETWORK, 255, 0, 0, 15); 
            break;

         default:
            printf("UNKNOWN NETWORK STATE\n");
            break;
      }

      status_leds.update_strip();
          
      network_blink_index++;
      if(network_blink_index >= BLINK_SAMPLES) {
         network_blink_index = 0;
      }
   }
}


void clock_task(void *pvParameters) {
   struct tm currentTime;
   WifiConnection *wifi = (WifiConnection *)pvParameters;

   printf("LED TASK STARTED, NOT WAITING FOR WIFI\n");

   xFrameTask = xTaskGetCurrentTaskHandle();
   xFrameTimer = xTimerCreate("FrameTimer", pdMS_TO_TICKS(40), pdTRUE, 0, vFrameTimerCallback);
   if(xFrameTimer != NULL) {
      xTimerStart(xFrameTimer, 0);
   }

   // amber: 255, 179, 51
   // amber: 255, 191, 64
   uint8_t brightness[5] = {3, 15, 31, 15, 3};
   uint8_t red = 230, green = 102, blue = 23;
   for(int digit = 0; digit < NUM_DIGITS; digit++) {
      for(int led = 0; led < DIGIT_ROW_WIDTH; led++) {
         digit_pattern[digit][led].red = red;
         digit_pattern[digit][led].green = green;
         digit_pattern[digit][led].blue = blue;
         digit_pattern[digit][led].brightness = brightness[led];
      }
      nixie_clock.set_digit_pattern((clock_digit_t)digit, digit_pattern[digit]);
   }

   uint64_t now = time_us_64();
   for(int n = 0; n < NUM_DIGITS; n++) {
      digit_flicker[n].last_flicker_time = 0;
      digit_flicker[n].next_flicker_time = now + (rand_gaussian() * 1000000);
      digit_flicker[n].flicker_frame_count = 0;
      digit_flicker[n].flicker_frame_index = 0;
      digit_flicker[n].flickering = false;
      digit_flicker[n].last_flicker_long = false;
      digit_flicker[n].this_flicker = NULL;
   }


   uint32_t frame = 0;

   for(;;) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      now = time_us_64();

      aon_timer_get_time_calendar(&currentTime);

      for(int digit = 0; digit < NUM_DIGITS; digit++) {
         flicker_t *flicker = &digit_flicker[digit];
         if(flicker->flickering == false) {
            if(flicker->next_flicker_time < now) {
               flicker->flickering = true;
               flicker->flicker_frame_index = 0;
               float next_flicker_length_rand = abs(rand_gaussian());
               bool next_flicker_long;
               if(flicker->last_flicker_long) {
                  flicker->this_flicker_long = next_flicker_length_rand < 1.5;
               }
               else {
                  flicker->this_flicker_long = next_flicker_length_rand < 0.8;
               }
               if(flicker->this_flicker_long) {
                  flicker->flicker_frame_count = LONG_FLICKER_FRAME_COUNT;
                  flicker->this_flicker = long_flicker;
               }
               else {
                  flicker->flicker_frame_count = SHORT_FLICKER_FRAME_COUNT;
                  flicker->this_flicker = short_flicker;
               }
            }
         }
         else {
            float flicker_multiplier = flicker->this_flicker[flicker->flicker_frame_index++];

            for(int led = 0; led < DIGIT_ROW_WIDTH; led++) {
               digit_pattern[digit][led].red = red * flicker_multiplier;
               digit_pattern[digit][led].green = green * flicker_multiplier;
               digit_pattern[digit][led].blue = blue * flicker_multiplier;
               digit_pattern[digit][led].brightness = brightness[led] * flicker_multiplier;
            }
            nixie_clock.set_digit_pattern((clock_digit_t)digit, digit_pattern[digit]);

            if(flicker->flicker_frame_index >= flicker->flicker_frame_count) {
               flicker->flickering = false;
               flicker->last_flicker_time = now;
               flicker->next_flicker_time = now + (rand_gaussian() * 50000000);
               flicker->last_flicker_long = flicker->this_flicker_long;
               flicker->this_flicker = NULL;
            }
         }
      }

      nixie_clock.set_time(&currentTime);

      frame++;
   }
}


void launch() {
   wifi.set_ssid(WIFI_SSID);
   wifi.set_password(WIFI_PASSWORD);

   printf("STARTING CYW43/WIFI INITIALIZATION\n");
   wifi.init();

   printf("STARTING NTP SYNC\n");
   // network_time.set_timezone(0, config.tz_offset_minutes);
   // network_time.set_timezone("America/Los_Angeles");
   network_time.set_wifi_connection(&wifi);
   network_time.init();

   xTaskCreate(clock_task, "LED Data Task", 1024, &wifi, 1, &led_task_handle);
   xTaskCreate(status_task, "Status Task", 1024, &wifi, 0, NULL);

   vTaskStartScheduler();
}


int main() {
   stdio_init_all();
   // sleep_ms(1000);
   // printf("UP\n");
   // sleep_ms(500);

   // printf("LAUNCHING\n");
   launch();
}
