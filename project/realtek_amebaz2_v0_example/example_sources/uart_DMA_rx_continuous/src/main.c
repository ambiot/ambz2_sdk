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

#include "serial_api.h"
#include "serial_ex_api.h"
#include "wait_api.h"

#define UART_TX         PA_14
#define UART_RX         PA_13

#define RX_DMA_SZ       2048
#define SRX_BUF_SZ      (RX_DMA_SZ * 6)

char rx_buf[SRX_BUF_SZ]__attribute__((aligned(32))) = {0};
//volatile uint32_t tx_busy=0;
//volatile uint32_t rx_done=0;

int main (void)
{
    serial_t sobj;
    int ret;
    int i;
    unsigned int rxed_len;

    dbg_printf("\r\n   UART DMA RX Continuous DEMO   \r\n");

    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 38400);
    serial_format(&sobj, 8, ParityNone, 1);
    
    while (1) {
        for (i = 0; i < 5; i++) {
            dbg_printf("Ready ... %d \r\n", (5 - i));
            wait_ms(1000);
        }

        dbg_printf("Start DMA RX... \r\n");

        rxed_len = 0;
        for (i = 0; i < 6; i++) {
            ret = serial_recv_stream_dma_timeout(&sobj, (rx_buf + (i * RX_DMA_SZ)), RX_DMA_SZ, 5000, NULL);
            if (ret > 0) {
                rxed_len += ret;
            } else {
                break;
            }
        }
        dbg_printf("RxLen=%d \r\n", rxed_len);
        //__rtl_memDump_v1_00(rx_buf, rxed_len, "Rx Dump:");
    }
}
