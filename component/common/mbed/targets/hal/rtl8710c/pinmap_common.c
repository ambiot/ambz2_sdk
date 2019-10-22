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
 * Copyright(c) 2007 - 2016 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
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

