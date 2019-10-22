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
#include "gpio_irq_api.h"

#define GPIO_LED_PIN        PA_20
#define GPIO_IRQ_PIN        PA_19

int led_ctrl;
gpio_t gpio_led;

void gpio_demo_irq_handler (uint32_t id, gpio_irq_event event)
{
    gpio_t *gpio_led;

    dbg_printf("%s==> \r\n", __FUNCTION__);
    gpio_led = (gpio_t *)id;

    led_ctrl = !led_ctrl;
    gpio_write(gpio_led, led_ctrl);
}

int main (void)
{
    gpio_irq_t gpio_btn;

    dbg_printf("\r\n   GPIO IRQ DEMO   \r\n");

    // Init LED control pin
    gpio_init(&gpio_led, GPIO_LED_PIN);
    gpio_dir(&gpio_led, PIN_OUTPUT);    // Direction: Output
    gpio_mode(&gpio_led, PullNone);     // No pull

    // Initial Push Button pin as interrupt source
    gpio_irq_init(&gpio_btn, GPIO_IRQ_PIN, gpio_demo_irq_handler, (uint32_t)(&gpio_led));
    gpio_irq_set(&gpio_btn, IRQ_FALL, 1);    // Falling Edge Trigger
    gpio_irq_enable(&gpio_btn);

    led_ctrl = 1;
    gpio_write(&gpio_led, led_ctrl);

    while(1);
}
