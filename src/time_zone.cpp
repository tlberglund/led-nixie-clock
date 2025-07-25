
#include "time_zone.h"


TimeZone::TimeZone() {
   memset(&tzData, 0, sizeof(timeZoneData_t));
}


TimeZone::~TimeZone() {
   vSemaphoreDelete(replySemaphore);
}


void TimeZone::requestTimeZone() {
   err_t err;

   printf("TimeZone::requestTimeZone() - about to take\n");
   xSemaphoreTake(replySemaphore, 0);
   printf("TimeZone::requestTimeZone() - requesting time zone from %s\n", apiUrl);
   err = get(apiUrl, parseReply);
}


void TimeZone::parseReply(http_request_context_t *context, struct pbuf *p) {
   TimeZone &tz = TimeZone::getInstance();

   char *data = (char *)p->payload;
   size_t len = p->len;

   printf("TimeZone::parseReply() - data=%s, len=%zu\n", data, len);

   char *timeZone, *offset, *dst;
   int charCount = 0;

   timeZone = data;
   for(char *cp = timeZone; *cp != '\n', charCount < len; cp++, charCount++);
   
   if(charCount < len) {
      timeZone[charCount] = '\0';
      tz.setTimezoneName(timeZone);

      offset = timeZone + charCount + 1;
      for(char *cp = offset; *cp != '\n', charCount < len; cp++, charCount++);
      
      if(charCount < len) {
         offset[charCount] = '\0';
         tz.setOffset(atoi(offset));

         dst = timeZone + charCount + 1;
         for(char *cp = dst; *cp != '\n', charCount < len; cp++, charCount++);

         if(charCount < len) {
            dst[charCount] = '\0';
            if(strcmp(dst, "true") == 0) {
               tz.setDST(true);
            }
            else {
               tz.setDST(false);
            }
         }
      }
   }

   xSemaphoreGive(tz.replySemaphore);
}


timeZoneData_t & TimeZone::getTimeZoneResponse() {
   if(xSemaphoreTake(replySemaphore, portMAX_DELAY) == pdTRUE) {
      xSemaphoreGive(replySemaphore);
   }
   return tzData;
}

void TimeZone::init() {
   printf("TimeZone::init()\n");
   replySemaphore = xSemaphoreCreateBinary();
   printf("TimeZone::init() - replySemaphore = %d\n", (int)replySemaphore);
   xTaskCreate(timeZoneTask, "Time Zone Task", 1024, nullptr, 0, nullptr);
}


void TimeZone::timeZoneTask(void *params) {
   printf("TimeZone::timeZoneTask() started\n");
   TimeZone &tz = TimeZone::getInstance();
   tz.wifi->wait_for_wifi_init();
   printf("wifi initialized\n");
   tz.requestTimeZone();
   tz.timeZoneTaskHandle = xTaskGetCurrentTaskHandle();
   tz.timeZoneTimer = xTimerCreate("TimeZoneTimer", pdMS_TO_TICKS(1000 * 5), pdTRUE, 0, timerCallback);
   if(tz.timeZoneTimer != NULL) {
      xTimerStart(tz.timeZoneTimer, 0);
   }

   for(;;) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      printf("TimeZone::timeZoneTask() - TIMER CALLBACK - requesting timezone\n");
      tz.requestTimeZone();
   }
}

void TimeZone::timerCallback(TimerHandle_t xTimer) {
   TimeZone &tz = TimeZone::getInstance();
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   vTaskNotifyGiveFromISR(tz.timeZoneTaskHandle, &xHigherPriorityTaskWoken);
   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
