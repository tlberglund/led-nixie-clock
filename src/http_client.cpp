

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include "http_client.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "altcp_tls_mbedtls_structs.h"


// static struct altcp_pcb *altcp_tls_alloc_sni(void *arg, u8_t ip_type) {
//    assert(arg);
//    http_request_context_t *req = (http_request_context_t *)arg;
//    struct altcp_pcb *pcb = altcp_tls_alloc(req->tls_config, ip_type);
//    if(!pcb) {
//       printf("Failed to allocate PCB\n");
//       return NULL;
//    }
//    mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(pcb), req->url_parts.authority);
//    return pcb;
// }


HttpClient::HttpClient() : http_connection(nullptr)
{
   memset(&requestContext, 0, sizeof(requestContext));
   requestContext.resultCallback = result_callback;
   requestContext.headerCallback = headers_done_callback;
   requestContext.receiveCallback = rx_callback;
   requestContext.settings.use_proxy = 0;
   requestContext.http_client = this;
}


err_t HttpClient::get(const char *url,
                      body_callback_fn body_callback)
{
   printf("HttpClient::get() - URL: %s\n", url);
   if(!parse_url((char *)url, &requestContext.url_parts))
   {
      printf("HttpClient::get() - Failed to parse URL: %s\n", url);
      return ERR_ARG;
   }

   set_body_callback(body_callback);

   if(requestContext.url_parts.port_number < 0)
   {
      if(strcmp(requestContext.url_parts.scheme, "https") == 0)
      {
         printf("HttpClient::get() - No port number specified for scheme '%s', defaulting to 443\n", requestContext.url_parts.scheme);
         requestContext.url_parts.port_number = 443;
         strcpy(requestContext.url_parts.port, "443");
      }
      else
      {
         printf("HttpClient::get() - No port number specified for scheme '%s', defaulting to 80\n", requestContext.url_parts.scheme);
         requestContext.url_parts.port_number = 80;
         strcpy(requestContext.url_parts.port, "80");
      }
   }

   // print_url(&url_parts);

   requestContext.connected = false;
   requestContext.settings.headers_done_fn = requestContext.headerCallback;
   requestContext.settings.result_fn = requestContext.resultCallback;
   printf("CALLING httpc_get_file_dns() with requestContext=%p\n", &requestContext);
   printf("bodyCallback: %p\n", bodyCallback);

   err_t ret;
   if(requestContext.url_parts.port_number == 443) {
      ret = make_https_request();
   }
   else {
      async_context_acquire_lock_blocking(cyw43_arch_async_context());
      ret = httpc_get_file_dns(requestContext.url_parts.authority, 
                               requestContext.url_parts.port_number, 
                               requestContext.url_parts.path, 
                               &requestContext.settings,
                               requestContext.receiveCallback, 
                               &requestContext, 
                               NULL);
      async_context_release_lock(cyw43_arch_async_context());
   }

   if(ret != ERR_OK) {
      printf("http request failed: %d", ret);
   }
   return ret;

   printf("err=%d\n", ret);   
   return ret;
}


void HttpClient::dns_callback(const char *name, const ip_addr_t *resolved, void *ipaddr) {
   if(resolved) {
      *((ip_addr_t *)ipaddr) = *resolved;
      // TODO: notify task
   }
   else {
      ((ip_addr_t *)ipaddr)->addr = IPADDR_NONE;
   }
}


