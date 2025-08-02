

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include "http_client.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "altcp_tls_mbedtls_structs.h"
#include "mbedtls/debug.h"


HttpClient::HttpClient() : http_connection(nullptr)
{
   memset(&request_context, 0, sizeof(request_context));
   request_context.resultCallback = result_callback;
   request_context.headerCallback = headers_done_callback;
   request_context.receiveCallback = rx_callback;
   request_context.settings.use_proxy = 0;
   request_context.http_client = this;
}


err_t HttpClient::get(const char *url,
                      body_callback_fn body_callback)
{
   printf("HttpClient::get() - URL: %s\n", url);

   if(!parse_url((char *)url, &request_context.url_parts))
   {
      printf("HttpClient::get() - Failed to parse URL: %s\n", url);
      return ERR_ARG;
   }

   set_body_callback(body_callback);

   if(request_context.url_parts.port_number < 0)
   {
      if(strcmp(request_context.url_parts.scheme, "https") == 0)
      {
         printf("HttpClient::get() - No port number specified for scheme '%s', defaulting to 443\n", request_context.url_parts.scheme);
         request_context.url_parts.port_number = 443;
      }
      else
      {
         printf("HttpClient::get() - No port number specified for scheme '%s', defaulting to 80\n", request_context.url_parts.scheme);
         request_context.url_parts.port_number = 80;
      }
   }

   // print_url(&url_parts);

   request_context.connected = false;
   request_context.settings.headers_done_fn = request_context.headerCallback;
   request_context.settings.result_fn = request_context.resultCallback;
   printf("CALLING httpc_get_file_dns() with request_context=%p\n", &request_context);
   printf("body_callback: %p\n", body_callback);

   err_t ret;
   if(request_context.url_parts.port_number == 443) {
      ret = make_https_request();
   }
   else {
      async_context_acquire_lock_blocking(cyw43_arch_async_context());
      ret = httpc_get_file_dns(request_context.url_parts.authority, 
                               request_context.url_parts.port_number, 
                               request_context.url_parts.path, 
                               &request_context.settings,
                               request_context.receiveCallback, 
                               &request_context, 
                               NULL);
      async_context_release_lock(cyw43_arch_async_context());
   }

   if(ret != ERR_OK) {
      printf("http request failed: %d", ret);
   }
   return ret;
}


void HttpClient::dns_callback(const char *name, const ip_addr_t *resolved, void *arg) {
   HttpClient *http_client = static_cast<HttpClient *>(arg);
   if(resolved) {
      http_client->ip_addr = *resolved;
      xTaskNotifyGive(http_client->blockedTaskHandle);
   }
   else {
      http_client->ip_addr.addr = IPADDR_NONE;
   }
}


bool HttpClient::resolve_hostname()
{
   ip_addr.addr = IPADDR_ANY;
   char *hostname = request_context.url_parts.authority;

   printf("HttpClient::resolve_hostname(): resolving %s\n", hostname);
   cyw43_arch_lwip_begin();
   err_t lwip_err = dns_gethostbyname(hostname, &ip_addr, dns_callback, this);
   printf("dns_gethostbyname() is back\n");
   cyw43_arch_lwip_end();

   // dns_callback() will unblock us
   blockedTaskHandle = xTaskGetCurrentTaskHandle();
   ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(3000));
   if(ip_addr.addr == IPADDR_ANY || ip_addr.addr == IPADDR_NONE) {
      printf("HttpClient::resolve_hostname(): DNS resolution failed for %s\n", hostname);
      return false;
   }

   printf("HttpClient::resolve_hostname(): resolved %s to %08x (%d.%d.%d.%d)\n",
          hostname, 
          ip_addr.addr,
          (ip_addr.addr & 0xff),
          (ip_addr.addr >> 8) & 0xff,
          (ip_addr.addr >> 16) & 0xff,
          (ip_addr.addr >> 24) & 0xff);
   
   return true;
}



