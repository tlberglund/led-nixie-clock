#ifndef __TIME_ZONE_H__
#define __TIME_ZONE_H__

#include "http_client.h"



class TimeZone : public HttpClient {
   public:
      void set_timezone(char *tz) { timezone_name = tz; }

      static TimeZone& getInstance() {
         static TimeZone instance;

         return instance;
      }


      // "http://worldtimeapi.org/api/ip.txt"
   private:
      TimeZone() {};
      char *timezone_name;


};

#endif
