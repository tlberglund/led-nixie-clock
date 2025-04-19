#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "lwip/apps/http_client.h"


// Buffer for storing the response
#define MAX_RESPONSE_SIZE 4096
char response_buffer[MAX_RESPONSE_SIZE];
size_t response_len = 0;

// Semaphore for signaling HTTP completion
SemaphoreHandle_t http_done_semaphore;
bool request_complete = false;
bool request_success = false;

#if 0

// Perform HTTP GET request using lwIP's HTTP client
bool perform_http_get_request(const char *server, const char *path) {
    // Initialize result variables
    request_complete = false;
    request_success = false;
    response_len = 0;
    
    // Create a binary semaphore for signaling completion
    http_done_semaphore = xSemaphoreCreateBinary();
    if (http_done_semaphore == NULL) {
        printf("Failed to create semaphore\n");
        return false;
    }
    
    // Set up HTTP client headers
    httpc_connection_t settings;
    memset(&settings, 0, sizeof(settings));
    settings.use_proxy = 0;
    settings.headers_done_fn = NULL;
    settings.result_fn = http_client_result_callback;
    settings.recv_fn = http_client_recv_callback;
    
    httpc_state_t *connection = NULL;
    
    err_t err = httpc_get_file_dns(
        server,              // Server hostname
        80,                  // Port number (80 for HTTP, 443 for HTTPS)
        path,                // URL path
        &settings,           // HTTP client settings
        http_client_recv_callback, // Data received callback
        NULL,                // Argument to pass to callbacks
        &connection          // HTTP client connection state
    );
    
    if (err != ERR_OK) {
        printf("HTTP client get file failed: %d\n", err);
        vSemaphoreDelete(http_done_semaphore);
        return false;
    }
    
    // Wait for HTTP request to complete with timeout
    if (xSemaphoreTake(http_done_semaphore, pdMS_TO_TICKS(15000)) != pdTRUE) {
        printf("HTTP request timed out\n");
        httpc_cancel(connection);
        vSemaphoreDelete(http_done_semaphore);
        return false;
    }
    
    vSemaphoreDelete(http_done_semaphore);
    
    // Process response if successful
    if (request_success) {
        process_response_body();
    }
    
    return request_success;
}

#endif