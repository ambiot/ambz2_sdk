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

#include "gpio_api.h"
#include "spi_api.h"
#include "spi_ex_api.h"
#include "wait_api.h"
#include "sys_api.h"

#define TEST_BUF_SIZE       1024
#define SCLK_FREQ           1000000
#define TEST_LOOP           100
#define SPI_GPIO_CS         PA_17

#define SPI0_MOSI           PA_19
#define SPI0_MISO           PA_20
#define SPI0_SCLK           PA_3
#define SPI0_CS             PA_2

extern void hal_ssi_toggle_between_frame(phal_ssi_adaptor_t phal_ssi_adaptor, u8 ctl);

void dump_data (const u8 *start, u32 size, char * strHeader)
{
    int row, column, index, index2, max;
    u8 *buf, *line;

    if (!start || (size == 0)) {
        return;
    }

    line = (u8*)start;

    /*
    16 bytes per line
    */
    if (strHeader) {
       dbg_printf("%s", strHeader);
    }

    column = size % 16;
    row = (size / 16) + 1;
    for (index = 0; index < row; index++, line += 16) {
        buf = (u8*)line;

        max = (index == row - 1) ? column : 16;
        if (max == 0) {
            break; /* If we need not dump this line, break it. */
        }

        dbg_printf("\r\n[%08x] ", line);

        //Hex
        for (index2 = 0; index2 < max; index2++) {
            if (index2 == 8) {
                dbg_printf("  ");
            }
            dbg_printf("%02x ", (u8)buf[index2]);
        }

        if (max != 16) {
            if (max < 8) {
                dbg_printf("  ");
            }
            for ((index2 = 16 - max); index2 > 0; index2--) {
                dbg_printf("   ");
            }
        }
    }

    dbg_printf("\r\n");
    return;
}

char TestBuf[TEST_BUF_SIZE];
volatile int TrDone;
gpio_t spi_cs;

void master_tr_done_callback (void *pdata, SpiIrq event)
{
    TrDone = 1;
}

void master_cs_tr_done_callback (void *pdata, SpiIrq event)
{
    /*Disable CS by pulling gpio to low*/
    gpio_write(&spi_cs, 0);
    dbg_printf("SPI Master CS High==> \r\n");
}

spi_t spi_master;

int main (void)
{
    int Counter = 0;
    int i;

    dbg_printf("\r\n   SPI GPIO Chipselect DEMO   \r\n");

    if ((SPI0_SCLK == PA_3) || (SPI0_CS == PA_2)) {
        sys_jtag_off();
    }

    gpio_init(&spi_cs, SPI_GPIO_CS);
    gpio_write(&spi_cs, 0);           // Initialize GPIO Pin to low
    gpio_dir(&spi_cs, PIN_OUTPUT);    // Direction: Output
    gpio_mode(&spi_cs, PullNone);     // No pull

    spi_init(&spi_master, SPI0_MOSI, SPI0_MISO, SPI0_SCLK, SPI0_CS);
    spi_format(&spi_master, DfsEightBits, ((int)SPI_SCLK_IDLE_LOW | (int)SPI_SCLK_TOGGLE_START), 0);
    spi_frequency(&spi_master, SCLK_FREQ);
    hal_ssi_toggle_between_frame(&spi_master.hal_ssi_adaptor, ENABLE);
    dbg_printf("Test Start \r\n");

    for (i = 0; i < TEST_BUF_SIZE; i++) {
        TestBuf[i] = i;
    }
    while (Counter < TEST_LOOP) {
        dbg_printf("======= Test Loop %d ======= \r\n", Counter);

        spi_irq_hook(&spi_master, (spi_irq_handler)master_tr_done_callback, (uint32_t)&spi_master);
        spi_bus_tx_done_irq_hook(&spi_master, (spi_irq_handler)master_cs_tr_done_callback, (uint32_t)&spi_master);
        dbg_printf("SPI Master Write Test==> \r\n");
        TrDone = 0;
        /*Enable CS by pulling gpio to high*/
        gpio_write(&spi_cs, 1);
        spi_master_write_stream(&spi_master, TestBuf, TEST_BUF_SIZE);

        i=0;
        dbg_printf("SPI Master Wait Write Done... \r\n");
        while (TrDone == 0) {
            wait_ms(10);
            i++;
        }
        dbg_printf("SPI Master Write Done!! \r\n");
        wait_ms(10000);
        Counter++;
    }
    spi_free(&spi_master);
    dbg_printf("SPI Master Test Done<== \r\n");

    dbg_printf("SPI Demo finished. \r\n");
    for(;;);
}