void HttpClient::altcp_free_pcb(struct altcp_pcb *pcb) {
    cyw43_arch_lwip_begin();
    err_t lwip_err = altcp_close(pcb);
    cyw43_arch_lwip_end();

    // If at first you don't succeed, sleep for 100ms and try again
    while(lwip_err != ERR_OK) {
        vTaskDelay(pdMS_TO_TICKS(100));
        cyw43_arch_lwip_begin();
        lwip_err = altcp_close(pcb);
        cyw43_arch_lwip_end();
    }
}


void HttpClient::altcp_free_config(struct altcp_tls_config *config) {
   cyw43_arch_lwip_begin();
   altcp_tls_free_config(config);
   cyw43_arch_lwip_end();
}


void HttpClient::callback_altcp_err(void *arg, err_t err) {
   printf("HttpClient::callback_altcp_err() - arg=%p, err=%d (%s)\n", arg, err, lwip_strerr(-err));
   if(((http_request_context_t *)arg)->tls_config) {
      altcp_free_config(((http_request_context_t *)arg)->tls_config);
   }
}


err_t HttpClient::callback_altcp_sent(void *arg, struct altcp_pcb *pcb, u16_t len) {
   printf("HttpClient::callback_altcp_sent() acknowledged %d bytes\n", pcb, len);
   http_request_context_t *context = static_cast<http_request_context_t *>(arg);
   HttpClient *http_client = static_cast<HttpClient *>(context->http_client);

   context->acknowledged_len = len;
   xTaskNotifyGive(http_client->blockedTaskHandle);  
   return ERR_OK;
}



// TCP + TLS data reception callback
err_t HttpClient::callback_altcp_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *buf, err_t err) {

   printf("HttpClient::callback_altcp_recv() - pcb=%p, buf=%p, err=%d\n", pcb, buf, err);

   switch(err) {
      case ERR_OK:

         // Handle packet buffer chain
         //
         //  * buf->tot_len == buf->len — Last buf in chain
         //    * && buf->next == NULL — last buf in chain, no packets in queue
         //    * && buf->next != NULL — last buf in chain, more packets in queue
         //

         if(buf) {
            while(buf->len != buf->tot_len) {
               for(u16_t i = 0; i < buf->len; i++) {
                  putchar(((char*)buf->payload)[i]);
               }
               buf = buf->next;
            }

            for(u16_t i = 0; i < buf->len; i++) {
               putchar(((char*)buf->payload)[i]);
            }
            assert(buf->next == NULL);

            altcp_recved(pcb, buf->tot_len);

            //TODO: figure out if this is headers or body, parse headers(?) and call header handler
            //TODO: figure out if we're doing chunked, and if so, when we're done
            //TODO: figure out if we got a content-length and if we're done
            //TODO: call actual application receive callback


         }
         // fall through

      case ERR_ABRT:
         if(buf) {
            pbuf_free(buf);
         }
         err = ERR_OK;
         break;
   }

   // Return error
   return err;
}


err_t HttpClient::callback_altcp_connect(void *arg, struct altcp_pcb *pcb, err_t err) {
   http_request_context_t *context = static_cast<http_request_context_t *>(arg);
   HttpClient *http_client = static_cast<HttpClient *>(context->http_client);
   context->connected = true;
   xTaskNotifyGive(http_client->blockedTaskHandle);
   return ERR_OK;
}


err_t HttpClient::callback_altcp_poll(void *arg, struct altcp_pcb *pcb) {
    // Callback not currently used
    return ERR_OK;
}


