/**************************************************************************//**
* @file        hal_efuse_nsc.c
* @brief       This file implements the entry functions of the Efuse Non-secure callable HAL functions.
*
* @version     V1.00
* @date        2018-07-26
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

#if CONFIG_EFUSE_EN && CONFIG_EFUSE_NSC_EN

#if !defined(CONFIG_BUILD_SECURE) && !defined(CONFIG_BUILD_NONSECURE)
#undef SECTION_NS_ENTRY_FUNC
#undef NS_ENTRY
#define SECTION_NS_ENTRY_FUNC
#define NS_ENTRY
#endif 
/**
 * @addtogroup hs_hal_efuse EFUSE
 * @{
 */

/**
 *  @brief The NSC function to enable efuse syson autoload.
 *
 *  @param[in]  enable  enable.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_efuse_autoload_en_nsc(uint8_t enable)
{
    return hal_efuse_autoload_en(enable);
}

/**
 *  @brief The NSC function to enable efuse hci autoload.
 *
 *  @param[in]  enable  enable.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_efuse_hci_autoload_en_nsc(uint8_t enable)
{
    return hal_efuse_hci_autoload_en(enable);
}

/**
 *  @brief The NSC function to read efuse.
 *
 *  @param[in]  addr            read address.
 *  @param[in]  pdata           address of read back data.
 *  @param[in]  l25out_voltage  LDOE25 voltage select.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_efuse_read_nsc(uint16_t addr, uint8_t *pdata, uint8_t l25out_voltage)
{
    return hal_efuse_read(addr, pdata, l25out_voltage);
}

/**
 *  @brief The NSC function to write efuse.
 *
 *  @param[in]  addr            write address.
 *  @param[in]  data            write data.
 *  @param[in]  l25out_voltage  LDOE25 voltage select.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_efuse_write_nsc(uint16_t addr, uint8_t data, uint8_t l25out_voltage)
{
    return hal_efuse_write(addr, data, l25out_voltage);
}

/**
 *  @brief The NSC function to read security efuse.
 *
 *  @param[in]  addr            read address.
 *  @param[in]  pdata           address of read back data.
 *  @param[in]  l25out_voltage  LDOE25 voltage select.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_sec_efuse_read_nsc(uint16_t addr, uint8_t *pdata, uint8_t l25out_voltage)
{
    return hal_sec_efuse_read(addr, pdata, l25out_voltage);
}

/**
 *  @brief The NSC function to write security efuse.
 *
 *  @param[in]  addr            write address.
 *  @param[in]  data            write data.
 *  @param[in]  l25out_voltage  LDOE25 voltage select.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_sec_efuse_write_nsc(uint16_t addr, uint8_t data, uint8_t l25out_voltage)
{
    return hal_sec_efuse_write(addr, data, l25out_voltage);
}

/**
 *  @brief The NSC function to read security key.
 *
 *  @param[in]  psec_key        adress of read back security key.
 *  @param[in]  key_num         select key number.
 *  @param[in]  length          security key length.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_sec_key_get_nsc(uint8_t *psec_key, uint8_t key_num, uint32_t length)
{
    return hal_sec_key_get(psec_key, key_num, length);
}

/**
 *  @brief The NSC function to write security key.
 *
 *  @param[in]  psec_key        address of 32-byte security key.
 *  @param[in]  key_num         select key number.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_sec_key_write_nsc(uint8_t *psec_key, uint8_t key_num)
{
    return hal_sec_key_write(psec_key, key_num);
}

/**
 *  @brief The NSC function to read super security key.
 *
 *  @param[in]  psec_key        address of read back 32-byte super security key.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_susec_key_get_nsc(uint8_t *psusec_key)
{
    return hal_susec_key_get(psusec_key);
}

/**
 *  @brief The NSC function to write super security key.
 *
 *  @param[in]  psec_key        address of 32-byte super security key.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_susec_key_write_nsc(uint8_t *psusec_key)
{
    return hal_susec_key_write(psusec_key);
}

/**
 *  @brief The NSC function to write a 128-bit security j-tag key.
 *
 *  @param[in]  pkey            address of 16-byte S-JTAG key.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_s_jtag_key_write_nsc(uint8_t *pkey)
{
    return hal_s_jtag_key_write(pkey);
}

/**
 *  @brief The NSC function to write a 128-bit non-security j-tag key.
 *
 *  @param[in]  pkey            address of 16-byte NS-JTAG key.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_ns_jtag_key_write_nsc(uint8_t *pkey)
{
    return hal_ns_jtag_key_write(pkey);
}


/**
 *  @brief The NSC function to get user otp
 *
 *  @param[in]  puser_otp       adress of read back user otp.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_user_otp_get_nsc(uint8_t *puser_otp)
{
    return hal_user_otp_get(puser_otp);
}

/**
 *  @brief The NSC function to set user otp
 *
 *  @param[in]  puser_otp       address of user otp value.
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_user_otp_set_nsc(uint8_t *puser_otp)
{
    return hal_user_otp_set(puser_otp);
}

/**
 *  @brief The NSC function to disable secure j-tag
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_efuse_disable_sec_jtag_nsc(void)
{
    return hal_efuse_disable_sec_jtag();
}

/**
 *  @brief The NSC function to disable non-secure j-tag
 *
 *  @return  TRUE
 *  @return  FALSE
 */
SECTION_NS_ENTRY_FUNC
uint32_t NS_ENTRY hal_efuse_disable_nonsec_jtag_nsc(void)
{
    return hal_efuse_disable_nonsec_jtag();
}

/** @} */ /* End of group hs_hal_efuse */

//#endif

#endif  /* end of "#if CONFIG_EFUSE_EN && CONFIG_EFUSE_NSC_EN" */

