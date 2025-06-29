#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "wifi.h"
#include "lwip/apps/http_client.h"


#define MAX_HTTP_RESPONSE_SIZE 4096


typedef struct http_request {
    const char *hostname;
    const char *url;
    httpc_headers_done_fn headers_fn;
    altcp_recv_fn recv_fn;
    httpc_result_fn result_fn;
    void *callback_arg;
    uint16_t port;
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    struct altcp_tls_config *tls_config;
    altcp_allocator_t tls_allocator;
#endif
    httpc_connection_t settings;
    int complete;
    httpc_result_t result;
} http_request_t;


class HttpClient {
   public:
      void set_wifi_connection(WifiConnection *wifi) { this->wifi = wifi; };

   protected:
      WifiConnection *wifi;

      int http_client_request_async(struct async_context *context, http_request_t *req);
      int http_client_request_sync(struct async_context *context, http_request_t *req);
      err_t http_client_header_print_fn(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);
      err_t http_client_receive_print_fn(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err);

      char response_buffer[MAX_HTTP_RESPONSE_SIZE];
      size_t response_len = 0;

   private:

};


#endif