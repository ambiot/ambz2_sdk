 /**************************************************************************//**
  * @file     power_mode_api.c
  * @brief    This file implements the mbed HAL API for POWER MODE function.
  *
  * @version  V1.00
  * @date     2018-06-25
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

#include "objects.h"
#include "hal_power_mode.h"
#include "power_mode_api.h"
#include "pinmap.h"
#include "gpio_irq_api.h"
#include "gpio_irq_ex_api.h"

/**
  * @brief The stubs functions table to exports POWER MODE HAL functions in ROM.
  */

//extern const hal_power_mode_func_stubs_t hal_power_mode_stubs;

/** 
 *  @brief The function for ls deep sleep mode.
 *         
 *  @param[in]  Option, To slect AON Timer and GPIO.
 *                - bit[1]: the GPIO as a Wake up event.
 *                - bit[0]: the LP Timer Wake up event.
 *  @param[in]  SDuration, wake up after SDuration value. Uint: us
 *  @param[in]  Clock, 1: 4MHz, 0: 250kHz.
 *
 *  @returns void
 */
void DeepSleep (u8 Option, u32 SDuration, u8 Clock)
{
    hal_gpio_pull_ctrl(PA_1, Pin_PullDown);
    hal_DeepSleep (Option, SDuration, Clock);
}


/** 
 *  @brief The function for sleep mode.
 *         
 *  @param[in]  Option, To slect  GTimer, GPIO and PWM...etc
 *                - bit[4]: the UART Wake up event.
 *                - bit[3]: the PWM Wake up event.
 *                - bit[2]: the GPIO Wake up event.
 *                - bit[1]: the GTimer Wake up event.
 *                - bit[0]: the LP Timer Wake up event.
 *  @param[in]  SDuration, wake up after SDuration value. Uint: us
 *  @param[in]  Clock, 1: 4MHz, 0: 250kHz.
 *  @param[in]  GpioOption, Select GPIO pin as a wake up trigger.
 *
 *  @returns void
 */
void SleepCG (u16 Option, u32 SDuration, u8 Clock, u8 GpioOption)
{
    hal_gpio_pull_ctrl(PA_1, Pin_PullDown);
    hal_SleepCG(Option, SDuration, Clock, GpioOption);
}

/** 
 *  @brief The function for Standby mode.
 *         
 *  @param[in]  Option, To slect GTimer, GPIO and PWM...etc
 *                - bit[4]: the UART Wake up event.
 *                - bit[3]: the PWM Wake up event.
 *                - bit[2]: the GPIO Wake up event.
 *                - bit[1]: the GTimer Wake up event.
 *                - bit[0]: the LP Timer Wake up event.
 *  @param[in]  SDuration, wake up after SDuration value. Uint: us
 *  @param[in]  Clock, 1: 4MHz, 0: 250kHz.
 *  @param[in]  GpioOption, Select GPIO pin as a wake up trigger.
 *
 *  @returns void
 */
void Standby (u16 Option, u32 SDuration, u8 Clock, u8 GpioOption)
{
    hal_gpio_pull_ctrl(PA_1, Pin_PullDown);
    hal_SleepPG (Option, SDuration, Clock, GpioOption);
}
