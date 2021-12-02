/**************************************************************************//**
 * @file     gpio_irq_api.c
 * @brief    This file implements the mbed HAL API for GPIO pin with interrupt function.
 *
 * @version  V1.00
 * @date     2017-07-24
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
#include "pinmap.h"

#include "gpio_irq_api.h"
#include "gpio_irq_ex_api.h"

/* map mbed pin mode definition to RTK HAL pull control type */
#define MAX_PIN_MODE            4

// follow globral variables are declared in gpio_api.c
extern const uint8_t mbed_pinmode_map[];
extern hal_gpio_comm_adapter_t mbd_gpio_com_adp;
extern uint32_t is_mbd_gpio_com_inited;

/**
  * @brief  Initializes the GPIO device interrupt mode, include mode/trigger/polarity registers.
  * @param  obj: gpio irq object define in application software.
  * @param  pin: PinName according to pinmux spec.
  * @note this API only works for Port A pins
  * @param  handler: Interrupt handler to be assigned to the specified pin.
  * @param  id: handler id.
  * @retval none
  */
int gpio_irq_init(gpio_irq_t *obj, PinName pin, gpio_irq_handler handler, uint32_t id)
{
    hal_status_t ret;

    if (!is_mbd_gpio_com_inited) {
        hal_gpio_comm_init (&mbd_gpio_com_adp);
        is_mbd_gpio_com_inited = 1;
    }

    ret = hal_gpio_irq_init (&obj->gpio_irq_adp, pin, (gpio_irq_callback_t)handler, id);
    if (ret == HAL_OK) {
    	return 0;
    } else {
        return -1;
    }
}

/**
  * @brief  Deinitializes the GPIO device interrupt mode, include mode/trigger/polarity registers.
  * @param  obj: gpio irq object define in application software.
  * @note this API only works for Port A pins
  * @retval none
  */
void gpio_irq_free(gpio_irq_t *obj)
{
    hal_gpio_irq_deinit(&obj->gpio_irq_adp);
}

/**
  * @brief  Enable/Disable gpio interrupt.
  * @param  obj: gpio irq object define in application software.
  * @param  event: gpio interrupt event, this parameter can be one of the following values:
  *		@arg IRQ_RISE: rising edge interrupt event
  *		@arg IRQ_FALL: falling edge interrupt event
  *		@arg IRQ_FALL_RISE: rising and falling edge interrupt event
  *		@arg IRQ_LOW: low level interrupt event
  *		@arg IRQ_HIGH: high level interrupt event
  *		@arg IRQ_NONE: no interrupt event
  * @param  enable: this parameter can be one of the following values:
  *		@arg 0 disable gpio interrupt
  *		@arg 1 enable gpio interrupt
  * @retval none
  */
void gpio_irq_set(gpio_irq_t *obj, gpio_irq_event event, uint32_t enable)
{
    gpio_int_trig_type_t int_type;

    int_type = (gpio_int_trig_type_t)event;
    if (int_type > IRQ_FALL_RISE) {
        DBG_GPIO_ERR ("Invalid GPIO IRQ Event(%u)\n", event);
        return;
    }

    hal_gpio_irq_set_trig_type (&obj->gpio_irq_adp, int_type);

    if (enable) {
        hal_gpio_irq_enable (&obj->gpio_irq_adp);
    } else {
        hal_gpio_irq_disable (&obj->gpio_irq_adp);
    }

}

/**
  * @brief  Enable gpio interrupt.
  * @param  obj: gpio irq object define in application software.
  * @retval none
  */
void gpio_irq_enable(gpio_irq_t *obj)
{
    hal_gpio_irq_enable (&obj->gpio_irq_adp);
}

/**
  * @brief  Disable gpio interrupt.
  * @param  obj: gpio irq object define in application software.
  * @retval none
  */
void gpio_irq_disable(gpio_irq_t *obj)
{
    hal_gpio_irq_disable (&obj->gpio_irq_adp);
}

