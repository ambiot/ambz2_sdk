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

// This example demo the function of Auto Flow control
// Please connect 2 board to run this example.
// Board1   <----->     Board2
// PA16     <----->     PA15
// PA15     <----->     PA16
// PA19     <----->     PA20
// PA20     <----->     PA19
// GND      <----->     GND

// The first started board will be the TX side, the other one will be the RX side
// The RX side will make some delay every 16-bytes received, 
// by this way we can trigger the flow control mechanism.

#include <string.h>
#include "pinmap.h"
#include "serial_api.h"
#include "wait_api.h"
#include "sys_api.h"

#define UART_TX         PA_16    //UART2  TX
#define UART_RX         PA_15    //UART2  RX
#define UART_RTS        PA_20    //UART2  RTS
#define UART_CTS        PA_19    //UART2  CTS
#define LOG_UART_TX     PA_14    //UART0  TX
#define LOG_UART_RX     PA_13    //UART0  RX

extern void log_uart_port_init (int log_uart_tx, int log_uart_rx, uint32_t baud_rate);

extern void serial_clear_rx(serial_t *obj);
/*
void uart_send_string(serial_t *sobj, char *pstr)
{
    unsigned int i=0;

    while (*(pstr+i) != 0) {
        serial_putc(sobj, *(pstr+i));
        i++;
    }
}*/
#define UART_BUF_SIZE   1000

serial_t sobj;
unsigned char buffer[UART_BUF_SIZE];

void main (void)
{
    // sample text
    char rc;
    char sent;
    int i,j;
    int rx_side=0;

    dbg_printf("\r\n   UART Auto Flow Ctrl DEMO   \r\n");

    sys_log_uart_off();
    log_uart_port_init(LOG_UART_TX, LOG_UART_RX, ((uint32_t)115200));

    /*UART2  used*/
    sobj.uart_adp.uart_idx = 2;

    // mbed uart test
    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 38400);
    serial_format(&sobj, 8, ParityNone, 1);
    serial_set_flow_control(&sobj, FlowControlNone, (PinName)0, (PinName)0);// Pin assignment can be ignored when autoflow control function is disabled

    wait_ms(10000);                    
    serial_clear_rx(&sobj);           
    for (sent = 0; sent < 126; sent++) {
        dbg_printf("Wait peer ready... \r\n");
        serial_putc(&sobj, sent);          
        if (serial_readable(&sobj)) {       
            rc = serial_getc(&sobj);         
            if (rc > sent) {                
                rx_side = 1;
                serial_putc(&sobj, 0);
            } else{
                rx_side = 0;
            }
            break;
        }
        wait_ms(100);
    }

    // Enable flow control
    serial_set_flow_control(&sobj, FlowControlRTSCTS, UART_RTS, UART_CTS);
    serial_clear_rx(&sobj);
    wait_ms(5000);

    if (rx_side) {
        dbg_printf("UART Flow Control: RX ==> \r\n");
        memset(buffer, 0, UART_BUF_SIZE);

        i = 0;
        j = 0;
        while (1) {
            if (serial_readable(&sobj)) {
                buffer[i] = serial_getc(&sobj);
                i++;
                if (i == UART_BUF_SIZE) {
                    break;
                }
                if ((i & 0xf) == 0) {
                    // Make some delay to cause the RX FIFO full and then trigger flow controll
                    wait_ms(100);
                    dbg_printf("UART RX got %d bytes \r\n", i);
                }
                j = 0;
            } else {
                wait_ms(10);
                j++;
                if (j == 1000) {
                    dbg_printf("UART RX Failed, Got %d bytes \r\n", i);
                    break;
                }
            }
        }
    } else {
        dbg_printf("UART Flow Control: TX ==> \r\n");
        wait_ms(500);
        for (i = 0; i < UART_BUF_SIZE; i++) {
            buffer[i] = 0x30 + (i % 10);
        }
        for (i = 0; i < UART_BUF_SIZE; i++) {
            serial_putc(&sobj, buffer[i]);
        }
    }

    dbg_printf("UART Flow Control Test Done! \r\n");
    while(1);
}
