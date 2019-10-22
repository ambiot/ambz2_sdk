/**************************************************************************//**
 * @file     reset_reason_api.c
 * @brief    This file implements the watch dog timer Mbed HAL API functions.
 *
 * @version  V1.00
 * @date     2019-06-21
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
#include "hal_sys_ctrl.h"
#include "reset_reason_api.h"

/** Fetch the reset reason for the last system reset
 *
 * This function must return the contents of the system reset reason registers
 * cast to an appropriate platform independent reset reason. If multiple reset
 * reasons are set this function should return RESET_REASON_MULTIPLE. If the
 * reset reason does not match any existing platform independent value this
 * function should return RESET_REASON_PLATFORM. If no reset reason can be
 * determined this function should return RESET_REASON_UNKNOWN.
 *
 * This function is not idempotent, there is no guarantee that the system
 * reset reason will not be cleared between calls to this function altering the
 * return value between calls.
 *
 * Note: Some platforms contain reset reason registers that persist through
 * system resets. If the registers haven't been cleared before calling this
 * function multiple reasons may be set within the registers. If multiple reset
 * reasons are detected this function will return RESET_REASON_MULTIPLE.
 *
 * @return enum containing the last reset reason for the board.
 */
reset_reason_t hal_reset_reason_get(void)
{
    hal_reset_reason_t hal_reason;
    reset_reason_t reason = RESET_REASON_UNKNOWN;
    rtl8710c_reset_reason_get(&hal_reason);
    switch(hal_reason){
        case HAL_RESET_REASON_POWER_ON:
            reason = RESET_REASON_POWER_ON;
            break;
        case HAL_RESET_REASON_SOFTWARE:
            reason = RESET_REASON_SOFTWARE;
            break;
        case HAL_RESET_REASON_WATCHDOG:
            reason = RESET_REASON_WATCHDOG;
            break;
        default:
            break;
    }
    return reason;
}

/** Fetch the raw platform specific reset reason register value
 *
 * This function must return the raw contents of the system reset reason
 * registers cast to a uint32_t value. If the platform contains reset reasons
 * that span multiple registers/addresses the value should be concatenated into
 * the return type.
 *
 * This function is not idempotent, there is no guarantee that the system
 * reset reason will not be cleared between calls to this function altering the
 * return value between calls.
 *
 * @return value containing the reset reason register for the given platform.
 *         If the platform contains reset reasons across multiple registers they
 *         will be concatenated here.
 */
uint32_t hal_reset_reason_get_raw(void)
{
    hal_reset_reason_t hal_reason;
    reset_reason_t reason;
    rtl8710c_reset_reason_get(&hal_reason);
    reason = (reset_reason_t)hal_reason;
    return reason;
}

/** Clear the reset reason from registers
 *
 * Reset the value of the reset status registers, the reset reason will persist
 * between system resets on certain platforms so the registers should be cleared
 * before the system resets. Failing to do so may make it difficult to determine
 * the cause of any subsequent system resets.
 */
void hal_reset_reason_clear(void)
{
    rtl8710c_reset_reason_clear(0xFFFFFFFF);
}

/** Set the reset reason to registers
 *
 * Set the value of the reset status registers, to let user applicatoin store
 * the reason before doing reset.
 */
void hal_reset_reason_set(reset_reason_t reason)
{
    hal_reset_reason_t hal_reason = HAL_RESET_REASON_SOFTWARE;
    switch(reason){
        case RESET_REASON_SOFTWARE:
            hal_reason = HAL_RESET_REASON_SOFTWARE;
            break;
        case RESET_REASON_WATCHDOG:
            hal_reason = HAL_RESET_REASON_WATCHDOG;
            break;
        default:
            break;
    }
    rtl8710c_reset_reason_set(hal_reason);
}

