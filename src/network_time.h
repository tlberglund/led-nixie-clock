
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
        void set_timezone(int offsetHours, int offsetMinutes = 0);
        void set_timezone(const char *tz);
        void add_sntp_server(const char *server);
        void start_sntp_sync();
        void set_time_in_seconds(uint32_t sec);
        static void time_task(void *params);

        static NetworkTime& getInstance() {
            static NetworkTime instance;
            
            return instance;
        }

        void set_wifi_connection(WifiConnection *wifi) { this->wifi = wifi; };

    private:
        NetworkTime() {
            sntp_server_count = 0;
            timezone_offset_minutes = 0;
            time_task_handle = (TaskHandle_t)0;
            add_sntp_server("0.us.pool.ntp.org");
            add_sntp_server("1.us.pool.ntp.org");
            add_sntp_server("2.us.pool.ntp.org");
            add_sntp_server("3.us.pool.ntp.org");
            wifi = NULL;
            aon_is_running = false;
        };
        TaskHandle_t time_task_handle;
        int sntp_server_count;
        int16_t timezone_offset_minutes;
        const char *timezone_name;
        WifiConnection *wifi;
        bool aon_is_running;
};

#endif