bool HttpClient::resolve_hostname(const char *hostname, ip_addr_t *ipaddr)
{
   ipaddr->addr = IPADDR_ANY;

   printf("HttpClient::resolve_hostname(): resolving %s\n", hostname);
   cyw43_arch_lwip_begin();
   err_t lwip_err = dns_gethostbyname(hostname, ipaddr, dns_callback, ipaddr);
   printf("dns_gethostbyname() is back\n");
   cyw43_arch_lwip_end();

   // Poll for DNS resolution :(
   // TODO: replace with task notification
   if(lwip_err == ERR_INPROGRESS) {
      while(ipaddr->addr == IPADDR_ANY) {
         sleep_ms(10);
         printf("WAITING FOR DNS RESOLUTION\n");
      }

      if(ipaddr->addr != IPADDR_NONE) {
         lwip_err = ERR_OK;
      }
   }

   printf("HttpClient::resolve_hostname(): resolved %s to %08x (%d.%d.%d.%d)\n",
          hostname, ipaddr->addr,
          (ipaddr->addr & 0xff),
          (ipaddr->addr >> 8) & 0xff,
          (ipaddr->addr >> 16) & 0xff,
          (ipaddr->addr >> 24) & 0xff);
   
   return !((bool)lwip_err);
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

void HttpClient::callback_altcp_err(void *arg, err_t err) {
   if(((http_request_context_t *)arg)->tls_config) {
      altcp_free_config(((http_request_context_t *)arg)->tls_config);
   }
}


void HttpClient::altcp_free_config(struct altcp_tls_config *config) {
    cyw43_arch_lwip_begin();
    altcp_tls_free_config(config);
    cyw43_arch_lwip_end();
}


err_t HttpClient::callback_altcp_sent(void *arg, struct altcp_pcb *pcb, u16_t len) {
    ((http_request_context_t *)arg)->acknowledged_len = len;
    return ERR_OK;
}



// TCP + TLS data reception callback
err_t HttpClient::callback_altcp_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *buf, err_t err) {

   struct pbuf *head = buf;

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

            altcp_recved(pcb, head->tot_len);
         }
         // fall through

        case ERR_ABRT:
            pbuf_free(head);
            err = ERR_OK;
            break;
   }

   // Return error
   return err;
}


err_t HttpClient::callback_altcp_connect(void *arg, struct altcp_pcb *pcb, err_t err) {
   ((http_request_context_t *)arg)->connected = true;
   return ERR_OK;
}


bool HttpClient::tls_connect(ip_addr_t *ipaddr, struct altcp_pcb **pcb) {
   cyw43_arch_lwip_begin();
   // struct altcp_tls_config *config = altcp_tls_create_config_client(CA_root_cert, CA_ROOT_CERT_LEN);
   struct altcp_tls_config *config = altcp_tls_create_config_client(NULL, 0);
   cyw43_arch_lwip_end();

   if(config == NULL) {
      return false;
   }

   cyw43_arch_lwip_begin();
   *pcb = altcp_tls_new(config, IPADDR_TYPE_V4);
   cyw43_arch_lwip_end();
   if(*pcb == NULL) {
      altcp_free_config(config);
      return false;
   }

    // Configure hostname for Server Name Indication extension
   cyw43_arch_lwip_begin();
   err_t mbedtls_err = mbedtls_ssl_set_hostname(
      &(((altcp_mbedtls_state_t *)((*pcb)->state))->ssl_context),
      requestContext.url_parts.authority
   );
   cyw43_arch_lwip_end();
   if(mbedtls_err) {
      altcp_free_pcb(*pcb);
      altcp_free_config(config);
      return false;
   }

   requestContext.tls_config = config;
   requestContext.connected = false;
   cyw43_arch_lwip_begin();
   altcp_arg(*pcb, (void *)&requestContext);
   cyw43_arch_lwip_end();

   // Configure connection fatal error callback
   cyw43_arch_lwip_begin();
   altcp_err(*pcb, callback_altcp_err);
   cyw43_arch_lwip_end();

   // Configure idle connection callback (and interval)
   // cyw43_arch_lwip_begin();
   // altcp_poll(
   //    *pcb,
   //    callback_altcp_poll,
   //    PICOHTTPS_ALTCP_IDLE_POLL_INTERVAL
   // );
   // cyw43_arch_lwip_end();

   // Configure data acknowledge callback
   cyw43_arch_lwip_begin();
   altcp_sent(*pcb, callback_altcp_sent);
   cyw43_arch_lwip_end();

   // Configure data reception callback
   cyw43_arch_lwip_begin();
   altcp_recv(*pcb, callback_altcp_recv);
   cyw43_arch_lwip_end();

   // Send connection request (SYN)
   cyw43_arch_lwip_begin();
   err_t lwip_err = altcp_connect(
      *pcb,
      ipaddr,
      LWIP_IANA_PORT_HTTPS,
      callback_altcp_connect
   );
   cyw43_arch_lwip_end();

   // Connection request sent
   if(lwip_err == ERR_OK) {

      // Await connection
      //
      //  Sucessful connection will be confirmed shortly in
      //  callback_altcp_connect.
      //
      while(!(requestContext.connected)) {
         sleep_ms(100);
      }

   }
   else {

      // Free allocated resources
      altcp_free_pcb(*pcb);
      altcp_free_config(config);
   }

    // Return
    return !((bool)lwip_err);

}


