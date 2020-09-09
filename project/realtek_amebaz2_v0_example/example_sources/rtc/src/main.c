/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */
#include "rtc_api.h"
#include "wait_api.h"
#include "stdio.h"

extern long long __wrap_mktime (struct tm *);
extern struct tm * __wrap_localtime (long long *);
extern char * __wrap_ctime (long long *);
#undef time_t
#define time_t long long
#undef localtime
#define localtime(a) __wrap_localtime(a)
#undef mktime
#define mktime(a) __wrap_mktime(a)
#undef ctime
#define ctime(a) __wrap_ctime (a)

void alarm_callback(void)
{
  printf("Alarm! \n");
}

 int main() 
 {
    time_t seconds;
    struct tm *timeinfo;
    
    rtc_init();
    rtc_write(1256729737);  // Set RTC time to Wed, 28 Oct 2009 11:35:37

#if 0 //Set Alarm example
    alarm_t alarm;
    alarm.yday = 300;//which day of the year
    alarm.hour = 11;
    alarm.min = 35;
    alarm.sec = 40;
    
    rtc_set_alarm(&alarm, alarm_callback);
#endif

    while(1) {
        seconds = rtc_read();
        timeinfo = localtime(&seconds);

        printf("Time as seconds since January 1, 1970 = %llu\n", seconds);
        printf("Time as a basic string = %s", ctime(&seconds));

        printf("Time as a custom formatted string = %d-%d-%d %d:%d:%d\n", 
            timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour,
            timeinfo->tm_min,timeinfo->tm_sec);

        wait(1.0);
    }
 }