bool HttpClient::tls_connect(struct altcp_pcb **pcb) {

   // LMAO MitM attacks
   cyw43_arch_lwip_begin();
   struct altcp_tls_config *config = altcp_tls_create_config_client(NULL, 0);
   // struct altcp_tls_config *config = altcp_tls_create_config_client(CA_root_cert, CA_ROOT_CERT_LEN);
   cyw43_arch_lwip_end();
   if(config == NULL) {
      return false;
   }
   request_context.tls_config = config;

   // TODO: mbedtls_ssl_conf_dbg();
   // mbedtls_ssl_conf_dbg(&(config->conf), NULL, stdout);

   cyw43_arch_lwip_begin();
   *pcb = altcp_tls_new(config, IPADDR_TYPE_V4);
   cyw43_arch_lwip_end();
   if(*pcb == NULL) {
      altcp_free_config(config);
      return false;
   }

    // Configure hostname for Server Name Indication extension
   cyw43_arch_lwip_begin();
   altcp_mbedtls_state_t *mbedtls_state = (altcp_mbedtls_state_t *)((*pcb)->state);
   err_t mbedtls_err = mbedtls_ssl_set_hostname(&mbedtls_state->ssl_context,
                                                request_context.url_parts.authority);
   cyw43_arch_lwip_end();
   if(mbedtls_err) {
      altcp_free_pcb(*pcb);
      altcp_free_config(config);
      return false;
   }

   // TODO: should be able to remove this; it's already done
   request_context.connected = false;

   cyw43_arch_lwip_begin();
   altcp_arg(*pcb, (void *)&request_context);
   cyw43_arch_lwip_end();

   cyw43_arch_lwip_begin();
   altcp_err(*pcb, callback_altcp_err);
   cyw43_arch_lwip_end();

   cyw43_arch_lwip_begin();
   altcp_poll(*pcb, callback_altcp_poll, 100);
   cyw43_arch_lwip_end();

   cyw43_arch_lwip_begin();
   altcp_sent(*pcb, callback_altcp_sent);
   cyw43_arch_lwip_end();

   cyw43_arch_lwip_begin();
   altcp_recv(*pcb, callback_altcp_recv);
   cyw43_arch_lwip_end();

   cyw43_arch_lwip_begin();
   err_t lwip_err = altcp_connect(*pcb,
                                  &ip_addr,
                                  request_context.url_parts.port_number,
                                  callback_altcp_connect);
   cyw43_arch_lwip_end();

   // callback_altcp_connect() will unblock us, or we'll time out
   blockedTaskHandle = xTaskGetCurrentTaskHandle();
   ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));

   if(request_context.connected) {
      printf("HttpClient::tls_connect() - CONNECTION MADE\n");
      return true;
   }
   else {
      altcp_free_pcb(*pcb);
      altcp_free_config(config);
      return false;
   }
}


static const char *http_get_request_template = 
   "GET %s HTTP/1.1\r\n"
   "Host: %s\r\n"
   "User-Agent: raspberry-pi-pico-http-client\r\n"
   "Accept: application/json, text/plain\r\n"
   "Accept-Encoding: identity\r\n"
   "\r\n";

bool HttpClient::send_get_request(struct altcp_pcb *pcb) {
   int request_len;
   
   request_len = snprintf((char *)requestBuffer, 
                          MAX_REQUEST_BUFFER_SIZE, 
                          http_get_request_template,
                          request_context.url_parts.path,
                          request_context.url_parts.authority);

    // Check send buffer and queue length
    //
    //  Docs state that altcp_write() returns ERR_MEM on send buffer too small
    //  _or_ send queue too long. Could either check both before calling
    //  altcp_write, or just handle returned ERR_MEM — which is preferable?
    //
    //if(
    //  altcp_sndbuf(pcb) < (LEN(request) - 1)
    //  || altcp_sndqueuelen(pcb) > TCP_SND_QUEUELEN
    //) return -1;

   headers_received = false;

   // Write to send buffer
   cyw43_arch_lwip_begin();
   err_t lwip_err = altcp_write(pcb, requestBuffer, request_len, 0);
   cyw43_arch_lwip_end();

   // Written to send buffer
   if(lwip_err == ERR_OK) {
      request_context.acknowledged_len = 0;
      cyw43_arch_lwip_begin();
      lwip_err = altcp_output(pcb);
      cyw43_arch_lwip_end();

      // callback_altcp_sent() will unblock us
      blockedTaskHandle = xTaskGetCurrentTaskHandle();
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

      if(request_context.acknowledged_len != request_len) {
         return false;
      }
      else {
         return true;
      }
   }
   else {
      return false;
   }
}


