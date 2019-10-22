/**************************************************************************//**
 * @file     wdt_api.c
 * @brief    This file implements the watch dog timer Mbed HAL API functions.
 *
 * @version  V1.00
 * @date     2017-11-10
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
#include "wdt_api.h"
#include "hal_wdt.h"


/**
 *  @brief   Initial the watch dog time setting
 *
 *  @param   timeout_ms: the watch-dog timer timeout value, in ms.
 *           default action of timeout is to reset the whole system.
 *  @return  None
 *
 */
void watchdog_init(uint32_t timeout_ms)
{
	hal_misc_wdt_init ((timeout_ms * 1000));
}

/**
 *  @brief   Start the watchdog counting
 *
 *  @param   None
 *  @return  None
 *
 */
void watchdog_start(void)
{
	hal_misc_wdt_enable ();
}

/**
 *  @brief   Stop the watchdog counting
 *
 *  @param   None
 *  @return  None
 *
 */
void watchdog_stop(void)
{
	hal_misc_wdt_disable ();
}

/**
 *  @brief   Refresh the watchdog counting to prevent WDT timeout
 *
 *  @param   None
 *  @return  None
 *
 */
void watchdog_refresh(void)
{
	hal_misc_wdt_refresh ();
}

/**
 *  @brief   Switch the watchdog timer to interrupt mode and
 *           register a watchdog timer timeout interrupt handler.
 *           The interrupt handler will be called when the watch-dog
 *           timer is timeout.
 *
 *  @param   handler: the callback function for WDT timeout interrupt.
 *           id: the parameter for the callback function
 *  @return  None
 *
 */
void watchdog_irq_init(wdt_irq_handler handler, uint32_t id)
{
	hal_misc_wdt_reg_irq ((irq_handler_t)handler, (void *)id);
}


