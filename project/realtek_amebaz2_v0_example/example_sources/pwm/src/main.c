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

#define PWM_5           PA_17
#define PWM_6           PA_18
#define PWM_7           PA_13
#define PWM_2           PA_14
#define PWM_PERIOD      20000
#define USE_FLOAT       0
#define HIGH_FREQUENCY  0

#if USE_FLOAT
#if !HIGH_FREQUENCY
#define PWM_STEP        (1.0/20.0)
float pwms[4] = {0.0, 0.25, 0.5, 0.75};
float steps[4] = {PWM_STEP, PWM_STEP, PWM_STEP, PWM_STEP};
#else
#define PWM_STEP        (1.0/10000.0)
float pwms[4] = {0.0, 0.25, 0.5, 0.75};
float steps[4] = {PWM_STEP, PWM_STEP, PWM_STEP, PWM_STEP};
#undef PWM_PERIOD
#define PWM_PERIOD      500
#endif
#else
#define PWM_STEP        (PWM_PERIOD/20)
int pwms[4] = {0, PWM_PERIOD/4, PWM_PERIOD/2, PWM_PERIOD/4*3};
int steps[4] = {PWM_STEP,PWM_STEP,PWM_STEP,PWM_STEP};
#endif

pwmout_t pwm_led[4];
PinName pwm_led_pin[4] = {PWM_5, PWM_6, PWM_7, PWM_2};

extern void RtlMsleepOS(u32 ms);

void pwm_delay (void)
{
    int i;
    for (i = 0; i < 1000000; i++) {
        asm(" nop");
    }
}

int main (void)
{
    int i;

    dbg_printf("\r\n   PWM DEMO   \r\n");

    for (i = 0; i < 4; i++) {
        pwmout_init(&pwm_led[i], pwm_led_pin[i]);
        pwmout_period_us(&pwm_led[i], PWM_PERIOD);
    }

    while (1) {
#if USE_FLOAT
        for (i = 0; i < 4; i++) {
            pwmout_write(&pwm_led[i], pwms[i]);

            pwms[i] += steps[i];
            if (pwms[i] >= 1.0) {
                steps[i] = -PWM_STEP;
                pwms[i] = 1.0;
            }

            if (pwms[i] <= 0.0) {
                steps[i] = PWM_STEP;
                pwms[i] = 0.0;
            }
        }
#else
        for (i = 0; i < 4; i++) {
            pwmout_pulsewidth_us(&pwm_led[i], pwms[i]);

            pwms[i] += steps[i];
            if (pwms[i] >= PWM_PERIOD) {
                steps[i] = -PWM_STEP;
                pwms[i] = PWM_PERIOD;
            }

            if (pwms[i] <= 0) {
                steps[i] = PWM_STEP;
                pwms[i] = 0;
            }
        }
#endif
//        wait_ms(20);
//        RtlMsleepOS(25);
#if !HIGH_FREQUENCY
        pwm_delay();
#endif
    }
}
