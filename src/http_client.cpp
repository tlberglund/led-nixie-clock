

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "http_client.h"
#include "lwip/altcp_tls.h"


static struct altcp_pcb *altcp_tls_alloc_sni(void *arg, u8_t ip_type) {
    assert(arg);
    http_request_context_t *req = (http_request_context_t *)arg;
    struct altcp_pcb *pcb = altcp_tls_alloc(req->tls_config, ip_type);
    if (!pcb) {
        printf("Failed to allocate PCB\n");
        return NULL;
    }
    mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(pcb), req->url_parts.authority);
    return pcb;
}


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

#if LWIP_ALTCP
    if (requestContext.tls_config) {
        if (!requestContext.tls_allocator.alloc) {
            requestContext.tls_allocator.alloc = altcp_tls_alloc_sni;
            requestContext.tls_allocator.arg = &requestContext;
        }
        requestContext.settings.altcp_allocator = &requestContext.tls_allocator;
    }
#else
    const uint16_t default_port = 80;
#endif
    requestContext.complete = false;
    requestContext.settings.headers_done_fn = requestContext.headerCallback;
    requestContext.settings.result_fn = requestContext.resultCallback;
    async_context_acquire_lock_blocking(cyw43_arch_async_context());
    err_t ret = httpc_get_file_dns(requestContext.url_parts.authority, 
                                   requestContext.url_parts.port_number, 
                                   requestContext.url_parts.path, 
                                   &requestContext.settings,
                                   requestContext.receiveCallback, 
                                   &requestContext, 
                                   NULL);
    async_context_release_lock(cyw43_arch_async_context());
    if(ret != ERR_OK) {
        printf("http request failed: %d", ret);
    }
    return ret;

   printf("err=%d\n", ret);   
   return ret;
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