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

#include <string.h>
#include "gpio_api.h"
#include "spi_api.h"
#include "spi_ex_api.h"
#include "wait_api.h"
#include "sys_api.h"

#define SPI_IS_AS_MASTER    1
#define TEST_BUF_SIZE       512
#define SCLK_FREQ           1000000
#define SPI_DMA_DEMO        0
#define GPIO_SYNC_PIN       PA_17

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

    //16 bytes per line
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
            dbg_printf("%02x ", (u8) buf[index2]);
        }

        if (max != 16) {
            if (max < 8) {
                dbg_printf("  ");
            }
            for (index2 = 16 - max; index2 > 0; index2--) {
                dbg_printf("   ");
            }
        }
    }

    dbg_printf("\r\n");
    return;
}

char TxBuf[TEST_BUF_SIZE];
char RxBuf[TEST_BUF_SIZE];

volatile int MasterRxDone;
volatile int SlaveRxDone;
gpio_t GPIO_Syc;

void master_trx_done_callback (void *pdata, SpiIrq event)
{
    switch (event) {
        case SpiRxIrq:
            dbg_printf("Master RX done! \r\n");
            MasterRxDone = 1;
            break;
        case SpiTxIrq:
            dbg_printf("Master TX done! \r\n");
            break;
        default:
            dbg_printf("unknown interrput evnent! \r\n");
    }
}

void slave_trx_done_callback (void *pdata, SpiIrq event)
{
    switch (event) {
        case SpiRxIrq:
            dbg_printf("Slave RX done! \r\n");
            SlaveRxDone = 1;
            break;
        case SpiTxIrq:
            dbg_printf("Slave TX done! \r\n");
            break;
        default:
            dbg_printf("unknown interrput evnent! \r\n");
    }
}

#if SPI_IS_AS_MASTER
spi_t spi_master;
#else
spi_t spi_slave;
#endif

int main (void)
{
    int i;

    dbg_printf("\r\n   SPI Stream Twoboard Concurrent DEMO   \r\n");

    if ((SPI0_SCLK == PA_3) || (SPI0_CS == PA_2)) {
        sys_jtag_off();
    }

    gpio_init(&GPIO_Syc, GPIO_SYNC_PIN);
    gpio_write(&GPIO_Syc, 1);//Initialize GPIO Pin to high 
#if SPI_IS_AS_MASTER 
    gpio_dir(&GPIO_Syc, PIN_INPUT);     // Direction: Input
    gpio_mode(&GPIO_Syc, PullUp);       // Pull up
#else
    gpio_dir(&GPIO_Syc, PIN_OUTPUT);    // Direction: Output
    gpio_mode(&GPIO_Syc, PullNone);     // No pull
#endif

#if SPI_IS_AS_MASTER
    spi_init(&spi_master, SPI0_MOSI, SPI0_MISO, SPI0_SCLK, SPI0_CS);
    spi_format(&spi_master, DfsEightBits, ((int)SPI_SCLK_IDLE_LOW | (int)SPI_SCLK_TOGGLE_MIDDLE), 0);
    spi_frequency(&spi_master, SCLK_FREQ);
    hal_ssi_toggle_between_frame(&spi_master.hal_ssi_adaptor, ENABLE);
    // wait Slave ready
    MasterRxDone = 0;
    wait_ms(1000);
    memset(TxBuf, 0, TEST_BUF_SIZE);
    memset(RxBuf, 0, TEST_BUF_SIZE);

    for (i = 0; i < TEST_BUF_SIZE; i++) {
        TxBuf[i] = i;
    }

    spi_irq_hook(&spi_master, (spi_irq_handler)master_trx_done_callback, (uint32_t)&spi_master);
    spi_flush_rx_fifo(&spi_master);

    while(gpio_read(&GPIO_Syc) == 1);
#if SPI_DMA_DEMO
    spi_master_write_read_stream_dma(&spi_master, TxBuf, RxBuf, TEST_BUF_SIZE);
#else
    spi_master_write_read_stream(&spi_master, TxBuf, RxBuf, TEST_BUF_SIZE);
#endif

    while (MasterRxDone == 0) {
        wait_ms(1000);
    }
    dump_data((u8 *)RxBuf, TEST_BUF_SIZE, "SPI Master Read Data:");
    spi_free(&spi_master);

#else
    spi_init(&spi_slave, SPI0_MOSI, SPI0_MISO, SPI0_SCLK, SPI0_CS);
    spi_format(&spi_slave, DfsEightBits, ((int)SPI_SCLK_IDLE_LOW | (int)SPI_SCLK_TOGGLE_MIDDLE), 1);
    hal_ssi_toggle_between_frame(&spi_slave.hal_ssi_adaptor, ENABLE);

    while (spi_busy(&spi_slave)) {
        dbg_printf("Wait SPI Bus Ready... \r\n");
        wait_ms(1000);
    }
    SlaveRxDone = 0;

    memset(TxBuf, 0, TEST_BUF_SIZE);
    memset(RxBuf, 0, TEST_BUF_SIZE);
    for (i = 0; i < TEST_BUF_SIZE; i++) {
        TxBuf[i] = ~i;
    }
    spi_irq_hook(&spi_slave, (spi_irq_handler)slave_trx_done_callback, (uint32_t)&spi_slave);
    spi_flush_rx_fifo(&spi_slave);
#if SPI_DMA_DEMO
    spi_slave_read_stream_dma(&spi_slave, RxBuf, TEST_BUF_SIZE);
    spi_slave_write_stream_dma(&spi_slave, TxBuf, TEST_BUF_SIZE);
#else
    spi_slave_read_stream(&spi_slave, RxBuf, TEST_BUF_SIZE);
    spi_slave_write_stream(&spi_slave, TxBuf, TEST_BUF_SIZE);
#endif
    gpio_write(&GPIO_Syc, 0);

    i = 0;
    dbg_printf("SPI Slave Wait Read Done... \r\n");
    while (SlaveRxDone == 0) {
        wait_ms(100);
        i++;
        if (i > 150) {
            dbg_printf("SPI Slave Wait Timeout \r\n");
            break;
        }
    }

    gpio_write(&GPIO_Syc, 1);
    dump_data((uint8_t *)RxBuf, TEST_BUF_SIZE, "SPI Slave Read Data:");
    spi_free(&spi_slave);
#endif

    dbg_printf("SPI Demo finished. \r\n");
    for(;;);
}
