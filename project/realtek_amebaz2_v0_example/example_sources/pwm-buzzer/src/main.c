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

#include "pwmout_api.h"

#define PWM_5    PA_17

pwmout_t pwm_led[1];
PinName  pwm_led_pin[1] = {PWM_5};
float period[8] = {1.0/523, 1.0/587, 1.0/659, 1.0/698, 1.0/784, 1.0/880, 1.0/988, 1.0/1047};

void pwm_delay (void)
{
    int i;
    for(i = 0; i < 1000000; i++) {
        asm(" nop");
    }
}

//int main_app(IN u16 argc, IN u8 *argv[])
int main (void)
{
    int i;

    dbg_printf("\r\n   PWM-buzzer DEMO   \r\n");

    pwmout_init(&pwm_led[0], pwm_led_pin[0]);

    while (1) {
        for (i = 0; i < 8; i++) {
            pwmout_period(&pwm_led[0], period[i]);
            pwmout_pulsewidth(&pwm_led[0], (period[i]/2));
            hal_delay_ms(1000);
        }
//        wait_ms(20);
//        RtlMsleepOS(25);
        pwm_delay();
    }
}
