/**************************************************************************//**
 * @file     rtl8710c_symbns4s.h
 * @brief    The ROM code symbols exportted by NS region and will be
 *           referenced by S region code.
 * @version  V1.00
 * @date     2016-11-08
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2016 Realtek Corporation. All rights reserved.
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


#ifndef _RTL8710C_SYMBNS4S_H_
#define _RTL8710C_SYMBNS4S_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "shell.h"

#define NS_SYMBOL_STUBS_ADDR                    0x0005FF80

//extern cmd_shell_func_stubs_t cmd_shell_stubs;
//extern hal_gpio_func_stubs_t hal_gpio_stubs;

typedef struct symb_ns4s_s {
    const cmd_shell_func_stubs_t *pcmd_shell_stubs;
    const hal_gpio_func_stubs_t *phal_gpio_stubs;
    const hal_spic_func_stubs_t *phal_spic_stubs;
    const hal_flash_func_stubs_t *phal_flash_stubs;
    const hal_sdiod_func_stubs_t *phal_sdiod_stubs;
    phal_timer_adapter_t *ppsystem_timer;
    void (*startup_ns) (void);
    void (*stdio_port_init_ns) (void *adapter, stdio_putc_t putc, stdio_getc_t getc);
    void (*shell_cmd_task_init) (void);
    void (*curve25519_donna)(unsigned char *, const unsigned char *, const unsigned char *);
    void (*rom_cmd_shell_set_prompt) (void);

    uint32_t reserved[16];
} symb_ns4s_t;

#ifdef __cplusplus
}

#endif

#endif  // #ifndef _RTL8710C_SYMBNS4S_H_

