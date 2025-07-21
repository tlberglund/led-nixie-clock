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


// typedef err_t (*httpc_headers_done_fn)(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);


typedef struct http_request_context {
   URL_PARTS url_parts;

   httpc_headers_done_fn headerCallback;
   altcp_recv_fn receiveCallback;
   httpc_result_fn resultCallback;

    void *callback_arg;

    uint16_t port;

#if LWIP_ALTCP && LWIP_ALTCP_TLS
   // TLS configuration, can be null or set to a correctly configured tls configuration.
   // e.g altcp_tls_create_config_client(NULL, 0) would use https without a certificate
   struct altcp_tls_config *tls_config;

   // TLS allocator, used internall for setting TLS server name indication
    altcp_allocator_t tls_allocator;
#endif

   httpc_connection_t settings;

   bool complete;

   //Overall result of http request, only valid when complete is set
   httpc_result_t result;

   void *http_client;

} http_request_context_t;

typedef void (*body_callback_fn)(http_request_context_t *context, struct pbuf *p);


class HttpClient {
   private:
      http_request_context_t requestContext;
      httpc_state_t *http_connection;
      body_callback_fn bodyCallback;

      static err_t rx_callback(void *arg, 
                               struct altcp_pcb *conn, 
                               struct pbuf *p, 
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
      HttpClient();
      void set_wifi_connection(WifiConnection *wifi) { this->wifi = wifi; };
      void set_body_callback(body_callback_fn callback) { this->bodyCallback = callback; }
      err_t get(const char *url,
                body_callback_fn body_callback);

   protected:
      WifiConnection *wifi;

      static BufferPool &bufferPool() { return getBufferPool(); }

};


#endif