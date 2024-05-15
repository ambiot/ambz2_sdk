/**************************************************************************//**
 * @file     app_start.c
 * @brief    The application entry function implementation. It initial the 
 *           application functions.
 * @version  V1.00
 * @date     2016-05-27
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

#undef ROM_REGION

#include "cmsis.h"
#include "platform_opts.h"
#include "hal_sys_ctrl.h"
#include "partition_rtl8710c.h"
#include "crypto_api.h"

extern void shell_cmd_init (void);
extern void shell_task (void);
extern void bus_idau_setup(uint32_t idau_idx, uint32_t start_addr, uint32_t end_addr);

//void app_start (void) __attribute__ ((noreturn));

#if !defined(PLATFORM_OHOS) && !defined ( __CC_ARM ) && !(defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) /* ARM Compiler 4/5 */
// for __CC_ARM compiler, it will add a __ARM_use_no_argv symbol for every main() function, that cause linker report error 
/// default main
__weak int main (void)
{
    while (1) {
        shell_task ();
    }
    //return 0;
}
#endif

__weak void __iar_data_init3(void)
{

}

__weak void __iar_zero_init3(void)
{

}

#if (defined(CONFIG_MIIO)&&(CONFIG_MIIO))
void HalHardFaultHandler_Patch_c(u32 HardDefaultArg, u32 lr)
{
    uint32_t crash_flag = 0xE32C04EDUL;
    extern int arch_psm_set_value(const char*, const char*, const void*, uint32_t);
    arch_psm_set_value("arch", "crash_flag", &crash_flag, sizeof(uint32_t));
    ((int_vector_t)(0xab5))();
}

void HalHardFaultHandler_Patch_asm(void)
{
    asm("TST LR, #4\n"
        "ITE EQ\n"
        "MRSEQ R0, MSP\n"
        "MRSNE R0, PSP\n"
        "MOV R1, LR\n"
        "B HalHardFaultHandler_Patch_c");
}
#endif

#if defined(CONFIG_BUILD_SECURE)
/** 
 *  @brief To setup SAUs to partition the system memory as Secure memory and Non-secure memory.
 *  @param[in]   ns_region  The table for this function to return the memory range of NS world.
 *                          For the boot flow to check whether the NS world entry is located in
 *                          NS world memory.
 *
 *  @returns void
 */
void sau_setup(ns_region_t *ns_region)
{
#if SAU_INIT_REGION0
    //dbg_printf ("SAU 0: 0x%08x ~ 0x%08x as %s\r\n", SAU_INIT_START0, SAU_INIT_END0, SAU_INIT_NSC0?"Secure(NSC)":"Non-Secure");
    if (SAU_INIT_NSC0 == 0) {
        // Set Bus IDAU to match the SAU setting, use IDAU2
        bus_idau_setup(0, SAU_INIT_START0, SAU_INIT_END0);
    }
    ns_region[0].start_addr = SAU_INIT_START0;
    ns_region[0].end_addr = SAU_INIT_END0;
    ns_region[0].is_valid = 1;
#endif
#if SAU_INIT_REGION1
    //dbg_printf ("SAU 1: 0x%08x ~ 0x%08x as %s\r\n", SAU_INIT_START1, SAU_INIT_END1, SAU_INIT_NSC1?"Secure(NSC)":"Non-Secure");
    if (SAU_INIT_NSC1 == 0) {
        // Set Bus IDAU to match the SAU setting, use IDAU3
        bus_idau_setup(1, SAU_INIT_START1, SAU_INIT_END1);
    }
    ns_region[1].start_addr = SAU_INIT_START1;
    ns_region[1].end_addr = SAU_INIT_END1;
    ns_region[1].is_valid = 1;
#endif

#if SAU_INIT_REGION2
    //dbg_printf ("SAU 2: 0x%08x ~ 0x%08x as %s\r\n", SAU_INIT_START2, SAU_INIT_END2, SAU_INIT_NSC2?"Secure(NSC)":"Non-Secure");
    if (SAU_INIT_NSC2 == 0) {
    // Set Bus IDAU to match the SAU setting, use IDAU0
        bus_idau_setup(2, SAU_INIT_START2, SAU_INIT_END2);
    }
    ns_region[2].start_addr = SAU_INIT_START2;
    ns_region[2].end_addr = SAU_INIT_END2;
    ns_region[2].is_valid = 1;
#endif
#if SAU_INIT_REGION3
    //dbg_printf ("SAU 3: 0x%08x ~ 0x%08x as %s\r\n", SAU_INIT_START3, SAU_INIT_END3, SAU_INIT_NSC3?"Secure(NSC)":"Non-Secure");
    if (SAU_INIT_NSC3 == 0) {
        // Set Bus IDAU to match the SAU setting, use IDAU0
        bus_idau_setup(3, SAU_INIT_START3, SAU_INIT_END3);

        ns_region[3].start_addr = SAU_INIT_START3;
        ns_region[3].end_addr = SAU_INIT_END3;
        ns_region[3].is_valid = 1;
    }
#endif
    TZ_SAU_Setup();
}
#endif


