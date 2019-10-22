/*
 * Copyright(c) 2007 - 2019 Realtek Corporation. All rights reserved.
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
 */

#include "wdt_api.h"

#define RUN_CALLBACK_IF_WATCHDOG_BARKS (0)

void dummy_task (void)
{
    //int i = 0;
    //for (i = 0; i < 50000000; i++) {
    //    asm(" nop");
    //}
    hal_delay_ms(1000);
}

void small_task (void)
{
    dbg_printf("\r\ndoing small task... \r\n");
    dummy_task();
    dbg_printf("refresh watchdog \r\n\r\n");
    watchdog_refresh();
}

void big_task (void)
{
    int i = 0;
    dbg_printf("\r\ndoing big task... \r\n");
    for (i = 0; i < 10; i++) {
        dbg_printf("doing dummy task %d \r\n", i);
        dummy_task();
    }
    dbg_printf("refresh watchdog \r\n\r\n");
    watchdog_refresh();
}

void my_watchdog_irq_handler (uint32_t id)
{
    dbg_printf("watchdog barks!!! \r\n");
    watchdog_stop();
}

int main (void)
{
    dbg_printf("\r\n   Watchdog DEMO   \r\n");

    watchdog_init(5000); // setup 5s watchdog

#if RUN_CALLBACK_IF_WATCHDOG_BARKS
    watchdog_irq_init(my_watchdog_irq_handler, 0);
#else
    // system would restart when watchdog barks
#endif

    watchdog_start();

    small_task();
    big_task();

    while(1);
}
