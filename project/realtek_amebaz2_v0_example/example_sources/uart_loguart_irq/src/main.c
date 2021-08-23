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

#define UART_TX    PA_16	//PINMUX of log uart
#define UART_RX    PA_15

volatile char rc = 0;

static void uart_send_string (serial_t *sobj, char *pstr)
{
    unsigned int i = 0;

    while (*(pstr+i) != 0) {
        serial_putc(sobj, *(pstr+i));
        i++;
    }
}

void uart_irq (uint32_t id, SerialIrq event)
{
    serial_t *sobj = (void*)id;

    if (event == RxIrq) {
        rc = serial_getc(sobj);
        serial_putc(sobj, rc);
    }

    if((event == TxIrq) && (rc != 0)){
        uart_send_string(sobj, "\r\n8710c$");
        rc = 0;
    }
}
extern hal_uart_adapter_t log_uart;	//extern log_uart adapter
int main (void)
{
    // sample text
    serial_t sobj;

    dbg_printf("\r\n   UART IRQ Demo   \r\n");

    // mbed uart test
    hal_uart_deinit(&log_uart);		//deinit log uart(which was inited by boot phase).
    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 115200);
    serial_format(&sobj, 8, ParityNone, 1);

    uart_send_string(&sobj, "UART IRQ API Demo... \r\n");
    uart_send_string(&sobj, "Hello World!! \r\n");
    uart_send_string(&sobj, "\r\n8710c$");
    serial_irq_handler(&sobj, uart_irq, (uint32_t)&sobj);
    serial_irq_set(&sobj, RxIrq, 1);
    serial_irq_set(&sobj, TxIrq, 1);

    while(1);
}
