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
#include "serial_api.h"
#include "timer_api.h"
#include "pwmout_api.h"

//wake up by Stimer    : 0
//wake up by GPIO      : 1
//wake up by UART      : 2
//wake up by Gtimer    : 3
//wake up by PWM       : 4
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
static int GPIO_init_count = 0;
#endif

#if (WAKEUP_SOURCE == 2)
//UART0
#define UART_TX    PA_14
#define UART_RX    PA_13
static serial_t my_UART;
static int UART_init_count = 0;
volatile char rc = 0;

static void uart_send_string (serial_t *my_UART, char *pstr)
{
    unsigned int i = 0;
    while (*(pstr+i) != 0) {
        serial_putc(my_UART, *(pstr+i));
        i++;
    }
}

static void uart_irq (uint32_t id, SerialIrq event)
{
    serial_t *my_UART = (void*)id;
    if (event == RxIrq) {
        rc = serial_getc(my_UART);
        serial_putc(my_UART, rc);
    }
    if((event == TxIrq) && (rc != 0)){
        uart_send_string(my_UART, "\r\n8710c$ \r\n");
        rc = 0;
    }
}
#endif

#if (WAKEUP_SOURCE == 3)
#define GTIMER_SLEEP_DURATION (5 * 1000 * 1000)
//TIMER0, TIMER1, TIMER2, TIMER3, TIMER4, TIMER5, TIMER6
#define WAKEUP_GTIMER TIMER3
static gtimer_t my_Gtimer;
static int Gtimer_init_count = 0;

static void Gtimer_timeout_handler (uint32_t id)
{
    dbg_printf("Gtimer wake up   \r\n");
}
#endif

#if (WAKEUP_SOURCE == 4)
#define PWM_SLEEP_DURATION 5
//PA_0, PA_1, PA_2, PA_3, PA_4, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, PA_23
//PA_11, PA_12; if PA_7 to PA_12 is enabled
#define WAKEUP_PWM_PIN PA_13
static pwmout_t my_PWM;
static int PWM_init_count = 0;
extern void pwmout_period_int(pwmout_t *obj, pwm_period_callback_t callback, u8 enable);

static void PWM_period_handler (uint32_t id)
{
    dbg_printf("PWM wake up   \r\n");
}
#endif


int main (void)
{
    while (1) {
        dbg_printf("\r\n   PM_SleepCG DEMO   \r\n");
        //dbg_printf("Wait 10s to enter SleepCG\r\n");
        //hal_delay_us(10 * 1000 * 1000);

#if (WAKEUP_SOURCE == 0)
        dbg_printf("Enter SleepCG, wake up by Stimer \r\n");
        for (int i = 5; i > 0; i--) {
            dbg_printf("Enter SleepCG by %d seconds \r\n", i);
            hal_delay_us(1 * 1000 * 1000);
        }
        SleepCG(SLP_STIMER, SLEEP_DURATION, CLOCK, 0);

#elif (WAKEUP_SOURCE == 1)
        dbg_printf("Enter SleepCG, wake up by GPIO");
//if there is no GPIO wakeup source please set a GPIO pin for wake up
#if (GPIO_WAKEUP_SOURCE  == 0)
        if (GPIO_init_count == 0) {
            gpio_irq_init(&my_GPIO_IRQ, WAKUPE_GPIO_PIN, NULL, (uint32_t)&my_GPIO_IRQ);
            GPIO_init_count = 1;
        }
        gpio_irq_pull_ctrl(&my_GPIO_IRQ, PullNone);
        gpio_irq_set(&my_GPIO_IRQ, IRQ_FALL, 1);
        dbg_printf("_A%d \r\n", WAKUPE_GPIO_PIN);
#else
        dbg_printf("   \r\n");
#endif
        for (int i = 5; i > 0; i--) {
            dbg_printf("Enter SleepCG by %d seconds \r\n", i);
            hal_delay_us(1 * 1000 * 1000);
        }
        SleepCG(SLP_GPIO, SLEEP_DURATION, CLOCK, 17);

#elif (WAKEUP_SOURCE == 2)
        dbg_printf("Enter SleepCG, wake up by UART \r\n");
        if (UART_init_count == 0) {
            serial_init(&my_UART, UART_TX, UART_RX);
            UART_init_count = 1;
        }
        serial_baud(&my_UART, 38400);
        serial_format(&my_UART, 8, ParityNone, 1);
        serial_irq_handler(&my_UART, uart_irq, (uint32_t)&my_UART);
        serial_irq_set(&my_UART, RxIrq, 1);
        serial_irq_set(&my_UART, TxIrq, 1);
        for (int i = 5; i > 0; i--) {
            dbg_printf("Enter SleepCG by UART %d seconds \r\n", i);
            hal_delay_us(1 * 1000 * 1000);
        }
        SleepCG(SLP_UART, SLEEP_DURATION, CLOCK, 0);

#elif (WAKEUP_SOURCE == 3)
        dbg_printf("Enter SleepCG, wake up by Gtimer \r\n");
        if (Gtimer_init_count == 0) {
            gtimer_init(&my_Gtimer, WAKEUP_GTIMER);
            Gtimer_init_count = 1;
        }
        for (int i = 5; i > 0; i--) {
            dbg_printf("Enter SleepCG by %d seconds \r\n", i);
            hal_delay_us(1 * 1000 * 1000);
        }
        gtimer_start_one_shout(&my_Gtimer, GTIMER_SLEEP_DURATION, (void*)Gtimer_timeout_handler, NULL);
        SleepCG(SLP_GTIMER, SLEEP_DURATION, CLOCK, 0);

#elif (WAKEUP_SOURCE == 4)
        dbg_printf("Enter SleepCG, wake up by PWM \r\n");

        if (PWM_init_count == 0) {
            pwmout_init(&my_PWM, WAKEUP_PWM_PIN);
            PWM_init_count = 1;
        }
        pwmout_period_int(&my_PWM, 0, 0);
        pwmout_period(&my_PWM, PWM_SLEEP_DURATION);
        for (int i = 5; i > 0; i--) {
            dbg_printf("Enter SleepCG by %d seconds \r\n", i);
            hal_delay_us(1 * 1000 * 1000);
        }
        pwmout_period_int(&my_PWM, (pwm_period_callback_t)PWM_period_handler, 1);
        SleepCG(SLP_PWM, SLEEP_DURATION, CLOCK, 0);
#endif
        __enable_irq();
        dbg_printf("\r\n   SleepCG resume   \r\n");
        hal_delay_ms(2 * 1000);
    }
}
