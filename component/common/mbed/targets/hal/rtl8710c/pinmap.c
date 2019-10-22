/**************************************************************************//**
 * @file     pinmap.c
 * @brief    This file implements the Mbed HAL API for IO pin function control.
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

//#include "mbed_assert.h"
#include "objects.h"
#include "pinmap.h"
//#include "error.h"
#include "hal_gpio.h"

/* map mbed pin mode definition to RTK HAL pull control type */
#define MAX_PIN_MODE            4

extern const uint8_t mbed_pinmode_map[];

/**
 * Configure pin enable and function
 */
void pin_function(PinName pin, int function)
{
    // MBED_ASSERT(pin != (PinName)NC);
    //1 Our HAL API cannot support  to configure the pin function by this way
    /* the pin function (pin mux) is depends on each IP On/Off and priority, so we cannot
       set the pin function directly */
}

/**
 * Configure pin pull-up/pull-down
 */
void pin_mode(PinName pin, PinMode mode)
{
    pin_pull_type_t pull_type;

//    MBED_ASSERT(pin != (PinName)NC);

    if (mode >= MAX_PIN_MODE) {
        mode = PullNone;
    }
    pull_type = mbed_pinmode_map[(uint8_t)mode];
    hal_gpio_pull_ctrl (pin, pull_type);
}

