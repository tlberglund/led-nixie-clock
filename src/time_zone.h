#ifndef __TIME_ZONE_H__
#define __TIME_ZONE_H__

#include "http_client.h"

#define TIMEZONE_MAX_LENGTH 64


typedef struct timeZoneData {
   char timeZoneName[TIMEZONE_MAX_LENGTH];
   int offset;
   bool dst;
} timeZoneData_t;


class TimeZone : public HttpClient {
   public:
      ~TimeZone();

      static TimeZone& getInstance() {
         static TimeZone instance;
         return instance;
      }

      void init();
      void requestTimeZone();

      timeZoneData_t &getTimeZoneResponse();

   private:
      TimeZone();

      const char *apiUrl = "http://what-time-zone.netlify.app/timezone";
      timeZoneData_t tzData;
      SemaphoreHandle_t replySemaphore;
      TaskHandle_t timeZoneTaskHandle;
      TimerHandle_t timeZoneTimer;

      static void parseReply(char *data, size_t len);

      static void timeZoneTask(void *params);
      static void timerCallback(TimerHandle_t xTimer);

      void setOffset(int offset) { tzData.offset = offset; };
      void setTimezoneName(char *name) { strncpy(tzData.timeZoneName, name, TIMEZONE_MAX_LENGTH); };
      void setDST(bool dst) { tzData.dst = dst; };
};

#endif