bool HttpClient::send_request(struct altcp_pcb *pcb) {
   int request_len;

   request_len = snprintf((char *)requestBuffer, MAX_REQUEST_BUFFER_SIZE, "GET %s HTTP/1.1\r\nHost: %s \r\n\r\n",
                          requestContext.url_parts.path,
                          requestContext.url_parts.authority);

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

    // Write to send buffer
    cyw43_arch_lwip_begin();
    err_t lwip_err = altcp_write(pcb, requestBuffer, request_len, 0);
    cyw43_arch_lwip_end();

    // Written to send buffer
    if(lwip_err == ERR_OK) {
        ((http_request_context_t *)(pcb->arg))->acknowledged_len = 0;
        cyw43_arch_lwip_begin();
        lwip_err = altcp_output(pcb);
        cyw43_arch_lwip_end();

        if(lwip_err == ERR_OK) {
            while(((http_request_context_t *)(pcb->arg))->acknowledged_len == 0) {
               vTaskDelay(pdMS_TO_TICKS(25));
            }

            if(((http_request_context_t *)(pcb->arg))->acknowledged_len != request_len) {
               lwip_err = -1;
            }
        }
    }

    // Return
    return !((bool)lwip_err);

}



err_t HttpClient::make_https_request()
{
   printf("HttpClient::make_https_request()\n");

   // ip_addr_t ipaddr;
   char* char_ipaddr;
   printf("make_https_request(): resolving %s\n", requestContext.url_parts.authority);
    if(!resolve_hostname(requestContext.url_parts.authority, &ip_addr)) {
        printf("Failed to resolve %s\n", requestContext.url_parts.authority);
        return HTTPC_RESULT_ERR_HOSTNAME;
    }

   // ipaddr_ntoa() is ruthlessly thread-unsafe in every implementation I have ever seen
   cyw43_arch_lwip_begin();
   char_ipaddr = ipaddr_ntoa(&ip_addr);
   cyw43_arch_lwip_end();
   printf("make_https_request(): resolved %s to %s\n", requestContext.url_parts.authority, char_ipaddr);



   // Establish TCP + TLS connection with server
#ifdef MBEDTLS_DEBUG_C
   mbedtls_debug_set_threshold(PICOHTTPS_MBEDTLS_DEBUG_LEVEL);
#endif

   struct altcp_pcb *pcb = NULL;
   printf("Connecting to %s://%s:%d\n", requestContext.url_parts.scheme, char_ipaddr, requestContext.url_parts.port_number);
   if(!tls_connect(&ip_addr, &pcb)) {
      printf("Failed to connect to https://%s:%d\n", char_ipaddr, requestContext.url_parts.port_number);
      return HTTPC_RESULT_ERR_CONNECT;
   }
   printf("CONNECTED\n");

   // Send HTTP request to server
   printf("Sending request\n");
   if(!send_request(pcb)){
      printf("Failed to send request\n");
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
         printf("HttpClient::rx_callback() - bodyCallback: %p\n", ((HttpClient *)context->http_client)->bodyCallback);

         ((HttpClient *)context->http_client)->bodyCallback(context, p);

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