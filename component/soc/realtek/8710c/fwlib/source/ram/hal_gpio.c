/**************************************************************************//**
 * @file     hal_gpio.c
 * @brief    This GPIO HAL API functions.
 *
 * @version  V1.00
 * @date     2017-04-26
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

/**
 * @addtogroup hs_hal_gpio GPIO
 * @{
 */


#include "hal_gpio.h"
#include "hal_pinmux.h"

#if CONFIG_GPIO_EN

#if !defined(CONFIG_BUILD_SECURE)

/**
  * @brief The table of all GPIO port data mode input enable registers address.
  */
const uint32_t *pport_in_en_reg[GPIO_MAX_PORT_NUM] = {
    (uint32_t *)&(GPIO->port_a_idm_en)
};

/**
  * @brief The table of all GPIO port data mode output enable registers address.
  */
const uint32_t *pport_out_en_reg[GPIO_MAX_PORT_NUM] = {
    (uint32_t *)&(GPIO->port_a_odm_en)
};

/**
 *  @brief Initials a GPIO pin.
 *           - Defult configure the GPIO pin as a normal input pin (not an interrupt pin).
 *
 *  @param[in]  pgpio_adapter  The GPIO pin adapter.
 *  @param[in]  pin_name  The GPIO pin.
 *                - bit[7:5]: the GPIO port number. Each port has 32 GPIO pins.
 *                - bit[4:0]: the pin number of the GPIO port.
 *
 *  @return     HAL_ERR_PARA:  Input arguments are invalid.
 *  @return     HAL_OK:  GPIO pin initialization OK.
 */
hal_status_t hal_gpio_init (phal_gpio_adapter_t pgpio_adapter, uint32_t pin_name)
{
    hal_status_t ret;

    ret = hal_pinmux_register((uint8_t)pin_name, PID_GPIO);
    if (ret != HAL_OK) {
        return ret;
    }
    return hal_gpio_stubs.hal_gpio_init (pgpio_adapter, pin_name);
}

/**
 *  @brief To de-initial a GPIO pin.
 *
 *  @param[in]  pgpio_adapter  The GPIO pin adapter.
 *
 *  @returns    void
 */
void hal_gpio_deinit (phal_gpio_adapter_t pgpio_adapter)
{
    hal_pinmux_unregister(pgpio_adapter->pin_name, PID_GPIO);
    hal_gpio_stubs.hal_gpio_deinit (pgpio_adapter);
}

/**
 *  @brief Initials a GPIO IRQ pin.
 *
 *  @param[in]  pgpio_irq_adapter  The GPIO IRQ pin adapter.
 *  @param[in]  pin_name  The GPIO IRQ pin.
 *                - bit[7:5]: the GPIO port number. Each port has 32 GPIO pins.
 *                - bit[4:0]: the pin number of the GPIO port.
 *  @param[in]  callback  The GPIO IRQ call back function.
 *  @param[in]  arg  The argument of the GPIO IRQ call back function. It is an application
 *                   priviate data to be passed by the call back function.
 *
 *  @return     HAL_NOT_READY:  The GPIO group adapter does not been initialed yet.
 *  @return     HAL_ERR_PARA:  Input arguments are invalid.
 *  @return     HAL_NO_RESOURCE: No GPIO IRQ resource (circuit). The GPIO IRQ resource is limited,
 *                               not all GPIO pin can has GPIO IRQ function.
 *  @return     HAL_OK: GPIO IRQ pin initialization OK.
 */
hal_status_t hal_gpio_irq_init (phal_gpio_irq_adapter_t pgpio_irq_adapter, uint32_t pin_name,
                                gpio_irq_callback_t callback, uint32_t arg)
{
    hal_status_t ret;

    ret = hal_pinmux_register((uint8_t)pin_name, PID_GPIO);
    if (ret != HAL_OK) {
        return ret;
    }

    return hal_gpio_stubs.hal_gpio_irq_init (pgpio_irq_adapter, pin_name, callback, arg);
}

/**
 *  @brief To de-initial and disable a GPIO IRQ pin.
 *
 *  @param[in]  pgpio_irq_adapter  The GPIO IRQ pin adapter.
 *
 *  @returns    void
 */
void hal_gpio_irq_deinit (phal_gpio_irq_adapter_t pgpio_irq_adapter)
{
    hal_pinmux_unregister(pgpio_irq_adapter->pin_name, PID_GPIO);
    hal_gpio_stubs.hal_gpio_irq_deinit (pgpio_irq_adapter);
}

