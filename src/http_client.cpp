

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "http_client.h"


HttpClient::HttpClient() : http_connection(nullptr)
{
   memset(&connection_settings, 0, sizeof(connection_settings));
   connection_settings.result_fn = result_callback;
   connection_settings.headers_done_fn = headers_done_callback;
   connection_settings.use_proxy = 0;
}


err_t HttpClient::get(const char *url,
                      ReceiveCallback on_receive,
                      HeadersCallback on_headers,
                      ResultCallback on_result)
{
   printf("HttpClient::get() - URL: %s\n", url);
   if(!parse_url((char *)url, &url_parts))
   {
      return ERR_ARG;
   }

   print_url(&url_parts); 

   if(url_parts.port_number < 0)
   {
      url_parts.port_number = 80;
      strcpy(url_parts.port, "80");
   }

   callback_data.client = this;
   callback_data.receive_cb = on_receive;
   callback_data.headers_cb = on_headers;
   callback_data.result_cb = on_result;

   printf("About to httpc_get_file_dns()\n");

   err_t err = httpc_get_file_dns(url_parts.authority,
                                  url_parts.port_number,
                                  url_parts.path,
                                  &connection_settings,
                                  rx_callback,
                                  &callback_data,
                                  &http_connection);

   printf("err=%d\n", err);   
   return err;
}


err_t HttpClient::rx_callback(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
   CallbackData *data = static_cast<CallbackData *>(arg);
   if(!data || !data->client)
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
      if(data->receive_cb)
      {
         char *buffer = (char *)bufferPool().allocate();

         u16_t copy_len = std::min((size_t)p->tot_len, bufferPool().get_buffer_size() - 1);
         pbuf_copy_partial(p, buffer, copy_len, 0);
         buffer[copy_len] = '\0';

         printf("%s\n", buffer);

         data->receive_cb(buffer, p->tot_len);

         bufferPool().deallocate(buffer);
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
   CallbackData *data = static_cast<CallbackData *>(arg);
   if(!data || !data->client)
   {
      return ERR_OK;
   }

   if(hdr != NULL && data->headers_cb)
   {
      char *headers = (char *)bufferPool().allocate();
      u16_t copy_len = std::min((size_t)(hdr_len + 1), bufferPool().get_buffer_size());

      pbuf_copy_partial(hdr, headers, copy_len, 0);
      headers[copy_len] = '\0';

      data->headers_cb(headers, copy_len, content_len);

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
   CallbackData *data = static_cast<CallbackData *>(arg);
   if(!data || !data->client)
   {
      return;
   }

   if(data->result_cb)
   {
      data->result_cb(httpc_result, rx_content_len, srv_res);
   }
}