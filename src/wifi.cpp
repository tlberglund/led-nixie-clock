/*
 * Wifi.cpp
 *
 *  Created on: 24 Nov 2021 by jondurrant
 *  Enhanced a lot on or about: 2 Jan 2025 by Tim Berglund
 */

 #include <time.h>
 #include "wifi.h"
 #include "pico/cyw43_arch.h"
 #include "FreeRTOS.h"
 #include "task.h"
 
 
 /***
  * Initialize the CYW43 network controller and connect to the wireless network specified by
  * the SSID and password set in the class. If the connection fails, continue retrying periodically.
  * All of this work happens in the WifiConnection::connect_task() FreeRTOS task, which this function 
  * creates.
  */
 void WifiConnection::init() {
     if(init_event_group == NULL) {
         init_event_group = xEventGroupCreate();
     }
 
     int r = xTaskCreate(connect_task, "Wifi Task", 1024, this, 2, &wifi_task_handle);
 }
 
 
 void WifiConnection::connect_task(void *params) {
     TaskHandle_t task;
     uint8_t ip[4];
     char ip_str[20];
 
     WifiConnection *wifi_utils = (WifiConnection *)params;
 
     wifi_utils->set_state(INIT);
     printf("WIFI CONNECT TASK STARTING\n");
 
     int res = cyw43_arch_init();
     if(res) {
         printf("CYW43 INIT FAILED, EXITING connect_task\n");
         sleep_ms(50);
         vTaskDelete(NULL);
     }
 
     printf("CYW43 ARCH INIT COMPLETE\n");
 
     cyw43_wifi_pm(&cyw43_state, CYW43_PERFORMANCE_PM);
 
     printf("CYW43 WIFI PM INIT COMPLETE\n");
 
     wifi_utils->set_state(CY43_INIT);
     wifi_utils->unblock_cyw43_init();
 
     // CYW43 link status appears to be inaccurate before the first attempt to join the 
     // network, so join before we loop
     printf("JOINING '%s'\n", wifi_utils->get_ssid());
     wifi_utils->set_state(CONNECTING);
     if(wifi_utils->join()) {
         wifi_utils->set_state(CONNECTED);
         wifi_utils->get_ip_address(ip);
         wifi_utils->ip_to_string(ip, ip_str);
         printf("IP ADDRESS: %s\n", ip_str);
     }
     else {
         wifi_utils->set_state(FAILED);
     }
 
     for(;;) {
         // If there's no network, the CYW43 (or the API) lies about there being
         // a connection at first. Wait a little bit so it has time to adjust to
         // the reality of being internet-less.
         vTaskDelay(1000);
         if(wifi_utils->is_joined()) {
             wifi_utils->set_state(CONNECTED);
             wifi_utils->unblock_wifi_init();
             vTaskDelay(5000);
         }
         else {
             if(wifi_utils->get_state() == CONNECTED) {
                 wifi_utils->set_state(DISCONNECTED);
             }
             wifi_utils->block_wifi_init();
             // printf("NOT CONNECTED TO WIFI\n", wifi_utils->get_ssid());
             printf("JOINING '%s'\n", wifi_utils->get_ssid());
             if(wifi_utils->join()) {
                 printf("JOINED '%s'\n", wifi_utils->get_ssid());
 
                 // WifiConnection::getMACAddressStr(buffer);
                 // printf("MAC ADDRESS: %s\n", buffer);
 
                 wifi_utils->get_ip_address(ip);
                 wifi_utils->ip_to_string(ip, ip_str);
                 printf("IP ADDRESS: %s\n", ip_str);
             }
             else {
                 // If we've disconnected, stay in that state until we reconnect.
                 // Otherwise, if we were trying to connect for the first time,
                 // fail.
                 if(wifi_utils->get_state() != DISCONNECTED) {
                     wifi_utils->set_state(FAILED);
                 }
             }
         }
     }
 }
 
 
 const char *WifiConnection::state_to_string() {
     switch(this->state) {
         case INIT:
             return "INIT";
         case CY43_INIT:
             return "CY43_INIT";
         case CONNECTING:
             return "CONNECTING";
         case FAILED:
             return "FAILED";
         case CONNECTED:
             return "CONNECTED";
         case DISCONNECTED:
             return "DISCONNECTED";
         default:
             return "UNKONWN STATE";
     }
 }
 
 void WifiConnection::set_state(ConnectionState state) {
     // printf("WIFI STATE=%s\n", state_to_string());
     this->state = state;
 }
 
 bool WifiConnection::join() {
     cyw43_arch_enable_sta_mode();
 
     printf("CONNECTING TO NETWORK '%s'\n", get_ssid());
 
     int r = -1;
     int attempts = 0;
     while(r < 0) {
         attempts++;
         r = cyw43_arch_wifi_connect_timeout_ms(get_ssid(), 
                                                get_password(),
                                                get_wifi_auth(),
                                                get_wifi_connect_timeout());
 
         if(r) {
             printf("FAILED TO JOIN NETWORK %d\n", attempts);
             if(attempts >= get_wifi_connect_retries()) {
                 printf("RETRIES EXHAUSTED\n");
                 return false;
             }
             vTaskDelay(1000);
         }
     }
 
     return true;
 }
 
 
 int32_t WifiConnection::get_rssi() {
     return cyw43_ioctl(&cyw43_state, 254, sizeof(rssi), (uint8_t *)&rssi, CYW43_ITF_STA);
 }
 
 
 /***
  * Get IP address of unit
  * @param ip - output uint8_t[4]
  */
 bool WifiConnection::get_ip_address(uint8_t *ip) {
     if(is_joined()) {
         memcpy(ip, netif_ip4_addr(&cyw43_state.netif[0]), 4);
         return true;
     }
     else {
         return false;
     }
 }
 
 
 char *WifiConnection::ip_to_string(uint8_t *ip, char *ips) {
     strcpy(ips, ipaddr_ntoa(netif_ip4_addr(&cyw43_state.netif[0])));
     return ips;
 }
 
 
 /***
  * Get Gateway address
  * @param ip - output uint8_t[4]
  */
 bool WifiConnection::get_gateway_address(uint8_t *ip) {
     if(is_joined()) {
         memcpy(ip, netif_ip4_gw(&cyw43_state.netif[0]), 4);
         return true;
     }
     else {
         return false;
     }
 }
 
 
 
 /***
  * Get Net Mask address
  * @param ip - output uint8_t[4]
  */
 bool WifiConnection::get_net_mask(uint8_t *ip) {
     if(is_joined()) {
         memcpy(ip, netif_ip4_netmask(&cyw43_state.netif[0]), 4);
         return true;
     }
     else {
         return false;
     }
 }
 
 
 bool WifiConnection::get_mac_address(uint8_t *mac) {
     if(is_joined()) {
         memcpy(mac, cyw43_state.mac, 6);
         // cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA,  mac);
         return true;
     }
     else {
         return false;
     }
 }
 
 
 /***
  * Get the mac address as a string
  * @param macStr: pointer to string of at least 14 characters
  * @return true if successful
  */
 bool WifiConnection::get_mac_address_str(char *macStr) {
     uint8_t mac[6];
     if(get_mac_address(mac)) {
         for(uint8_t i = 0; i < 6; i++) {
             if(mac[i] < 16) {
                 sprintf(&macStr[i*2], "0%X", mac[i]);
             } else {
                 sprintf(&macStr[i*2], "%X", mac[i]);
             }
         }
         macStr[13] = 0;
         return true;
     }
     return false;
 }
 
 
 /***
  * Returns if joined to the network and we have a link
  * @return true if joined.
  */
 bool WifiConnection::is_joined() {
     int status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
     if(status == CYW43_LINK_JOIN) {
         return true;
     }
     else {
         return false;
     }
 }
 
 
 void WifiConnection::unblock_wifi_init() { 
     xEventGroupSetBits(init_event_group, WIFI_INIT_COMPLETE_BIT); 
 }
 
 
 void WifiConnection::unblock_cyw43_init() { 
     xEventGroupSetBits(init_event_group, CYW43_INIT_COMPLETE_BIT); 
 };
 
 
 void WifiConnection::block_wifi_init() { 
     xEventGroupClearBits(init_event_group, WIFI_INIT_COMPLETE_BIT);
 };
 
 
 bool WifiConnection::wait_for_wifi_init() {
     EventBits_t bits = xEventGroupWaitBits(
         init_event_group,
         WIFI_INIT_COMPLETE_BIT,
         pdFALSE,              // Don't clear bits after waiting
         pdTRUE,               // Wait for all bits (just one in this case)
         portMAX_DELAY
     );
 
     return (bits & WIFI_INIT_COMPLETE_BIT);
 }
 
 
 bool WifiConnection::wait_for_cyw43_init() {
     EventBits_t bits = xEventGroupWaitBits(
         init_event_group,
         CYW43_INIT_COMPLETE_BIT,
         pdFALSE,              // Don't clear bits after waiting
         pdTRUE,               // Wait for all bits (just one in this case)
         portMAX_DELAY
     );
 
     return (bits & CYW43_INIT_COMPLETE_BIT);
 }
 
 