/**
  * @brief  Deinitializes the GPIO device interrupt mode, include mode/trigger/polarity registers.
  * @param  obj: gpio irq object define in application software.
  * @retval none
  */
void gpio_irq_deinit(gpio_irq_t *obj)
{
    hal_gpio_irq_deinit (&obj->gpio_irq_adp);
}

/**
  * @brief  Sets pull type to the selected interrupt pin.
  * @param  obj: gpio irq object define in application software.
  * @param  pull_type: this parameter can be one of the following values:
  *		@arg PullNone: HighZ, user can input high or low use this pin
  *		@arg OpenDrain(is OpenDrain output): no pull + OUT + GPIO[gpio_bit] = 0
  *		@arg PullDown: pull down
  *		@arg PullUp: pull up
  * @retval none
  */
void gpio_irq_pull_ctrl(gpio_irq_t *obj, PinMode pull_type)
{
    pin_pull_type_t io_pull_type;

    if (pull_type < MAX_PIN_MODE) {
        io_pull_type = (pin_pull_type_t)mbed_pinmode_map[pull_type];
    } else {
        // invalid pin mode
        io_pull_type = Pin_PullNone;
    }

    hal_gpio_pull_ctrl (obj->gpio_irq_adp.pin_name, io_pull_type);
}

/**
  * @brief  Enable the specified gpio interrupt event.
  * @param  obj: gpio irq object define in application software.
  * @param  event: gpio interrupt event, this parameter can be one of the following values:
  *		@arg IRQ_RISE: rising edge interrupt event
  *		@arg IRQ_FALL: falling edge interrupt event
  *		@arg IRQ_FALL_RISE: rising and falling edge interrupt event
  *		@arg IRQ_LOW: low level interrupt event
  *		@arg IRQ_HIGH: high level interrupt event
  *		@arg IRQ_NONE: no interrupt event
  * @retval none
  */
void gpio_irq_set_event(gpio_irq_t *obj, gpio_irq_event event)
{
    gpio_int_trig_type_t int_type;

    int_type = (gpio_int_trig_type_t)event;
    if (int_type > IRQ_FALL_RISE) {
        DBG_GPIO_ERR ("Invalid GPIO IRQ Event(%u)\n", event);
        return;
    }

    hal_gpio_irq_set_trig_type (&obj->gpio_irq_adp, int_type);
}

/** 
 *  @brief To enables or disable the debounce function of the given GPIO IRQ pin.
 *         The debounce resource(circuit) is limited, not all GPIO pin 
 *         can has debounce function.
 *
 *  @param[in]  pgpio_irq_adapter  The GPIO IRQ pin adapter.
 *  @param[in]  debounce_us  The time filter for the debounce, in micro-second. 
 *                           But the time resolution is 31.25us (1/32K) and the
 *                           maximum time is 512 ms.
 * @param[in]  enable: this parameter can be one of the following values:
 *     @arg 0 disable gpio debounce interrupt
 *     @arg 1 enable gpio debounce interrupt
 *  @return     0:  Setting Succeed.
 *  @return     -1:  Setting Fail.
 */
int gpio_irq_debounce_set (gpio_irq_t *obj, uint32_t debounce_us, u8 enable)
{
    u8 ret = 0;
    if(enable) {
        ret = hal_gpio_irq_debounce_enable (&obj->gpio_irq_adp, debounce_us);
        if (ret == HAL_OK) {
            return 0;
        } else {
            return -1;
        }
    } else {
        hal_gpio_irq_debounce_disable (&obj->gpio_irq_adp);
        return 0;
    }
}

/**
  * @brief  Reads the specified gpio irq port pin.
  * @param  obj: gpio irq object define in application software.
  * @retval 1: pin state is high
  * @retval 0: pin state is low
  */
int gpio_irq_read (gpio_irq_t *obj)
{
    return hal_gpio_irq_read (&obj->gpio_irq_adp);
}
