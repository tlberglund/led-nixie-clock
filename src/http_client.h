#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "wifi.h"
#include "lwip/apps/http_client.h"
#include "lwip/apps/http_client.h"
#include "lwip/ip_addr.h" 
#include "lwip/err.h"
#include "lwip/pbuf.h"


#define MAX_HTTP_RESPONSE_SIZE 4096



class HttpClient {
   public:
      void set_wifi_connection(WifiConnection *wifi) { this->wifi = wifi; };

   protected:
      WifiConnection *wifi;


      char response_buffer[MAX_HTTP_RESPONSE_SIZE];
      size_t response_len = 0;

   private:

};


#endif