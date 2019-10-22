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

#include "gpio_api.h"
#include "timer_api.h"

#define GPIO_LED_PIN1       PA_19
#define GPIO_LED_PIN2       PA_20

#define PERIODICAL_TIMER    TIMER2
#define ONE_SHOUT_TIMER     TIMER3

gtimer_t my_timer1;
gtimer_t my_timer2;
gpio_t gpio_led1;
gpio_t gpio_led2;
volatile uint32_t time2_expired = 0;

static void timer1_timeout_handler (uint32_t id)
{
    gpio_t *gpio_led = (gpio_t *)id;
    gpio_write(gpio_led, !gpio_read(gpio_led));
}

static void timer2_timeout_handler (uint32_t id)
{
    time2_expired = 1;
}

int main (void)
{
    dbg_printf("\r\n   Gtimer DEMO   \r\n");

    // Init LED control pin
    gpio_init(&gpio_led1, GPIO_LED_PIN1);
    gpio_dir(&gpio_led1, PIN_OUTPUT);// Direction: Output
    gpio_mode(&gpio_led1, PullNone);// No pull

    gpio_init(&gpio_led2, GPIO_LED_PIN2);
    gpio_dir(&gpio_led2, PIN_OUTPUT);// Direction: Output
    gpio_mode(&gpio_led2, PullNone);// No pull

    // Initial a periodical timer
    gtimer_init(&my_timer1, PERIODICAL_TIMER);
    gtimer_start_periodical(&my_timer1, 1000000, (void*)timer1_timeout_handler, (uint32_t)&gpio_led1);

    // Initial a one-shout timer and re-trigger it in while loop
    gtimer_init(&my_timer2, ONE_SHOUT_TIMER);
    time2_expired = 0;
    gtimer_start_one_shout(&my_timer2, 500000, (void*)timer2_timeout_handler, NULL);

    while (1) {
        if (time2_expired) {
            gpio_write(&gpio_led2, !gpio_read(&gpio_led2));
            time2_expired = 0;
            gtimer_start_one_shout(&my_timer2, 500000, (void*)timer2_timeout_handler, NULL);
        }
    }
}
