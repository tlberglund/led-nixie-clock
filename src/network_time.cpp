
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
void NetworkTime::set_timezone(int offsetHours, int offsetMinutes) {
    printf("TIMEZONE OFFSET: %d hrs, %d mins\n", offsetHours, offsetMinutes);
    timezone_offset_minutes = (offsetHours * 60) + offsetMinutes;
    printf("TIMEZONE OFFSET: %d mins\n", timezone_offset_minutes);
}


/**
 * Add SNTP server - can call to add multiple servers
 * @param server - string name of server. Should remain in scope
 */
void NetworkTime::add_sntp_server(const char *server){
    sntp_setservername(sntp_server_count++, server);
}


/**
 * Call the LWIP Simple Network Time Protocol (SNTP) library to start the clock
 * running, literally. See the NetworkTime::init() method for more details on 
 * what's going on behind the scenes.
 */
void NetworkTime::start_sntp_sync() {
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
    struct tm tim;

    printf("RAW TIME:        %lu\n", sec);
    printf("TIMEZONE OFFSET: %d\n", timezone_offset_minutes);

    //
    // Handling of 64-bit integers seems suspect. ms_to_timespec() takes a
    // uint64_t argument and doesn't seem to work, and %llu doesn't give
    // reliable results∑ in printf() either. So, fine...I'll have to do it
    // myself.
    //
    ts.tv_sec = sec + (timezone_offset_minutes * 60);
    ts.tv_nsec = 0;

    printf("TIMESPEC: %lu %lu\n", ts.tv_sec, ts.tv_nsec);

    if(aon_is_running) {
        aon_timer_set_time(&ts);
    }
    else {
        printf("STARTING AON TIMER\n");
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

    printf("TIMEZONE OFFSET: %d\n", time->timezone_offset_minutes);

    time->start_sntp_sync();

    printf("NTP SYNC RUNNING; EXITING NTP TASK\n");

    vTaskDelete(NULL);
}
