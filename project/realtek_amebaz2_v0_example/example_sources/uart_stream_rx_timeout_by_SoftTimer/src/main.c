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

#define UART_TX                PA_14
#define UART_RX                PA_13

#define SRX_BUF_SZ             100
#define UART_TIMEOUT_MS        5000    //ms

char rx_buf[SRX_BUF_SZ]__attribute__((aligned(32))) = {0};
volatile uint32_t rx_bytes = 0;
TimerHandle_t UartTimerHandle;
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

    xTimerStopFromISR(UartTimerHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
        xSemaphoreGive(UartTxSema);
    }
}

void UartTimeoutCallbck (xTimerHandle pxTimer)
{
    serial_t *psobj;

    psobj = (serial_t*) pvTimerGetTimerID(pxTimer);

    rx_bytes = serial_recv_stream_abort(psobj);
    xSemaphoreGive(UartRxSema);
}

int uart_test_demo (void *param)
{
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
	rtw_create_secure_context(512);
#endif	
    serial_t sobj;
    int ret;

    dbg_printf("\r\n   UART Stream RX Timeout by SoftTimer Demo   \r\n");

    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 115200);
    serial_format(&sobj, 8, ParityNone, 1);

    serial_send_comp_handler(&sobj, (void*)uart_send_string_done, (uint32_t)&sobj);
    serial_recv_comp_handler(&sobj, (void*)uart_recv_string_done, (uint32_t)&sobj);

    // Create a Software Timer to wait UART RX done
    UartTimerHandle = xTimerCreate((const char*)"UART_WAIT", (UART_TIMEOUT_MS / portTICK_RATE_MS), 0, (void *)&sobj, UartTimeoutCallbck);
    // Create semaphore for UART RX done(received espected bytes or timeout)
    UartRxSema = xSemaphoreCreateBinary();

    // Create semaphore for UART TX done
    UartTxSema = xSemaphoreCreateBinary();
    xSemaphoreGive(UartTxSema);    // Ready to TX

    while (1) {
        rx_bytes = 0;
#if 0
        ret = serial_recv_stream(&sobj, rx_buf, 10);    // Interrupt mode
#else
        ret = serial_recv_stream_dma(&sobj, rx_buf, 10);    // DMA mode
#endif
        if (ret != 0) {
            dbg_printf("ERRORS in Interrupt/DMA mode \r\n");
        }
        xTimerStart(UartTimerHandle, 0);
        xSemaphoreTake(UartRxSema, portMAX_DELAY);

        if (rx_bytes > 0) {
            rx_buf[rx_bytes] = 0x00;    // end of string
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