/**
 *  @brief Initials a GPIO port. A GPIO port has 32 GPIO pins. However, may not all
 *         GPIO pin will be bound out. Checks the IC package to know what pins are
 *         available on this GPIO port.
 *
 *  @param[in]  pgpio_port_adapter  The GPIO port adapter.
 *  @param[in]  port_idx The GPIO port index.
 *                - 0: Port A
 *                - 1: Port B
 *  @param[in]  mask  The GPIO pin mask to indicate what pin are included in this port.
 *                    Bit 0 maps to pin 0; bit 1 maps to pin 1; ... etc. The bit value 1
 *                    means the maped pin is included in this GPIO port.
 *  @param[in]  dir  The GPIO port direction, IN or OUT.
 *
 *  @return     HAL_ERR_PARA:  Input arguments are invalid.
 *  @return     HAL_OK:  GPIO port initialization OK.
 */
hal_status_t hal_gpio_port_init (phal_gpio_port_adapter_t pgpio_port_adapter, uint32_t port_idx,
                                 uint32_t mask, gpio_dir_t dir)
{
    hal_status_t ret = HAL_OK;
    gpio_pin_t pin;
    uint32_t i;

    pin.pin_name_b.port = port_idx;
    for (i=0; i<MAX_PIN_IN_PORT; i++) {
        if (mask & (1 << i)) {
            pin.pin_name_b.pin = i;
            ret |= hal_pinmux_register(pin.pin_name, PID_GPIO);
            if (ret != HAL_OK) {
                break;
            }
        }
    }
    
    if (ret != HAL_OK) {
        return ret;
    }

    return hal_gpio_stubs.hal_gpio_port_init (pgpio_port_adapter, port_idx, mask, dir);
}

/**
 *  @brief To de-initial a GPIO port. All pins in this GPIO port will be switched
 *         as input pin.
 *
 *  @param[in]  pgpio_port_adapter The GPIO port adapter.
 *
 *  @returns    void
 */
void hal_gpio_port_deinit (phal_gpio_port_adapter_t pgpio_port_adapter)
{
    gpio_pin_t pin;
    uint32_t i;
    uint32_t mask;

    pin.pin_name_b.port = pgpio_port_adapter->port_idx;
    mask = pgpio_port_adapter->pin_mask;
    for (i=0; i<MAX_PIN_IN_PORT; i++) {
        if (mask & (1 << i)) {
            pin.pin_name_b.pin = i;
            hal_pinmux_unregister(pin.pin_name, PID_GPIO);
        }
    }
    hal_gpio_stubs.hal_gpio_port_deinit (pgpio_port_adapter);
}

/**
 *  @brief To set the direction of the given GPIO port.
 *
 *  @param[in]  pgpio_port_adapter The GPIO port adapter.
 *  @param[in]  dir  The GPIO port direction, IN or OUT.
 *                     -0: input.
 *                     -1: output.
 *
 *  @returns    void
 */
void hal_gpio_port_dir (phal_gpio_port_adapter_t pgpio_port_adapter, gpio_dir_t dir)
{
    uint32_t *port_dir_en;
    uint32_t port_idx;

    port_idx = pgpio_port_adapter->port_idx;
    // set direction
    if (dir == GPIO_IN) {
        port_dir_en = (uint32_t *)pport_in_en_reg[port_idx];

    } else {
        port_dir_en = (uint32_t *)pport_out_en_reg[port_idx];
    }

    *port_dir_en = pgpio_port_adapter->pin_mask << pgpio_port_adapter->pin_offset;
}

/**
 *  @brief To set the IO Power Voltage of the GPIO A13 and A14.
 *
 *  @param[in]  h5l3  The GPIO A13 and A14 Voltage Selection.
 *                     -0: 3.3V.
 *                     -1: 5V.
 *
 *  @returns    void
 */
void hal_gpio_h5l3 (u8 h5l3)
{
    u32 buff = 0;
    buff = HAL_READ32(0x40000000, 0xAC);
    // set voltage
    if (h5l3 == 1) {
        buff = buff | 0x30000;
    } else {
        buff = buff & 0xFFFCFFFF;
    }
    HAL_WRITE32(0x40000000, 0xAC, buff);
}


#endif  // end of else of "#if defined(CONFIG_BUILD_SECURE) && (CONFIG_BUILD_SECURE == 1)"

#endif  // end of "#if CONFIG_GPIO_EN"

/** @} */ /* End of group hs_hal_gpio */

