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

#define UART_TX    PA_14
#define UART_RX    PA_13

static void uart_send_string(serial_t *sobj, char *pstr)
{
    unsigned int i=0;

    while (*(pstr+i) != 0) {
        serial_putc(sobj, *(pstr+i));
        i++;
    }
}

int main (void)
{
    // sample text
    char rc;
    serial_t sobj;

    dbg_printf("\r\n   UART DEMO   \r\n");

    // mbed uart test
    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 38400);
    serial_format(&sobj, 8, ParityNone, 1);

    uart_send_string(&sobj, "UART API Demo... \r\n");
    uart_send_string(&sobj, "Hello World!! \r\n");
    while (1) {
        uart_send_string(&sobj, "\r\n8710c$");
        rc = serial_getc(&sobj);
        serial_putc(&sobj, rc);
    }
}
