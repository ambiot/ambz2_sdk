/*
 * Copyright(c) 2007 - 2019 Realtek Corporation. All rights reserved.
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
 */

#include "flash_api.h"

#define FLASH_APP_BASE    0xFF000

uint32_t check_dword_before_erase[1024];
uint32_t check_dword_after_erase[1024];

flash_t flash;
uint32_t address = FLASH_APP_BASE;

uint32_t val32_to_write_dword_1 =       0x12345678;
uint32_t val32_to_write_dword_2 =       0x11223344;
uint32_t val32_to_write_dword_100 =     0x55667788;
uint32_t val32_to_write_dword_1024 =    0xaabbccdd;
uint32_t val32_to_read;

int result = 0;
int loop = 0;

extern void flash_erase_dword(u32 address, u32 dword_num);

static void flash_erase_dword_test (u32 dword_num)
{
    result = 0;

    flash_erase_sector(&flash, address);
    flash_write_word(&flash, address, val32_to_write_dword_1);
    flash_write_word(&flash, (address + 4), val32_to_write_dword_2);
    flash_write_word(&flash, (address + (4 * (100 - 1))), val32_to_write_dword_100);
    flash_write_word(&flash, (address + (4 * (1024 - 1))), val32_to_write_dword_1024);

    for (loop = 0; loop < 1024; loop++) {
        flash_read_word(&flash, (address + (uint32_t)(loop * 4)), &val32_to_read);
        check_dword_before_erase[loop] = val32_to_read;
    }

    flash_erase_dword(address, dword_num);

    for (loop = 0; loop < 1024; loop++) {
        flash_read_word(&flash, (address + (uint32_t)(loop * 4)), &val32_to_read);
        check_dword_after_erase[loop] = val32_to_read;
    }

    for (loop = 0; loop < dword_num; loop++) {
        if (check_dword_after_erase[loop] != 0xffffffff) {
            dbg_printf(" erease dword error \r\n");
            result++;
        }
    }

    for (loop = dword_num; loop < 1024; loop++) {
        if (check_dword_before_erase[loop] != check_dword_after_erase[loop]) {
            dbg_printf(" copy from backup memory error. \r\n");
            dbg_printf(" address :   0x%x \r\n", (address + (uint32_t)(loop * 4)));
            dbg_printf(" dword :     0x%x \r\n", (check_dword_after_erase[loop]));
            result++;
        }
    }

// check the erase dword sector
#if 1
    for (loop = 0; loop < 1024; loop++) {
        dbg_printf("\r\n dword :   %04d   0x%x ", (loop + 1), (check_dword_after_erase[loop]));
    }
#endif

    if (result != 0) {
        dbg_printf("\r\nResult of erase %d dword is %s ! \r\n\n", dword_num, "fail");
    } else {
        dbg_printf("\r\nResult of erase %d dword is %s ! \r\n\n", dword_num, "success");
    }
}

static void flash_test_task(void *param)
{
    dbg_printf("\r\n   FLASH Erase Dword DEMO   \r\n");

    flash_erase_dword_test(1);
    flash_erase_dword_test(2);
    flash_erase_dword_test(100);
    flash_erase_dword_test(1024);

    dbg_printf("Test Done!!! \r\n");

    vTaskDelete(NULL);
}

int main (void)
{
    if (xTaskCreate(flash_test_task, ((const char*)"flash_test_task"), 1024, NULL, (tskIDLE_PRIORITY + 1), NULL) != pdPASS) {
        dbg_printf("\n\r%s xTaskCreate(flash_test_task) failed", __FUNCTION__);
    }
    vTaskStartScheduler();
    while (1) {
        vTaskDelay((1000 / portTICK_RATE_MS));
    }
}
