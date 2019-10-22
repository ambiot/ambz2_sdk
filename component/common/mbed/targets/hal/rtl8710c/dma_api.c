/**************************************************************************//**
 * @file     dma_api.c
 * @brief    This file implements the DMA Mbed HAL API functions.
 * 
 * @version  V1.00
 * @date     2017-05-04
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

#include "dma_api.h"
#include "cmsis.h"

/**
 *  @brief   Initial the GDMA
 *
 *  @param   dma_obj: the GDMA object
 *           handler: the callback function for a DMA transfer complete.
 *           id: the argument of the callback function.
 *  @return  None
 *         
 */

void dma_memcpy_init(gdma_t *dma_obj, dma_irq_handler handler, uint32_t id)
{
    phal_gdma_adaptor_t phal_gdma_adaptor = &(dma_obj->hal_gdma_adaptor);

    hal_gdma_memcpy_init(phal_gdma_adaptor);
    hal_gdma_memcpy_irq_hook(phal_gdma_adaptor, (gdma_callback_t) handler, phal_gdma_adaptor);
}

/**
 *  @brief   De-Initial the GDMA
 *
 *  @param   dma_obj: the GDMA object
 *  @return  None
 *         
 */
void dma_memcpy_deinit(gdma_t *dma_obj)
{
    phal_gdma_adaptor_t phal_gdma_adaptor = &(dma_obj->hal_gdma_adaptor);
    hal_gdma_memcpy_deinit(phal_gdma_adaptor);
}

/**
 *  @brief   To do a memory copy by DMA
 *
 *  @param   None
 *  @return  None
 *         
 */
void dma_memcpy(gdma_t *dma_obj, void *dst, void* src, uint32_t len)
{
    phal_gdma_adaptor_t phal_gdma_adaptor = &(dma_obj->hal_gdma_adaptor);
    hal_gdma_memcpy(phal_gdma_adaptor, dst, src, len);
}

