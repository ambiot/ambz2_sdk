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

// Decide starting flash address for storing application data
// User should pick address carefully to avoid corrupting image section
#define FLASH_APP_BASE    0x40000    //the start address of the fourth block
#define length            8

int main (void)
{
    flash_t flash;
    uint32_t address = FLASH_APP_BASE;

    int result = 1;
    int i = 0;
    //int length = 8;
    int loop = 0;

    char data[length];
    char buff[length];

    dbg_printf("\r\n   FLASH Micron Block Protect DEMO   \r\n");

    for (loop = 0; loop < 8; loop++) {
        dbg_printf(ANSI_COLOR_MAGENTA"Test Address = %x \r\n"ANSI_COLOR_RESET, address);
        flash_erase_sector(&flash, address);

        for (i = 0; i < length; i++) {
            data[i] = i;
        }

        flash_burst_write(&flash, address, length, (uint8_t *)&data[0]);
        flash_stream_read(&flash, address, length, (uint8_t *)&buff[0]);

        dbg_printf("Before Lock \r\n");

        for (i = 0; i < length; i++) {
            if (data[i] != buff[i]) {
                dbg_printf(ANSI_COLOR_YELLOW"Error : Write Data is = %x, Read Data is %x \r\n"ANSI_COLOR_RESET, data[i], buff[i]);
                result = 0;
            }
        }
        if (result == 1) {
            dbg_printf("Success 1 \r\n");
        }

        dbg_printf("Lock first 8 blocks \r\n");
        result = 1;
        dbg_printf("Status Register Before Setting= %x \r\n", flash_get_status(&flash));

        flash_set_status(&flash, 0x30);    //Protect 0~7 sectors

        flash_erase_sector(&flash, address);    //This erase should be ignored if the block is protected

        for (i = 0; i < length; i++) {
            data[i] = ~i;
        }

        flash_burst_write(&flash, address, length, (uint8_t *)&data[0]);
        flash_stream_read(&flash, address, length, (uint8_t *)&buff[0]);
            for (i = 0; i < length; i++) {
                if (data[i] != buff[i]) {
                    dbg_printf(ANSI_COLOR_YELLOW"Error : Write Data is = %x, Read Data is %x \r\n"ANSI_COLOR_RESET, data[i], buff[i]);
                    result = 0;
                }
            }
        if (result == 1) {
            dbg_printf("Success 2 \r\n");
        }

        dbg_printf("Unlock \r\n");
        result = 1;
        dbg_printf("Status Register Before Setting= %x \r\n", flash_get_status(&flash));

        flash_set_status(&flash, flash_get_status(&flash) & (~0x30));    //Unlock the protected block

        flash_erase_sector(&flash, address);    //Now the erase operation should be valid

        flash_burst_write(&flash, address, length, (uint8_t *)&data[0]);

        flash_stream_read(&flash, address, length, (uint8_t *)&buff[0]);

        for (i = 0; i < length; i++) {
            if (data[i] != buff[i]) {
                dbg_printf(ANSI_COLOR_YELLOW"Error : Write Data is = %x, Read Data is %x \r\n"ANSI_COLOR_RESET, data[i], buff[i]);
                result = 0;
            }
        }
        if (result == 1) {
            dbg_printf("Success 3 \r\n");
        }

        flash_reset_status(&flash);    //make sure the status register is reset if users would like to reload code

        dbg_printf("Status Register After Reset= %x \r\n", flash_get_status(&flash));
        address += 0x10000;
    }

    dbg_printf("Test Done \r\n");
    for(;;);
}
