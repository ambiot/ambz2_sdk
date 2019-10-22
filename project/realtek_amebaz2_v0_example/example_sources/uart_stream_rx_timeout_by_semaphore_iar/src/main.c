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

#define UART_TX                 PA_14
#define UART_RX                 PA_13

#define SRX_BUF_SZ              100
#define UART_TIMEOUT_MS         5000 //ms

#define TASK_STACK_SIZE         2048
#define TASK_PRIORITY           (tskIDLE_PRIORITY + 1)

char rx_buf[SRX_BUF_SZ]__attribute__((aligned(32))) = {0};
volatile uint32_t rx_bytes = 0;
SemaphoreHandle_t UartRxSema;
SemaphoreHandle_t UartTxSema;

void uart_send_string_done (uint32_t id)
{
//    serial_t    *sobj = (void*)id;
    signed portBASE_TYPE xHigherPriorityTaskWoken;

    xSemaphoreGiveFromISR(UartTxSema, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void uart_recv_string_done (uint32_t id)
{
    serial_t *sobj = (void*)id;
    signed portBASE_TYPE xHigherPriorityTaskWoken;

    rx_bytes += sobj->uart_adp.rx_count;
    xSemaphoreGiveFromISR(UartRxSema, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void uart_send_string (serial_t *sobj, char *pstr)
{
    int32_t ret = 0;

    xSemaphoreTake(UartTxSema, portMAX_DELAY);

    ret = serial_send_stream(sobj, pstr, strlen(pstr));
    if (ret != 0) {
        dbg_printf("%s Error(%d) \r\n", __FUNCTION__, ret);
        xSemaphoreGive( UartTxSema );
    }
}

void uart_demo (void)
{
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
	rtw_create_secure_context(512);
#endif	
    serial_t sobj;
    int ret;

    dbg_printf("\r\n   UART Stream RX Timeout by Semaphore IAR Demo   \r\n");

    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 115200);
    serial_format(&sobj, 8, ParityNone, 1);

    serial_send_comp_handler(&sobj, (void*)uart_send_string_done, (uint32_t)&sobj);
    serial_recv_comp_handler(&sobj, (void*)uart_recv_string_done, (uint32_t)&sobj);

    // Create semaphore for UART RX done(received espected bytes or timeout)
    UartRxSema = xSemaphoreCreateBinary();

    // Create semaphore for UART TX done
    UartTxSema = xSemaphoreCreateBinary();
    xSemaphoreGive( UartTxSema );    // Ready to TX

    while (1) {
        rx_bytes = 0;
#if 1
        ret = serial_recv_stream(&sobj, rx_buf, 10);    // Interrupt mode
#else
        ret = serial_recv_stream_dma(&sobj, rx_buf, 10);    // DMA mode
#endif
        if (ret != 0) {
            dbg_printf("ERRORS in Interrupt/DMA mode \r\n");
        }

        if (xSemaphoreTake(UartRxSema, ((TickType_t)UART_TIMEOUT_MS / portTICK_RATE_MS)) != pdTRUE ) {
            rx_bytes = serial_recv_stream_abort(&sobj);
        }

        if (rx_bytes > 0) {
            rx_buf[rx_bytes] = 0x00; // end of string
            uart_send_string(&sobj, rx_buf);
        }
    }
}

int main (void)
{
    // create demo Task
    if (xTaskCreate((TaskFunction_t)uart_demo, "UART DEMO", (TASK_STACK_SIZE / 4), NULL, TASK_PRIORITY, NULL) != pdPASS) {
        dbg_printf("Cannot create demo task \r\n");
    }

    vTaskStartScheduler();

    while(1);
}
