/**************************************************************************//**
 * @file     rtc_api.c
 * @brief    This file implements the GPIO Mbed HAL API functions.
 *
 * @version  V1.00
 * @date     2019-05-31
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/
#include "rtc_api.h"

#if DEVICE_RTC
#include <time.h>
#include "timer_api.h"     // software-RTC: use a g-timer for the tick of the RTC

#define SW_RTC_TIMER_ID    TIMER4

static gtimer_t sw_rtc;
static struct tm rtc_timeinfo;
static int sw_rtc_en = 0, sw_rtc_count_en = 0;;
static uint32_t count_down = 0;

static alarm_irq_handler rtc_alarm_handler = NULL;
void rtc_alarm_intr_handler(void);
void rtc_disable_alarm(void);

const static u8 dim[12] = { 31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static inline bool is_leap_year(unsigned int year)
{
    return (!(year % 4) && (year % 100)) || !(year % 400);
}

static u8 days_in_month (u8 month, u8 year)
{
    u8 ret = dim [month];
    if (ret == 0) {
        ret = is_leap_year (year) ? 29 : 28;
    }
    return ret;
}

void sw_rtc_tick_handler(uint32_t id)
{
    if (++rtc_timeinfo.tm_sec > 59) {                               // Increment seconds, check for overflow
        rtc_timeinfo.tm_sec = 0;                                    // Reset seconds
        if (++rtc_timeinfo.tm_min > 59) {                           // Increment minutes, check for overflow
            rtc_timeinfo.tm_min = 0;                                // Reset minutes
            if (++rtc_timeinfo.tm_hour > 23) {                      // Increment hours, check for overflow
                rtc_timeinfo.tm_hour = 0;                           // Reset hours
                ++rtc_timeinfo.tm_yday;                             // Increment day of year
                if(++rtc_timeinfo.tm_wday > 6)                     // Increment day of week, check for overflow
                    rtc_timeinfo.tm_wday = 0;                       // Reset day of week
                                                        // Increment day of month, check for overflow
                if (++rtc_timeinfo.tm_mday >
                    days_in_month(rtc_timeinfo.tm_mon, rtc_timeinfo.tm_year + 1900)) {
                    rtc_timeinfo.tm_mday = 1;                       // Reset day of month
                    if(++rtc_timeinfo.tm_mon > 11) {                // Increment month, check for overflow
                        rtc_timeinfo.tm_mon = 0;                    // Reset month
                        rtc_timeinfo.tm_yday = 0;                   // Reset day of year
                        ++rtc_timeinfo.tm_year;                     // Increment year
                    }                                               // - year
                }                                                   // - month
            }                                                       // - day
        }                                                           // - hour
    }
    if (sw_rtc_count_en) {
        count_down--;
        if (!count_down) {
            if(rtc_alarm_handler != NULL){
            rtc_alarm_handler();
            }
            rtc_disable_alarm();;
                sw_rtc_count_en = 0;
            }
        }
}

void rtc_init(void)
{
    // Initial a periodical timer
    gtimer_init(&sw_rtc, SW_RTC_TIMER_ID);
    // Tick every 1 sec
    gtimer_start_periodical(&sw_rtc, 1000000, (void*)sw_rtc_tick_handler, (uint32_t)&sw_rtc);
    sw_rtc_en = 1;
}

void rtc_free(void)
{
    sw_rtc_en = 0;
    gtimer_stop(&sw_rtc);
    gtimer_deinit(&sw_rtc);
}

int rtc_isenabled(void)
{
    return(sw_rtc_en);
}

time_t rtc_read(void)
{
    time_t t;

    // Convert to timestamp
    t = mktime(&rtc_timeinfo);

    return t;
}

void rtc_write(time_t t)
{
    // Convert the time in to a tm
    struct tm *timeinfo = localtime(&t);

    if (timeinfo == NULL) {
        // Error
        return;
    }

    gtimer_stop(&sw_rtc);

    // Set the RTC
    rtc_timeinfo.tm_sec = timeinfo->tm_sec;
    rtc_timeinfo.tm_min = timeinfo->tm_min;
    rtc_timeinfo.tm_hour = timeinfo->tm_hour;
    rtc_timeinfo.tm_mday = timeinfo->tm_mday;
    rtc_timeinfo.tm_wday = timeinfo->tm_wday;
    rtc_timeinfo.tm_yday = timeinfo->tm_yday;
    rtc_timeinfo.tm_mon = timeinfo->tm_mon;
    rtc_timeinfo.tm_year = timeinfo->tm_year;

    gtimer_start(&sw_rtc);
}

/**
  * @brief  Set the specified RTC Alarm and interrupt.
  * @param  alarm: alarm object define in application software.
  * @param  alarmHandler:  alarm interrupt callback function.
  * @retval   status:
  *            - 1: success
  *            - Others: failure
  */
u32 rtc_set_alarm(alarm_t *alrm, alarm_irq_handler alarmHandler)
{
    uint32_t alarm_time_s = 0;
    time_t current_t = rtc_read();
    struct tm *alarm_timeinfo = localtime(&current_t);

    alarm_time_s = (alrm->yday - alarm_timeinfo->tm_yday) * (24 * 60 * 60);
    alarm_time_s = alarm_time_s + (alrm->hour) * (60 * 60) + (alrm->min) * (60) + (alrm->sec);
    alarm_time_s = alarm_time_s - ((alarm_timeinfo->tm_hour) * (60 * 60) + (alarm_timeinfo->tm_min) * (60) + (alarm_timeinfo->tm_sec));

    count_down = alarm_time_s;
    sw_rtc_count_en = 1;
    rtc_alarm_handler = alarmHandler;

    return _TRUE;
}

/**
  * @brief  Disable RTC Alarm and function.
  * @param  none
  * @retval   none
  */
void rtc_disable_alarm(void)
{
    rtc_alarm_handler = NULL;
}

#endif  // endof "#if DEVICE_RTC"


