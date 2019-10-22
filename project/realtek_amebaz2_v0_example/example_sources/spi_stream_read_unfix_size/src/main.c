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

#define SPI_DMA_DEMO                0
#define SPI_IS_AS_MASTER            1
#define TEST_BUF_SIZE               2048
#define TEST_RECEIVE_BUF_SIZE       4096
#define SCLK_FREQ                   1000000

#define SPI0_MOSI                   PA_19
#define SPI0_MISO                   PA_20
#define SPI0_SCLK                   PA_3
#define SPI0_CS                     PA_2
#define GPIO_SYNC_PIN               PA_17

extern void hal_ssi_toggle_between_frame(phal_ssi_adaptor_t phal_ssi_adaptor, u8 ctl);

char TestBuf[TEST_BUF_SIZE];
volatile int MasterTxDone;
volatile int MasterRxDone;
volatile int SlaveTxDone;
volatile int SlaveRxDone;
gpio_t GPIO_Syc;

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
            for ((index2 = 16 - max); index2 > 0; index2--) {
                dbg_printf("   ");
            }
        }
    }
    dbg_printf("\r\n");
    return;
}

char TestBuf[TEST_BUF_SIZE];
char TestReceiveBuf[TEST_RECEIVE_BUF_SIZE];
void master_tr_done_callback (void *pdata, SpiIrq event)
{
    MasterTxDone = 1;
}

void slave_tr_done_callback (void *pdata, SpiIrq event)
{
    SlaveRxDone = 1;
}

#if SPI_IS_AS_MASTER
spi_t spi_master;
#else
spi_t spi_slave;
extern int32_t spi_slave_read_stream_unfix_size(spi_t *obj, char *rx_buffer, uint32_t length);
#endif

int main (void)
{
    int i;

    dbg_printf("\r\n   SPI Stream Read Unfix Size DEMO   \r\n");

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
    spi_format(&spi_master, DfsSixteenBits, ((int)SPI_SCLK_IDLE_LOW | (int)SPI_SCLK_TOGGLE_MIDDLE), 0);
    spi_frequency(&spi_master, SCLK_FREQ);
    hal_ssi_toggle_between_frame(&spi_master.hal_ssi_adaptor, ENABLE);
    // wait Slave ready

    for (i = 0; i < TEST_BUF_SIZE; i++) {
        TestBuf[i] = i;
    }

    spi_irq_hook(&spi_master, (spi_irq_handler)master_tr_done_callback, (uint32_t)&spi_master);
    dbg_printf("SPI Master Write Test==> \r\n");
    MasterTxDone = 0;
    while(gpio_read(&GPIO_Syc) == 1);
#if SPI_DMA_DEMO
    spi_master_write_stream_dma(&spi_master, TestBuf, TEST_BUF_SIZE);
#else
    spi_master_write_stream(&spi_master, TestBuf, TEST_BUF_SIZE);
#endif
    i = 0;
    dbg_printf("SPI Master Wait Write Done... \r\n");
    while (MasterTxDone == 0) {
        wait_ms(10);
        i++;
    }
    dbg_printf("SPI Master Write Done!! \r\n");
#else
    spi_init(&spi_slave, SPI0_MOSI, SPI0_MISO, SPI0_SCLK, SPI0_CS);
    spi_format(&spi_slave, DfsSixteenBits, ((int)SPI_SCLK_IDLE_LOW | (int)SPI_SCLK_TOGGLE_MIDDLE), 1);
    hal_ssi_toggle_between_frame(&spi_slave.hal_ssi_adaptor, ENABLE);

    while (spi_busy(&spi_slave)) {
        dbg_printf("Wait SPI Bus Ready... \r\n");
        wait_ms(1000);
    }

    memset(TestReceiveBuf, 0, TEST_RECEIVE_BUF_SIZE);
    dbg_printf("SPI Slave Read Test ==> \r\n");
    spi_irq_hook(&spi_slave, (spi_irq_handler)slave_tr_done_callback, (uint32_t)&spi_slave);
    SlaveRxDone = 0;
    spi_flush_rx_fifo(&spi_slave);
    spi_slave_read_stream_unfix_size( &spi_slave, TestReceiveBuf, TEST_RECEIVE_BUF_SIZE);
    gpio_write(&GPIO_Syc, 0);

    i = 0;
    dbg_printf("SPI Slave Wait Read Done... \r\n");
    while (SlaveRxDone == 0) {
        wait_ms(100);
        i++;
        if (i>1000) {
            dbg_printf("SPI Slave Wait Timeout \r\n");
            break;
        }
    }
    int read_length = (TEST_RECEIVE_BUF_SIZE - spi_slave.hal_ssi_adaptor.rx_length);
    dump_data((const u8*)TestReceiveBuf, read_length, "SPI Slave Read Data:");

    spi_free(&spi_slave);
#endif
    dbg_printf("SPI Demo finished. \r\n");
    for(;;);
}
