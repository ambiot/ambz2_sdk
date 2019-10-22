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
#include "serial_api.h"
#include "serial_ex_api.h"

#define DMA_MODE      0 // 0: interrupt mode, 1: DMA mode

#define UART_TX       PA_14
#define UART_RX       PA_13

#define SRX_BUF_SZ    100

char rx_buf[SRX_BUF_SZ] = {0};
volatile uint32_t tx_busy = 0;
volatile uint32_t rx_done = 0;

void uart_send_string_done (uint32_t id)
{
//    serial_t    *sobj = (void*)id;
    tx_busy = 0;
}

static void uart_send_string (serial_t *sobj, char *pstr)
{
    int32_t ret = 0;

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

void Release_CPU (void)
{
    // while waitting UART transfer done, try to wakeup other task
#if 1
    // make this task to sleep, so other task can wakeup
    vTaskDelay(10 / portTICK_RATE_MS);
#else
    // force the OS scheduler to do a context switch, but if the
    // priority of this task is the highest then no other task can wake up
    taskYIELD();
#endif
}

void uart_test_demo (void *param)
{
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
	rtw_create_secure_context(512);
#endif	
    serial_t sobj;
    int ret;

    dbg_printf("\r\n   UART Stream RX Timeout Demo   \r\n");

    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 38400);
    serial_format(&sobj, 8, ParityNone, 1);

    serial_send_comp_handler(&sobj, (void*)uart_send_string_done, (uint32_t)&sobj);

    while (1) {
        // expect to receive maximum 13 bytes with timeout 1000ms
#if DMA_MODE
    #if 0
        // If you don't know what is Task Yield or no RTOS, then just keep the last argument is NULL
        ret = serial_recv_stream_dma_timeout(&sobj, rx_buf, 13, 5000, NULL);
    #else
        // Do Task Yield while waitting UART RX done
        ret = serial_recv_stream_dma_timeout(&sobj, rx_buf, 13, 5000, (void *)Release_CPU);
    #endif
#else
    #if 1
        // If you don't know what is Task Yield or no RTOS, then just keep the last argument is NULL
        ret = serial_recv_stream_timeout(&sobj, rx_buf, 13, 1000, NULL);
    #else
        // Do Task Yield while waitting UART RX done
        ret = serial_recv_stream_timeout(&sobj, rx_buf, 13, 1000, Release_CPU);
    #endif
#endif
        if (ret < 100) {
            dbg_printf("Serial Rcv Timeout, Got %d bytes \r\n", ret);
        }

        if (ret > 0) {
            rx_buf[ret] = 0x00; // end of string
            uart_send_string(&sobj, rx_buf);
        }
    }
}

int main (void)
{
    // create demo Task
    if (xTaskCreate((TaskFunction_t)uart_test_demo, "uart test demo", (2048 / 2), (void *)NULL, (tskIDLE_PRIORITY + 1), NULL) != pdPASS) {
        dbg_printf("Cannot create uart test demo task \r\n");
        goto end_demo;
    }

    vTaskStartScheduler();

end_demo:
    while(1);
}