#if defined (__ICCARM__)
extern void *__iar_cstart_call_ctors(void *ptr);
#elif defined (__GNUC__)
extern void __libc_init_array(void);
void _init(void){}
#endif

#if defined(CONFIG_BUILD_SECURE)
NS_ENTRY
#endif
void __secure_init_array( void )
{
#if defined (__ICCARM__)
    // __iar_data_init3 replaced by __iar_cstart_call_ctors, just do c++ constructor
    __iar_cstart_call_ctors(NULL);
#elif defined (__GNUC__)
    __libc_init_array();
#endif
}

#ifdef PLATFORM_OHOS
#include "los_tick_pri.h"
extern VOID osPendSV(VOID);
extern VOID OsHwiDefaultHandler(VOID);
extern VOID SysTick_Handler(VOID);
#endif
extern int_vector_t ram_vector_table[];

static u32 rand_seed[4]; //z1, z2, z3, z4,
static u32 rand_first;
u32 Rand (void)
{
    u32 b;

    if (!rand_first) {
        rand_seed[0] = 12345;
        rand_seed[1] = 12345;
        rand_seed[2] = 12345;
        rand_seed[3] = 12345;
        rand_first = 1;
    }

    b  = ((rand_seed[0] << 6) ^ rand_seed[0]) >> 13;
    rand_seed[0] = ((rand_seed[0] & 4294967294U) << 18) ^ b;
    b  = ((rand_seed[1] << 2) ^ rand_seed[1]) >> 27;
    rand_seed[1] = ((rand_seed[1] & 4294967288U) << 2) ^ b;
    b  = ((rand_seed[2]<< 13) ^ rand_seed[2]) >> 21;
    rand_seed[2] = ((rand_seed[2] & 4294967280U) << 7) ^ b;
    b  = ((rand_seed[3] << 3) ^ rand_seed[3]) >> 12;
    rand_seed[3] = ((rand_seed[3] & 4294967168U) << 13) ^ b;
    return (rand_seed[0] ^ rand_seed[1] ^ rand_seed[2] ^ rand_seed[3]);
}

static void app_gen_random_seed(void)
{
    u8 random[4] = {0};
    u32 data;
      /* to reset gtimer8 */
    hal_timer_adapter_t ls_timer;
    hal_timer_init (&ls_timer, GTimer8);
    hal_timer_deinit(&ls_timer);
    crypto_init();
    crypto_random_generate((unsigned char *)random, 4);
    rand_first = 1;
    data = (random[3] << 24) | (random[2] << 16) | (random[1] << 8) | (random[0]);
    rand_seed[0] = data;
    rand_seed[1] = data;
    rand_seed[2] = data;
    rand_seed[3] = data;
}

#if defined(CONFIG_BUILD_SECURE)
SECTION_NS_ENTRY_FUNC
void NS_ENTRY app_gen_random_seed_nsc(void)
#else
void app_gen_random_seed_nsc(void)
#endif
{
    app_gen_random_seed();
}

void app_start (void)
{
	dbg_printf ("Build @ %s, %s\r\n", __TIME__, __DATE__);

#if (defined(CONFIG_MIIO)&&(CONFIG_MIIO))
	ram_vector_table[3] = (int_vector_t)HalHardFaultHandler_Patch_asm;
	ram_vector_table[4] = (int_vector_t)HalHardFaultHandler_Patch_asm;
	ram_vector_table[5] = (int_vector_t)HalHardFaultHandler_Patch_asm;
	ram_vector_table[6] = (int_vector_t)HalHardFaultHandler_Patch_asm;
	ram_vector_table[7] = (int_vector_t)HalHardFaultHandler_Patch_asm;
#endif
#ifdef PLATFORM_OHOS	
	ram_vector_table[2] = (int_vector_t)OsHwiDefaultHandler;
	ram_vector_table[3] = (int_vector_t)OsHwiDefaultHandler;
	ram_vector_table[4] = (int_vector_t)OsHwiDefaultHandler;
	ram_vector_table[5] = (int_vector_t)OsHwiDefaultHandler;
	ram_vector_table[6] = (int_vector_t)OsHwiDefaultHandler;
	ram_vector_table[14] = (int_vector_t)osPendSV;
	ram_vector_table[15] = (int_vector_t)SysTick_Handler;
#endif

	__secure_init_array();
	shell_cmd_init ();
	app_gen_random_seed();
	main();
#if defined ( __ICCARM__ )
	// for compile issue, If user never call this function, Liking fail
	__iar_data_init3();
#endif
}
