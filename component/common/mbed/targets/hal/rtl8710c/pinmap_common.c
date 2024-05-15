/**************************************************************************//**
 * @file     pinmap_common.c
 * @brief    This file implements the Mbed HAL API for IO pin mapping control.
 *
 * @version  V1.00
 * @date     2017-05-03
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

#include "basic_types.h"
#include "diag.h"
#include "pinmap.h"
//#include "error.h"

void pinmap_pinout(PinName pin, const PinMap *map)
{
    // do nothing for our peripheral devices
    // the IO pin direction is auto configured by HW when a peripheral device function is enabled
}

uint32_t pinmap_merge(uint32_t a, uint32_t b)
{
    // both are the same (inc both NC)
    if (a == b)
        return a;

    // one (or both) is not connected
    if (a == (uint32_t)NC)
        return b;
    if (b == (uint32_t)NC)
        return a;

    // mis-match error case
    DBG_MISC_ERR("pinmap_merge: pinmap mis-match\n");
    return (uint32_t)NC;
}

uint32_t pinmap_find_peripheral(PinName pin, const PinMap *map)
{
    while (map->pin != NC) {
        if (map->pin == pin)
            return map->peripheral;
        map++;
    }
    return (uint32_t)NC;
}

uint32_t pinmap_peripheral(PinName pin, const PinMap *map)
{
    uint32_t peripheral = (uint32_t)NC;

    if (pin == (PinName)NC) {
        return (uint32_t)NC;
    }

    peripheral = pinmap_find_peripheral(pin, map);
    if ((uint32_t)NC == peripheral) { // no mapping available
        DBG_MISC_ERR("pinmap_peripheral: pinmap not found for peripheral\n");
    }
    return peripheral;
}

