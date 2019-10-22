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

#include "power_mode_api.h"
#include "gpio_irq_api.h"
#include "gpio_irq_ex_api.h"

//wake up by Stimer : 0
//wake up by GPIO   : 1
#define WAKEUP_SOURCE 0
//Clock, 1: 4MHz, 0: 250kHz
#define CLOCK 0
//SLEEP_DURATION, 5s
#define SLEEP_DURATION (5 * 1000 * 1000)

#if (WAKEUP_SOURCE == 1)
#define GPIO_WAKEUP_SOURCE 0
//PA_0, PA_1, PA_2, PA_3, PA_4, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, PA_23
//PA_7, PA_8, PA_9, PA_10, PA_11, PA_12; if PA_7 to PA_12 is enabled
#define WAKUPE_GPIO_PIN PA_17
static gpio_irq_t my_GPIO_IRQ;
#endif

int main (void)
{
    dbg_printf("\r\n   PM_DeepSleep DEMO   \r\n");

    //dbg_printf("Wait 10s to enter DeepSleep\r\n");
    //hal_delay_us(10 * 1000 * 1000);

#if (WAKEUP_SOURCE == 0)
    dbg_printf("Enter DeepSleep, wake up by Stimer \r\n");
    for (int i = 5; i > 0; i--) {
        dbg_printf("Enter DeepSleep by %d seconds \r\n", i);
        hal_delay_us(1 * 1000 * 1000);
    }
    DeepSleep(DS_STIMER, SLEEP_DURATION, CLOCK);

#elif (WAKEUP_SOURCE == 1)
    dbg_printf("Enter DeepSleep, wake up by GPIO");

//if there is no GPIO wakeup source please set a GPIO pin for wake up
#if (GPIO_WAKEUP_SOURCE  == 0)
    gpio_irq_init(&my_GPIO_IRQ, WAKUPE_GPIO_PIN, NULL, (uint32_t)&my_GPIO_IRQ);
    gpio_irq_pull_ctrl(&my_GPIO_IRQ, PullNone);
    gpio_irq_set(&my_GPIO_IRQ, IRQ_FALL, 1);
    dbg_printf("_A%d \r\n", WAKUPE_GPIO_PIN);
#else
    dbg_printf(" \r\n");
#endif

    for (int i = 5; i > 0; i--) {
        dbg_printf("Enter DeepSleep by %d seconds \r\n", i);
        hal_delay_us(1 * 1000 * 1000);
    }
    DeepSleep(DS_GPIO, SLEEP_DURATION, CLOCK);
#endif

    dbg_printf("You won't see this log \r\n");
    while(1);
}
