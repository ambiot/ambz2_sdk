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
#include "timer_api.h"
#include "serial_api.h"
#include "serial_ex_api.h"

#define UART_TX             PA_14
#define UART_RX             PA_13

#define SRX_BUF_SZ          100
#define UART_TIMEOUT_MS     5000 //ms

char rx_buf[SRX_BUF_SZ]__attribute__((aligned(32))) = {0};
volatile uint32_t tx_busy = 0;
volatile uint32_t rx_done = 0;
volatile uint32_t rx_bytes = 0;
gtimer_t uart_timer;

void uart_send_string_done (uint32_t id)
{
//    serial_t    *sobj = (void*)id;
    tx_busy = 0;
}

void uart_recv_string_done (uint32_t id)
{
    serial_t *sobj = (void*)id;
    gtimer_stop(&uart_timer);
    rx_bytes = sobj->uart_adp.rx_count;
    rx_done = 1;
}

static void uart_send_string (serial_t *sobj, char *pstr)
{
    int32_t ret=0;

    if (tx_busy) {
        return;
    }

    tx_busy = 1;
    ret = serial_send_stream(sobj, pstr, strlen(pstr));
    if (ret != 0) {
        dbg_printf("%s Error(%d) \r\n", __FUNCTION__, ret);
        tx_busy = 0;
    }
}

void UartTimeoutCallbck (uint32_t id)
{
    serial_t *psobj;

    psobj = (serial_t *)id;

    rx_bytes = serial_recv_stream_abort(psobj);
    rx_done = 1;
}

int main (void)
{
    serial_t sobj;
    int ret;

    dbg_printf("\r\n   UART Stream RX Timeout by Gtimer Demo   \r\n");

    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 115200);
    serial_format(&sobj, 8, ParityNone, 1);

    serial_send_comp_handler(&sobj, (void*)uart_send_string_done, (uint32_t)&sobj);
    serial_recv_comp_handler(&sobj, (void*)uart_recv_string_done, (uint32_t)&sobj);

    // Initial a timer to wait UART RX done
    gtimer_init(&uart_timer, TIMER3);

    while (1) {
        // expect to receive maximum 100 bytes with timeout 5000ms
        rx_bytes = 0;
        rx_done = 0;
#if 0
        ret = serial_recv_stream(&sobj, rx_buf, 10); // Interrupt mode
#else
        ret = serial_recv_stream_dma(&sobj, rx_buf, 10); // DMA mode
#endif
        if (ret != 0) {
            dbg_printf("ERRORS in Interrupt/DMA mode \r\n");
        }

        gtimer_start_one_shout(&uart_timer, (UART_TIMEOUT_MS * 1000), (void*)UartTimeoutCallbck, (uint32_t)&sobj);
        while(!rx_done);

        if (rx_bytes > 0) {
            rx_buf[rx_bytes] = 0x00; // end of string
            uart_send_string(&sobj, rx_buf);
        }
    }
}
