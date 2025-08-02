#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "FreeRTOS.h"
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
#define MAX_REQUEST_BUFFER_SIZE 512

static const u8_t CA_root_cert[] = {
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

#define CA_ROOT_CERT_LEN (sizeof(CA_root_cert) / sizeof(CA_root_cert[0]))


// typedef err_t (*httpc_headers_done_fn)(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);


typedef struct http_request_context {
   URL_PARTS url_parts;

   httpc_headers_done_fn headerCallback;
   altcp_recv_fn receiveCallback;
   httpc_result_fn resultCallback;

   //  void *callback_arg;

   //  uint16_t port;

#if LWIP_ALTCP && LWIP_ALTCP_TLS
   // TLS configuration, can be null or set to a correctly configured tls configuration.
   // e.g altcp_tls_create_config_client(NULL, 0) would use https without a certificate
   struct altcp_tls_config *tls_config;

   // TLS allocator, used internall for setting TLS server name indication
   //  altcp_allocator_t tls_allocator;
#endif

   u16_t acknowledged_len;

   httpc_connection_t settings;

   bool connected;

   //Overall result of http request, only valid when complete is set
   httpc_result_t result;

   void *http_client;

} http_request_context_t;

typedef void (*body_callback_fn)(http_request_context_t *context, struct pbuf *p);


class HttpClient {
   private:
      http_request_context_t request_context;
      httpc_state_t *http_connection;
      body_callback_fn body_callback;
      TaskHandle_t blockedTaskHandle;
      bool headers_received;
      bool chunking;

      u8_t requestBuffer[MAX_REQUEST_BUFFER_SIZE];
      ip_addr_t ip_addr;

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

      static void dns_callback(const char *name, const ip_addr_t *resolved, void *ipaddr);
      static void altcp_free_config(struct altcp_tls_config *config);
      static void altcp_free_pcb(struct altcp_pcb* pcb);
      static void callback_altcp_err(void *arg, err_t err);
      static err_t callback_altcp_sent(void *arg, struct altcp_pcb *pcb, u16_t len);
      static err_t callback_altcp_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *buf, err_t err);
      static err_t callback_altcp_connect(void *arg, struct altcp_pcb *pcb, err_t err);
      static err_t callback_altcp_poll(void *arg, struct altcp_pcb *pcb);

      err_t make_https_request();
      bool resolve_hostname();
      bool tls_connect(struct altcp_pcb **pcb);
      bool send_get_request(struct altcp_pcb *pcb);

   public:
      HttpClient();
      void set_wifi_connection(WifiConnection *wifi) { this->wifi = wifi; };
      void set_body_callback(body_callback_fn callback) { this->body_callback = callback; }
      err_t get(const char *url,
                body_callback_fn body_callback);

   protected:
      WifiConnection *wifi;
      static BufferPool &bufferPool() { return getBufferPool(); }
};


#endif