/**************************************************************************//**
 * @file     hal_irq.c
 * @brief    This file implements the IRQ control functions.
 *
 * @version  V1.00
 * @date     2019-08-14
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
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

#include "cmsis.h"

#if CONFIG_CMSIS_FREERTOS_EN
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
#endif

void sys_irq_enable(int32_t irqn);
void sys_irq_disable(int32_t irqn);
void sys_irq_set_vector(int32_t irqn, uint32_t vector);
uint32_t sys_irq_get_vector(int32_t irqn);
void sys_irq_set_priority(int32_t irqn, uint32_t priority);
uint32_t sys_irq_get_priority(int32_t irqn);
void sys_irq_set_pending(int32_t irqn);
uint32_t sys_irq_get_pending(int32_t irqn);
void sys_irq_clear_pending(int32_t irqn);
void sys_interrupt_enable(void);
void sys_interrupt_disable(void);

static volatile uint32_t primask;

#if !defined(CONFIG_BUILD_SECURE)
// Building for Non-Secure and Ignore Secure

/* implement system level IRQ control functions here */
const hal_irq_api_t sys_irq_api = {
    .irq_enable = sys_irq_enable, 
    .irq_disable = sys_irq_disable, 
    .irq_set_vector = sys_irq_set_vector, 
    .irq_get_vector = sys_irq_get_vector, 
    .irq_set_priority = sys_irq_set_priority, 
    .irq_get_priority = sys_irq_get_priority, 
    .irq_set_pending = sys_irq_set_pending, 
    .irq_get_pending = sys_irq_get_pending, 
    .irq_clear_pending = sys_irq_clear_pending, 
    .interrupt_enable = sys_interrupt_enable,        
    .interrupt_disable = sys_interrupt_disable
};

/**
 * @addtogroup hs_hal_irq
 * @{
 */

/**
  \brief   Enable Interrupt
  \details Enables a device specific interrupt in the NVIC interrupt controller.
  \param [in]      irqn  Device specific interrupt number.
  \note    irqn must not be negative.
 */
void sys_irq_enable(int32_t irqn)
{
    __NVIC_EnableIRQ (irqn);
}

/**
  \brief   Disable Interrupt
  \details Disables a device specific interrupt in the NVIC interrupt controller.
  \param [in]      irqn  Device specific interrupt number.
  \note    irqn must not be negative.
 */
void sys_irq_disable(int32_t irqn)
{
    __NVIC_DisableIRQ(irqn);
}

/**
  \brief   Set Interrupt Vector
  \details Sets an interrupt vector in SRAM based interrupt vector table.
           The interrupt number can be positive to specify a device specific interrupt,
           or negative to specify a processor exception.
           VTOR must been relocated to SRAM before.
  \param [in]   irqn      Interrupt number
  \param [in]   vector    Address of interrupt handler function
 */
void sys_irq_set_vector(int32_t irqn, uint32_t vector)
{
    __NVIC_SetVector(irqn, vector);
    __DSB();
}

/**
  \brief   Get Interrupt Vector
  \details Reads an interrupt vector from interrupt vector table.
           The interrupt number can be positive to specify a device specific interrupt,
           or negative to specify a processor exception.
  \param [in]   irqn      Interrupt number.
  \return                 Address of interrupt handler function
 */
uint32_t sys_irq_get_vector(int32_t irqn)
{
    return __NVIC_GetVector(irqn);
}

/**
  \brief   Set Interrupt Priority
  \details Sets the priority of a device specific interrupt or a processor exception.
           The interrupt number can be positive to specify a device specific interrupt,
           or negative to specify a processor exception.
  \param [in]      irqn  Interrupt number.
  \param [in]  priority  Priority to set.
  \note    The priority cannot be set for every processor exception.
 */
void sys_irq_set_priority(int32_t irqn, uint32_t priority)
{
    __NVIC_SetPriority(irqn, priority);
}

/**
  \brief   Get Interrupt Priority
  \details Reads the priority of a device specific interrupt or a processor exception.
           The interrupt number can be positive to specify a device specific interrupt,
           or negative to specify a processor exception.
  \param [in]   irqn  Interrupt number.
  \return             Interrupt Priority.
                      Value is aligned automatically to the implemented priority bits of the microcontroller.
 */
uint32_t sys_irq_get_priority(int32_t irqn)
{
    return __NVIC_GetPriority(irqn);
}

/**
  \brief   Set Pending Interrupt
  \details Sets the pending bit of a device specific interrupt in the NVIC pending register.
  \param [in]      irqn  Device specific interrupt number.
  \note    irqn must not be negative.
 */
void sys_irq_set_pending(int32_t irqn)
{
    __NVIC_SetPendingIRQ(irqn);
}

/**
  \brief   Get Pending Interrupt
  \details Reads the NVIC pending register and returns the pending bit for the specified device specific interrupt.
  \param [in]      irqn  Device specific interrupt number.
  \return             0  Interrupt status is not pending.
  \return             1  Interrupt status is pending.
  \note    irqn must not be negative.
 */
uint32_t sys_irq_get_pending(int32_t irqn)
{
    return __NVIC_GetPendingIRQ(irqn);
}

/**
  \brief   Clear Pending Interrupt
  \details Clears the pending bit of a device specific interrupt in the NVIC pending register.
  \param [in]      irqn  Device specific interrupt number.
  \note    irqn must not be negative.
 */
void sys_irq_clear_pending(int32_t irqn)
{
    __NVIC_ClearPendingIRQ(irqn);
}

/**
  \brief   Enable IRQ Interrupts
  \details Enables IRQ interrupts by clearing the I-bit in the CPSR.
           Can only be executed in Privileged modes.
 */
void sys_interrupt_enable(void)
{
    __set_PRIMASK(primask);
}

/**
  \brief   Disable IRQ Interrupts
  \details Disables IRQ interrupts by setting the I-bit in the CPSR.
           Can only be executed in Privileged modes.
 */
void sys_interrupt_disable(void)
{
    primask = __get_PRIMASK();
    __disable_irq();
}

#endif

/** @} */ /* End of group hs_hal_irq */

