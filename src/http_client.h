#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "pico.h"
#include "wifi.h"
#include "lwip/apps/http_client.h"
#include "lwip/apps/http_client.h"
#include "lwip/ip_addr.h" 
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/arch.h"
#include "buffer_pool.h"

extern "C" {
   #include "url_parser.h"
}

#define MAX_HTTP_RESPONSE_SIZE 4096


using ReceiveCallback = void (*)(char *, size_t);
using HeadersCallback = void (*)(char *, size_t, u32_t);
using ResultCallback = void (*)(httpc_result_t, u32_t, u32_t);



class HttpClient {
   private:
      httpc_connection_t connection_settings;
      httpc_state_t *http_connection;
      URL_PARTS url_parts;

      static err_t rx_callback(void *arg, 
                               struct altcp_pcb* conn, 
                               struct pbuf* p, 
                               err_t err);

      static err_t headers_done_callback(httpc_state_t *connection, 
                                         void *arg, 
                                         struct pbuf *hdr, 
                                         uint16_t hdr_len, 
                                         u32_t content_len);

      static void result_callback(void *arg, 
                                  httpc_result_t httpc_result, 
                                  u32_t rx_content_len, 
                                  u32_t srv_res, 
                                  err_t err);

      static BufferPool& getBufferPool() {
         static BufferPool instance(3, TCP_MSS);
         return instance;
      }

   public:

      struct CallbackData {
         HttpClient *client;
         ReceiveCallback receive_cb;
         HeadersCallback headers_cb;
         ResultCallback result_cb;
      };

      HttpClient();
      void set_wifi_connection(WifiConnection *wifi) { this->wifi = wifi; };

      err_t get(const char *url,
                ReceiveCallback on_receive = nullptr,
                HeadersCallback on_headers = nullptr,
                ResultCallback on_result = nullptr);

   protected:
      WifiConnection *wifi;
      CallbackData callback_data;
      ReceiveCallback receive_cb;
      HeadersCallback headers_cb;
      ResultCallback result_cb;

      static BufferPool &bufferPool() { return getBufferPool(); }

};


#endif