err_t HttpClient::make_https_request()
{
   printf("HttpClient::make_https_request()\n");
   char *char_ipaddr;

   printf("make_https_request(): resolving %s\n", request_context.url_parts.authority);
   if(!resolve_hostname()) {
      printf("Failed to resolve %s\n", request_context.url_parts.authority);
      return HTTPC_RESULT_ERR_HOSTNAME;
   }

   // ipaddr_ntoa() is ruthlessly thread-unsafe in every implementation I have ever seen
   cyw43_arch_lwip_begin();
   char_ipaddr = ipaddr_ntoa(&ip_addr);
   cyw43_arch_lwip_end();
   printf("make_https_request(): resolved %s to %s\n", request_context.url_parts.authority, char_ipaddr);

#ifdef MBEDTLS_DEBUG_C
   mbedtls_debug_set_threshold(3);
#endif

   struct altcp_pcb *pcb = NULL;
   printf("Connecting to %s://%s:%d\n", request_context.url_parts.scheme, char_ipaddr, request_context.url_parts.port_number);
   if(!tls_connect(&pcb)) {
      printf("FAILED TO CONNECT to https://%s:%d\n", char_ipaddr, request_context.url_parts.port_number);
      return HTTPC_RESULT_ERR_CONNECT;
   }
   printf("CONNECTED\n");

   // Send HTTP request to server
   printf("Sending request\n");
   if(!send_get_request(pcb)) {
      printf("Failed to send GET request\n");
      altcp_free_config(((http_request_context_t *)(pcb->arg))->tls_config);
      altcp_free_pcb(pcb);
      return HTTPC_RESULT_ERR_CLOSED;
   }
   printf("Request sent\n");

   return ERR_OK;
}


err_t HttpClient::rx_callback(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
   http_request_context_t *context = static_cast<http_request_context_t *>(arg);
   if(!context)
   {
      if(p != NULL) {
         pbuf_free(p);
      }
      return ERR_OK;
   }

   if(err != ERR_OK)
   {
      if(p != NULL) {
         pbuf_free(p);
      }
      return err;
   }

   if(p != NULL)
   {
      if(context->receiveCallback)
      {
         // char *buffer = (char *)bufferPool().allocate();

         // u16_t copy_len = std::min((size_t)p->tot_len, bufferPool().get_buffer_size() - 1);
         // pbuf_copy_partial(p, buffer, copy_len, 0);
         // buffer[copy_len] = '\0';

         printf("%s\n", p->payload);

         printf("HttpClient::rx_callback() - context: %p\n", context);
         printf("HttpClient::rx_callback() - context->http_client: %p\n", context->http_client);
         printf("HttpClient::rx_callback() - body_callback: %p\n", ((HttpClient *)context->http_client)->body_callback);

         ((HttpClient *)context->http_client)->body_callback(context, p);

         // bufferPool().deallocate(buffer);
      }
      pbuf_free(p);
   }

   return ERR_OK;
}

err_t HttpClient::headers_done_callback(httpc_state_t *connection, 
                                        void *arg,
                                        struct pbuf *hdr, 
                                        u16_t hdr_len, 
                                        u32_t content_len)
{
   http_request_context_t *context = static_cast<http_request_context_t *>(arg);
   if(!context)
   {
      return ERR_OK;
   }

   if(hdr != NULL && context->headerCallback)
   {
      char *headers = (char *)bufferPool().allocate();
      u16_t copy_len = std::min((size_t)(hdr_len + 1), bufferPool().get_buffer_size());

      pbuf_copy_partial(hdr, headers, copy_len, 0);
      headers[copy_len] = '\0';

      // context->headerCallback(headers, copy_len, content_len);

      bufferPool().deallocate(headers);
   }

   return ERR_OK;
}

void HttpClient::result_callback(void *arg, 
                                 httpc_result_t httpc_result,
                                 u32_t rx_content_len, 
                                 u32_t srv_res, 
                                 err_t err)
{
   http_request_context_t *context = static_cast<http_request_context_t *>(arg);
   if(!context)
   {
      return;
   }

   if(context->resultCallback)
   {
      // context->resultCallback(httpc_result, rx_content_len, srv_res);
   }
}