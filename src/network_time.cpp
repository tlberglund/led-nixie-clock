
#include <stdlib.h>
#include <string.h>
#include "network_time.h"
#include "pico/util/datetime.h"
#include "pico/aon_timer.h"
#include "lwip/apps/sntp.h"




/**
 * This function is declared in $PICO_SDK/lib/lwip/src/apps/sntp/sntp.c, but not
 * implemented anywhere in that library code. It is called from the lwIP stack
 * when the SNTP client gets a result back from an NTP server, and it's our
 * responsibility to do something with that time. The NetworkTime class, (which
 * is involved because it holds time zone offset state, which NTP itself doesn't
 * care about) provides the set_time_in_seconds() method to implement the RTC
 * interface.
 */
void sntpSetTimeSec(uint32_t sec) {
    NetworkTime::getInstance().set_time_in_seconds(sec);
}


/**
 * Kicks off the SNTP process in the Pico SDK LWIP "apps" library. This function
 * can be called before or after the FreeRTOS scheduler is running, but it can't
 * be called before the WifiConnection has been set with set_wifi_connection().
 *
 * The function creates a short-lived task that waits for the wifi connection to
 * be established through the syncrhronization mechanism exposed by the
 * WifiConnection class. That task kicks off the periodic SNTP sync process,
 * whose period is determined by SNTP_UPDATE_DELAY defined in lwipopts.h. That
 * task dies as soon as the SNTP process is started, since an internal LWIP task
 * is managing the update timer for us.
 */
void NetworkTime::init()
{
    xTaskCreate(time_task, "SNTP Task", 1024, this, 1, &time_task_handle);
}


/***
 * Set timezone offset
 *
 * @param offsetHours - hours of offset -23 to + 23
 * @param offsetMinutes - for timezones that use odd mintes you can add or sub
 * additional minutes
 */
void NetworkTime::sntp_set_timezone(int offsetHours, int offsetMinutes) {
    sntp_timezone_minutes_offset = (offsetHours * 60) + offsetMinutes;
}


/**
 * Add SNTP server - can call to add multiple servers
 * @param server - string name of server. Should remain in scope
 */
void NetworkTime::sntp_add_server(const char *server){
    sntp_setservername(sntp_server_count++, server);
}


/**
 * Call the LWIP Simple Network Time Protocol (SNTP) library to start the clock
 * running, literally. See the NetworkTime::init() method for more details on 
 * what's going on behind the scenes.
 */
void NetworkTime::sntp_start_sync() {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_init();
}


/**
 * Actually sets the Pico real-time clock (RTC) with a specific time. In the
 * case of this class, that time should be coming from an NTP server. See the
 * sntpSetTimeSec() function (declared in this file but not a part of the
 * NetworkTime class) for how this call is made.
 */
void NetworkTime::set_time_in_seconds(uint32_t sec) {
    struct timespec ts;
    uint64_t ms = (sec + (60 * sntp_timezone_minutes_offset)) * 1000;

    printf("SETTING TIME TO %lu\n", ms);

    ms_to_timespec(ms, &ts);

    if(aon_is_running) {
        aon_timer_set_time(&ts);
    }
    else {
        aon_timer_start(&ts);
        aon_is_running = true;
    }
}


void NetworkTime::time_task(void *params) {
    printf("NTP TASK STARTED\n");
    NetworkTime *time = (NetworkTime *)params;
    WifiConnection *wifi = time->wifi;

    printf("NTP TASK WAITING FOR WIFI INIT\n");
    wifi->wait_for_wifi_init();
    printf("NTP TASK WIFI INIT COMPLETE\n");

    time->sntp_start_sync();

    printf("NTP SYNC RUNNING; EXITING NTP TASK\n");

    vTaskDelete(NULL);
}
