/**************************************************************************//**
* @file        efuse_logical_api.h
* @brief       This file implements the Efuse Mbed HAL API functions.
*
* @version     V1.00
* @date        2019-01-15
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

#ifndef MBED_EXT_EFUSE_LOGICAL_API_EXT_H
#define MBED_EXT_EFUSE_LOGICAL_API_EXT_H
 
#include "device.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup efuse EFUSE
 *  @ingroup    hal
 *  @brief      efuse functions
 */

/**
  * @brief  Read efuse content on logical map
  * @param  laddr: address on logical map
  * @param  size: size of wanted data
  * @param  pbuf: buffer of read data
  * @retval : return number of used bytes
  */
int efuse_logical_read(u16 laddr, u16 size, u8 *pbuf);

/**
  * @brief  write in pg packet format 
  * @param  offset: offset is to set the number of 8 bytes
  * @param  wden: word enable is to decide which word need to be written
  * @param  data: data need to be written
  * @retval 0: success -1: failure 
  */
//static int efuse_pg_packet(u8 offset , u8 wden, u8 *data);

/**
  * @brief  Write user's content to efuse on logical map
  * @param  addr: address on logical map
  * @param  cnts: how many bytes of data
  * @param  data: data need to be written
  * @retval 0: success <0: failure   
  */
int efuse_logical_write(u16 addr, u16 cnts, u8 *data);

/**
  * @brief  To enable secure boot
  * @retval 0: success <0: failure 
  */
int efuse_fw_verify_enable(void);

/**
  * @brief  To check the secure boot is enabled or not
  * @retval 1: success 0: failure 
  */
int efuse_fw_verify_check(void);

/**
  * @brief  Disable ROM boot debugging message
  * @retval   status: Success:0 or Failure: -1.
  */
int efuse_boot_message_disable(void);

/**
  * @brief  Enable ROM boot debugging message
  * @retval   status: Success:0 or Failure: -1.
  */
int efuse_boot_message_enable(void);

#ifdef __cplusplus
}
#endif

#endif 