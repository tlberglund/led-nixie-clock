
#ifndef __NETWORK_TIME_H__
#define __NETWORK_TIME_H__

#include <stdlib.h>
#include "pico/stdlib.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "wifi.h"


class NetworkTime {
    public:
        void init();
        void sntp_set_timezone(int offsetHours, int offsetMinutes = 0);
        void sntp_add_server(const char *server);
        void sntp_start_sync();
        void set_time_in_seconds(uint32_t sec);
        static void time_task(void *params);

        static NetworkTime& getInstance() {
            static NetworkTime instance;
            
            instance.sntp_server_count = 0;
            instance.sntp_timezone_minutes_offset = 0;
            instance.time_task_handle = (TaskHandle_t)0;
            instance.sntp_add_server("0.us.pool.ntp.org");
            instance.sntp_add_server("1.us.pool.ntp.org");
            instance.sntp_add_server("2.us.pool.ntp.org");
            instance.sntp_add_server("3.us.pool.ntp.org");
            instance.wifi = NULL;
            instance.aon_is_running = false;

            return instance;
        }

        void set_wifi_connection(WifiConnection *wifi) { this->wifi = wifi; };

    private:
        NetworkTime() {};
        TaskHandle_t time_task_handle;
        int sntp_server_count;
        int32_t sntp_timezone_minutes_offset;
        WifiConnection *wifi;
        bool aon_is_running;
};

#endif
