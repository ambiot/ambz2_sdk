/**************************************************************************//**
* @file        efuse_api.c
* @brief       This file implements the Efuse Mbed HAL API functions.
*
* @version     V1.00
* @date        2019-01-03
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
#include "hal_efuse.h"
#include "hal_efuse_nsc.h"
#include "efuse_api.h"
#include "hal.h"
#include "memory.h"

#if CONFIG_EFUSE_EN

#define EFUSE_SSLOCK_OFFSET     0x1B0

int efuse_otp_read(u8 offset, u8 len, u8 *buf)
{
    u8 content[32];	// the OTP max length is 32
	
	if ((offset+len) > 32) {
		return -1;
    }

    hal_user_otp_get(content);
	memcpy(buf, (content+offset), len);
	return 0;
}

int efuse_otp_write(u8 offset, u8 len, u8 *buf)
{
    u8 content[32];	// the OTP max length is 32
	
	if ((offset+len) > 32) {
		return -1;
    }

	memset(content, 0xFF, 32);
	memcpy((content+offset), buf, len);
	hal_user_otp_set(content);
	return 0;
}

int efuse_sec_key_write(u8 *buf, u8 key_num)
{
    if (hal_sec_key_write(buf, key_num) == _FALSE) {
        return -1;
    }
    return 0;
}

int efuse_susec_key_write(u8 *buf)
{
    if (hal_susec_key_write(buf) == _FALSE) {
        return -1;
    }
    return 0;
}

int efuse_s_jtag_key_write(u8 *buf)
{
    if (hal_s_jtag_key_write(buf) == _FALSE) {
        return -1;
    }
    return 0;
}

int efuse_ns_jtag_key_write(u8 *buf)
{
    if (hal_ns_jtag_key_write(buf) == _FALSE) {
        return -1;
    }
    return 0;
}

int efuse_disable_sec_jtag(void)
{
    hal_efuse_disable_sec_jtag();
    return 0;
}

int efuse_disable_nonsec_jtag(void)
{
    hal_efuse_disable_nonsec_jtag();
    return 0;
}

int efuse_lock_susec_key(void)
{
    uint32_t ret = _FALSE;
    uint8_t ss_lock = 0;

    ret = hal_efuse_read (EFUSE_SSLOCK_OFFSET, &ss_lock, LDO_OUT_DEFAULT_VOLT);    
    if (ret == _TRUE) {
        // if eFuse 0x1b0[0] = 0 -> lock SSE
        ss_lock &= ~0x01;
        ret = hal_efuse_write (EFUSE_SSLOCK_OFFSET, ss_lock, LDO_OUT_DEFAULT_VOLT);
        if (ret == _TRUE) {
            DBG_8710C("eFuse Key Locked!!, Super-Secure Key Reading is Inhibited!!\r\n");
            return (0);
        } else {
            return (-1);
        }
    } else {
        return (-1);
    }
}

void efuse_rotpk_enable(void)
{
	uint8_t rotpk_en = 0;
	hal_efuse_read(EFUSE_SSLOCK_OFFSET, &rotpk_en, LDO_OUT_DEFAULT_VOLT);
	rotpk_en &= ~(BIT(1));
	hal_efuse_write(EFUSE_SSLOCK_OFFSET, rotpk_en, LDO_OUT_DEFAULT_VOLT);
}
#endif
