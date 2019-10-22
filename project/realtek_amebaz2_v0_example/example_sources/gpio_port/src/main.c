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

#include "port_api.h"

#define PORT_OUTPUT_TEST    1//1: output test, 0: input test
#define LED_PATTERN_NUM     12
#define PIN_MASK            0x9E6000

port_t port0;
const uint32_t led_pattern[LED_PATTERN_NUM] = {0x9E6000, 0x1E6000, 0x8E6000, 0x966000, 0x9A6000, 0x9C6000, 0x9E2000, 0x9E4000, 0x0, 0x802000};    // 0x9E6000 assign to PA_23, PA_20, PA_19, PA_18, PA_17, PA_14, PA_13

extern void wait_ms(u32);

#if PORT_OUTPUT_TEST
int main (void)
{
    int i;
    unsigned int pin_mask;

    dbg_printf("\r\n   GPIO Port Output DEMO   \r\n");

    port_mode(&port0, PullNone);
    // Assign pins to this port 
    pin_mask = PIN_MASK;    // each bit map to 1 pin: 0: pin disable, 1: pin enable
    port_init(&port0, PortA, pin_mask, PIN_OUTPUT);

    while (1) {
        for (i = 0; i < LED_PATTERN_NUM; i++) {
            port_write(&port0, led_pattern[i]);
            wait_ms(200);
        }
    }
}

#else
int main(void)
{
    unsigned int pin_mask;
    int value_new, value_tmp, value_old;
    int stable;

    dbg_printf("\r\n   GPIO Port Input DEMO   \r\n");

    port_mode(&port0, PullNone);
    // Assign pins to this port
    pin_mask = PIN_MASK;    // each bit map to 1 pin: 0: pin disable, 1: pin enable
    port_init(&port0, PortA, pin_mask, PIN_INPUT);

    value_old = port_read(&port0);
    while (1) {
        // De-bonse
        value_new = port_read(&port0);
        stable = 0;
        while (stable < 3) {
            value_tmp = port_read(&port0);
            if (value_new != value_tmp) {
                value_new = value_tmp;
                stable = 0;
            } else {
                stable++;
            }
        }

        if (value_old != value_new) {
            dbg_printf("0x%x \r\n", value_new);
            value_old = value_new;
        }
        wait_ms(50);
    }
}
#endif
