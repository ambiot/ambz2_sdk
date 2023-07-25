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

extern void shell_cmd_init (void);
extern void shell_task (void);
extern void bus_idau_setup(uint32_t idau_idx, uint32_t start_addr, uint32_t end_addr);

//void app_start (void) __attribute__ ((noreturn));

#if !defined ( __CC_ARM ) && !(defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) /* ARM Compiler 4/5 */
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

#ifdef ENABLE_AMAZON_COMMON
__weak void os_heap_init(void) { /* default os_heap_init, for FreeRTOS heap5 */ }
#endif

void app_start (void)
{
	dbg_printf ("Build @ %s, %s\r\n", __TIME__, __DATE__);

#ifdef ENABLE_AMAZON_COMMON
	os_heap_init();
#endif

#if (defined(CONFIG_MIIO)&&(CONFIG_MIIO))
	extern int_vector_t ram_vector_table[];
	ram_vector_table[3] = (int_vector_t)HalHardFaultHandler_Patch_asm;
	ram_vector_table[4] = (int_vector_t)HalHardFaultHandler_Patch_asm;
	ram_vector_table[5] = (int_vector_t)HalHardFaultHandler_Patch_asm;
	ram_vector_table[6] = (int_vector_t)HalHardFaultHandler_Patch_asm;
	ram_vector_table[7] = (int_vector_t)HalHardFaultHandler_Patch_asm;
#endif
	
#if defined (__ICCARM__)
	// __iar_data_init3 replaced by __iar_cstart_call_ctors, just do c++ constructor
	//__iar_cstart_call_ctors(NULL);
#endif

	shell_cmd_init ();
	main();
#if defined ( __ICCARM__ )
	// for compile issue, If user never call this function, Liking fail
	__iar_data_init3();
#endif
}


