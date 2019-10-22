/**************************************************************************//**
 * @file     spdio_api.c
 * @brief    This file implements the SDIO device API functions.
 * 
 * @version  V1.00
 * @date     2017-09-20
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

#include "cmsis.h"

#if CONFIG_SDIO_DEVICE_EN

#include "spdio_api.h"
#include "hal_sdio_dev.h"

struct spdio_t *g_spdio_priv = NULL;

s8 spdio_rx_done_cb (void *padapter, u8 *data, u16 offset, u16 pktsize, u8 type)
{
	struct spdio_buf_t *buf = (struct spdio_buf_t *)data;
	struct spdio_t *obj = (struct spdio_t *)padapter;

	if(obj) {
		return obj->rx_done_cb(obj, buf, (u8 *)(buf->buf_addr+offset), pktsize, type);
    } else {
		SPDIO_API_PRINTK("spdio rx done callback function is null!");
    }
	return SUCCESS;
}

s8 spdio_tx_done_cb (void *padapter, void *data, u16 offset, u16 pktsize, u8 type)
{
	struct spdio_t *obj = (struct spdio_t *)padapter;
	struct spdio_buf_t *buf = (struct spdio_buf_t *)data;
    
	if(obj) {
		return obj->tx_done_cb(obj, buf);
    } else {
		SPDIO_API_PRINTK("spdio tx done callback function is null!");
    }
	return SUCCESS;		
}

s8 spdio_tx (struct spdio_t *obj, struct spdio_buf_t *pbuf)
{
	return hal_sdio_dev_rx_pkt_queue_push((u8 *)pbuf, 0, pbuf->buf_size, pbuf->type);
}

void spdio_structinit(struct spdio_t *obj)
{
	obj->rx_bd_bufsz = SPDIO_RX_BUFSZ_ALIGN(2048+24); //extra 24 bytes for sdio header
	obj->rx_bd_num = 24;
	obj->tx_bd_num = 24;
	obj->priv = NULL;
	obj->rx_buf = NULL;
	obj->rx_done_cb = NULL;
	obj->tx_done_cb = NULL;
}

void spdio_init(struct spdio_t *obj)
{
	if(obj == NULL){
		SPDIO_API_PRINTK("spdio obj is NULL, spdio init failed!");
		return;
	}
    
	if((obj->rx_bd_num == 0) ||(obj->rx_bd_bufsz == 0) ||  (obj->rx_bd_bufsz%64)
		||(obj->tx_bd_num == 0) ||(obj->tx_bd_num%2)||(obj->rx_buf == NULL))
	{
		SPDIO_API_PRINTK("spdio obj resource isn't correctly inited, spdio init failed!");
		return;
	}

	g_spdio_priv = obj;
    hal_sdio_dev_init ();
	hal_sdio_dev_register_tx_callback (spdio_rx_done_cb, (void *)obj);
	hal_sdio_dev_register_rx_done_callback (spdio_tx_done_cb, (void *)obj);
}

void spdio_deinit(struct spdio_t *obj)
{	
	if(obj == NULL){
		SPDIO_API_PRINTK("spdio obj is NULL, spdio deinit failed");
		return;
	}
    
	hal_sdio_dev_deinit();
	g_spdio_priv = NULL;
}

#endif  // end of "#if CONFIG_SDIO_DEVICE_EN"

