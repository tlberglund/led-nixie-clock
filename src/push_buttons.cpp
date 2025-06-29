

#include <list>
#include "FreeRTOS.h"
#include "task.h"
#include "push_buttons.h"


TimerHandle_t button_timer_handle;
TaskHandle_t button_task_handle;


void button_timer_callback(TimerHandle_t xTimer) {
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   vTaskNotifyGiveFromISR(button_task_handle, &xHigherPriorityTaskWoken);
   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void button_task(void *pvParameters) {
   PushButtons *buttons = (PushButtons *)pvParameters;

   printf("Button task started\n");
   gpio_init(19);
   gpio_set_dir(19, GPIO_OUT);
   gpio_pull_up(19);

   button_task_handle = xTaskGetCurrentTaskHandle();
   button_timer_handle = xTimerCreate("FrameTimer", pdMS_TO_TICKS(5), pdTRUE, 0, button_timer_callback);
   if(button_timer_handle != NULL) {
      xTimerStart(button_timer_handle, 0);
   }

   printf("Button timer created, starting loop\n");

   bool toggle = false;

   for(;;) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

      gpio_put(19, toggle);
      toggle = !toggle;

      auto iterator = buttons->buttons.begin();
      auto last = buttons->buttons.end();
      while(iterator != last) {
         PushButton *button = *iterator;
         if(button) {
            button->process_tick();
         }
         iterator++;
      }
   }
}

void PushButtons::init() {
   printf("PushButtons init\n");
   xTaskCreate(button_task, "Button Task", 1024, this, 1, NULL);
}

PushButtons::PushButtons() {
}


