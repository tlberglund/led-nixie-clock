#include <cstdio>
#include "pico/stdlib.h"
#include "secrets.h"
#include "wifi.h"
#include "network_time.h"
#include "apa102.h"
#include "strip_config.h"
#include "pico_led.h"
#include "base64.h"

extern "C" {

    #include "pico/cyw43_arch.h"

    #include "FreeRTOSConfig.h"
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "timers.h"

    #include "lwip/apps/fs.h"
    #include "lwip/apps/httpd.h"
}



config_t config = {
    MAGIC_NUMBER,
    WIFI_SSID,
    WIFI_PASSWORD,
    {0, 0, 0},       // padding
    true,            // use DHCP
    {0, 0, 0},       // padding
    false,           // first LED is at the start of the strip
    {0, 0, 0, 0},    // static IP address
    {0, 0, 0, 0},    // static gateway address
    {0, 0, 0, 0},    // static netmask
    60,              // strip length
    0x00000000       // CRC (not used right now)
};


WifiConnection& wifi = WifiConnection::getInstance();
NetworkTime& network_time = NetworkTime::getInstance();

APA102 led_strip(config.strip_length);

static TaskHandle_t led_task_handle;
#define LED_STATE_BUFSIZE 1000
#define LED_DECODED_BUFSIZE 500
static uint8_t led_state_buffer[LED_STATE_BUFSIZE];
static uint8_t led_decoded_buffer[LED_DECODED_BUFSIZE];
static uint8_t *led_state_current;
static uint16_t led_state_len;

static void dump_buffer(uint8_t *buffer, uint16_t len) {
    for(int n = 0; n < len; n++) {
        printf("%02x ", buffer[n]);
        if((n + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}


static void *current_connection;

err_t httpd_post_begin(void *connection, 
                       const char *uri, 
                       const char *http_request,
                       u16_t http_request_len, 
                       int content_len, 
                       char *response_uri,
                       u16_t response_uri_len, 
                       u8_t *post_auto_wnd) {

    if(memcmp(uri, "/strip", 6) == 0 && current_connection != connection) {
        current_connection = connection;
        led_state_len = 0;
        led_state_current = led_state_buffer;
        snprintf(response_uri, response_uri_len, "/");
        *post_auto_wnd = 1;
        return ERR_OK;
    }
    return ERR_VAL;
}

// Return a value for a parameter
char *httpd_param_value(struct pbuf *p, 
                        const char *param_name, 
                        char *value_buf, 
                        size_t value_buf_len) {
    size_t param_len = strlen(param_name);
    u16_t param_pos = pbuf_memfind(p, param_name, param_len, 0);
    if(param_pos != 0xffff) {
        u16_t param_value_pos = param_pos + param_len;
        u16_t param_value_len = 0;
        u16_t tmp = pbuf_memfind(p, "&", 1, param_value_pos);
        if(tmp != 0xffff) {
            param_value_len = tmp - param_value_pos;
        } else {
            param_value_len = p->tot_len - param_value_pos;
        }
        if(param_value_len > 0 && param_value_len < value_buf_len) {
            char *result = (char *)pbuf_get_contiguous(p, value_buf, value_buf_len, param_value_len, param_value_pos);
            if(result) {
                result[param_value_len] = 0;
                return result;
            }
        }
    }
    return NULL;
}


err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    err_t ret = ERR_VAL;
    LWIP_ASSERT("NULL pbuf", p != NULL);
    printf("POST RX len=%d, tot_len=%d\n", p->len, p->tot_len);
    printf("%s\n", p->payload);

    if(current_connection == connection && 
       (p->len + led_state_len) < LED_STATE_BUFSIZE) {
        memcpy(led_state_current, p->payload, p->len);
        led_state_current += p->len;
        printf("%s\n", led_state_buffer);
        ret = ERR_OK;
    }

    printf("COMPLETE BUFFER:\n");
    printf("%s\n", led_state_buffer);

    pbuf_free(p);
    return ret;
}


void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
    snprintf(response_uri, response_uri_len, "/error.html");
    if(current_connection == connection) {
        snprintf(response_uri, response_uri_len, "/");
        printf("POST FINISHED\n");
        printf("%s\n", led_state_buffer);

        size_t led_decoded_len = LED_DECODED_BUFSIZE;
        base64_decode(led_state_buffer, led_decoded_buffer, &led_decoded_len);

        printf("DECODED %d bytes for %d LEDs\n", led_decoded_len, led_strip.get_strip_len());

        for(int n = 0; n < led_strip.get_strip_len(); n++) {
            uint8_t brightness, red, green, blue;
            uint8_t *led_config;
            led_config = &led_decoded_buffer[n * 4];
            brightness = led_config[0];
            red = led_config[1];
            green = led_config[2];
            blue = led_config[3];
            led_strip.set_led(n, red, green, blue, brightness);
        }
        led_strip.update_strip();

        // Turn Base64 into binary and send it to the APA102 strip
    }
    current_connection = NULL;
}


// static const char *cgi_handler_root(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {

//     printf("cgi_handler_root index=%d, numParams=%d\n", iIndex, iNumParams);
//     for(int n = 0; n < iNumParams; n++) {
//         printf("%s = %s\n", n, pcParam[n], pcValue[n]);
//     }

//     return "PICO OK";
// }


// static tCGI cgi_handlers[] = {
//     { "/", cgi_handler_root }
// };


void led_task(void *pvParameters) {
    WifiConnection *wifi = (WifiConnection *)pvParameters;

    printf("LED TASK STARTED, WAITING FOR WIFI\n");

    wifi->wait_for_wifi_init();

    printf("LED TASK WIFI IS UP\n");

    // http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
    httpd_init();

    for(;;) {
        vTaskDelay(1000);
    }
}


void launch() {

    wifi.set_ssid(WIFI_SSID);
    wifi.set_password(WIFI_PASSWORD);

    printf("STARTING CYW43/WIFI INITIALIZATION\n");
    wifi.init();

    printf("STARTING NTP SYNC\n");
    network_time.sntp_set_timezone(-7);
    network_time.set_wifi_connection(&wifi);
    network_time.init();

    xTaskCreate(led_task, "LED Data Task", 1024, &wifi, 1, &led_task_handle);

    vTaskStartScheduler();
}


int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("UP\n");
    sleep_ms(500);

    printf("LAUNCHING\n");
    launch();
}
