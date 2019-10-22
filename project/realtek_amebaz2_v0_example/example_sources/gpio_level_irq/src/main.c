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
#include "gpio_irq_ex_api.h"

#define GPIO_IRQ_LEVEL_PIN        PA_19
#define GPIO_SIGNAL_SOURCE        PA_20

gpio_irq_t gpio_level;
int current_level = IRQ_LOW;

void gpio_level_irq_handler (uint32_t id, gpio_irq_event event)
{
    uint32_t *level = (uint32_t *) id;

    // Disable level irq because the irq will keep triggered when it keeps in same level.
    gpio_irq_disable(&gpio_level);

    // make some software de-bounce here if the signal source is not stable.

    if (*level == IRQ_LOW) {
        dbg_printf("low level event \r\n");

        // Change to listen to high level event
        *level = IRQ_HIGH;
        gpio_irq_set(&gpio_level, (gpio_irq_event)IRQ_HIGH, 1);
        gpio_irq_enable(&gpio_level);
    } else if (*level == IRQ_HIGH) {
        dbg_printf("high level event \r\n");

        // Change to listen to low level event
        *level = IRQ_LOW;
        gpio_irq_set(&gpio_level, (gpio_irq_event)IRQ_LOW, 1);
        gpio_irq_enable(&gpio_level);
    }
}

int main (void)
{
    int i;

    dbg_printf("\r\n   GPIO Level IRQ DEMO   \r\n");

    // configure level trigger handler
    gpio_irq_init(&gpio_level, GPIO_IRQ_LEVEL_PIN, gpio_level_irq_handler, (uint32_t)(&current_level));
    gpio_irq_set(&gpio_level, (gpio_irq_event)IRQ_LOW, 1);
    gpio_irq_enable(&gpio_level);

    // configure gpio as signal source for high/low level trigger
    gpio_t gpio_src;
    gpio_init(&gpio_src, GPIO_SIGNAL_SOURCE);
    gpio_dir(&gpio_src, PIN_OUTPUT);    // Direction: Output
    gpio_mode(&gpio_src, PullNone);

    while (1) {
        gpio_write(&gpio_src, 1);
        for (i=0; i<20000000; i++) asm("nop");
        gpio_write(&gpio_src, 0);
        for (i=0; i<20000000; i++) asm("nop");
    }
}
